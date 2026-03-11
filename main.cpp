#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>
#include <pthread.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <vector>
#include <string>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"

#define LOG_TAG "MemTool"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static ModInfo g_modinfo("com.burhan.memtool", "MemTool", "2.0", "Burhanudin");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;

// ── ImGui state ───────────────────────────────────────
static bool g_imguiInited   = false;
static bool g_showWindow    = true;
static int  g_screenW       = 1280;
static int  g_screenH       = 720;

// ── Scan state ────────────────────────────────────────
struct ScanResult { uintptr_t addr; float fval; int ival; bool isFloat; };
struct LockEntry  { uintptr_t addr; float fval; int ival; bool isFloat; bool active; std::string label; };

static std::vector<ScanResult> g_results;
static std::vector<ScanResult> g_unknownPrev;
static std::vector<LockEntry>  g_locks;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

static char  g_searchVal[32]  = "100";
static int   g_maxResults     = 50;
static bool  g_typeFloat      = true;
static bool  g_typeInt        = false;
static bool  g_rangeGTASA     = true;
static bool  g_rangeSAMP      = false;
static bool  g_rangeHeap      = false;
static bool  g_scanning       = false;
static bool  g_unknownMode    = false;
static char  g_statusMsg[128] = "Ready";

// ── Touch passthrough ─────────────────────────────────
typedef struct { int type; int action; float x; float y; int ptr; } NVTouch;
typedef struct { int type; union { NVTouch touch; }; } NVEvent;

int (*origGetNextEvent)(NVEvent*, int) = nullptr;

int HookedGetNextEvent(NVEvent* ev, int waitMS)
{
    int result = origGetNextEvent(ev, waitMS);
    if(result && ev && g_showWindow)
    {
        // NV_EVENT_MULTITOUCH = 6
        if(ev->type == 6)
        {
            ImGuiIO& io = ImGui::GetIO();
            float x = ev->touch.x * g_screenW;
            float y = ev->touch.y * g_screenH;
            // action 0=down, 1=up, 2=move
            if(ev->touch.action == 0) { io.MousePos = ImVec2(x,y); io.MouseDown[0] = true;  }
            if(ev->touch.action == 1) { io.MousePos = ImVec2(x,y); io.MouseDown[0] = false; }
            if(ev->touch.action == 2) { io.MousePos = ImVec2(x,y); }

            // kalau ImGui handle touch ini, consume event
            if(io.WantCaptureMouse) ev->type = 0;
        }
    }
    return result;
}

// ── Readable memory regions ───────────────────────────
struct MemRegion { uintptr_t start, end; char name[64]; };

std::vector<MemRegion> GetRegions()
{
    std::vector<MemRegion> v;
    FILE* f = fopen("/proc/self/maps", "r");
    if(!f) return v;
    char line[512];
    while(fgets(line, sizeof(line), f))
    {
        MemRegion r; char perms[8]=""; r.name[0]=0;
        sscanf(line, "%x-%x %4s %*s %*s %*s %63s", &r.start, &r.end, perms, r.name);
        if(perms[0] != 'r') continue;
        if(perms[2] == 'x') continue;
        if(r.end - r.start > 0x4000000) continue;

        bool isGTASA = strstr(r.name,"libGTASA") != nullptr;
        bool isSAMP  = strstr(r.name,"libsamp")  != nullptr;
        bool isHeap  = strstr(r.name,"[heap]")   != nullptr ||
                       (r.name[0]==0 && perms[1]=='w');

        if((g_rangeGTASA && isGTASA) ||
           (g_rangeSAMP  && isSAMP)  ||
           (g_rangeHeap  && isHeap))
            v.push_back(r);
    }
    fclose(f);
    return v;
}

// ── Scan thread ───────────────────────────────────────
struct ScanArgs { float fval; int ival; int maxR; bool sf, si; std::string filter; };

void* ScanThread(void* arg)
{
    ScanArgs* a = (ScanArgs*)arg;
    auto regions = GetRegions();

    pthread_mutex_lock(&g_mutex);
    g_results.clear();
    int found = 0;

    if(a->filter.empty()) // normal search
    {
        for(auto& r : regions)
        {
            for(uintptr_t addr = r.start; addr <= r.end-4 && found < a->maxR; addr += 4)
            {
                if(a->sf)
                {
                    float v = *(float*)addr;
                    if(!std::isnan(v) && !std::isinf(v))
                    {
                        bool m = (a->fval==0.f) ? v==0.f : fabsf(v-a->fval)/fabsf(a->fval)<0.001f;
                        if(m) { g_results.push_back({addr,v,0,true}); found++; continue; }
                    }
                }
                if(a->si && found < a->maxR)
                {
                    int v = *(int*)addr;
                    if(v == a->ival) { g_results.push_back({addr,0.f,v,false}); found++; }
                }
            }
            if(found >= a->maxR) break;
        }
        snprintf(g_statusMsg, sizeof(g_statusMsg), "Found: %d results", found);
    }
    else if(a->filter == "first") // unknown first scan
    {
        g_unknownPrev.clear();
        for(auto& r : regions)
            for(uintptr_t addr = r.start; addr <= r.end-4; addr += 4)
            {
                float v = *(float*)addr;
                if(!std::isnan(v) && !std::isinf(v))
                    g_unknownPrev.push_back({addr,v,*(int*)addr,true});
            }
        snprintf(g_statusMsg, sizeof(g_statusMsg), "First scan: %zu values stored", g_unknownPrev.size());
        g_unknownMode = true;
    }
    else // unknown filter: changed/unchanged/increased/decreased
    {
        for(auto& prev : g_unknownPrev)
        {
            if(found >= a->maxR) break;
            float cur = *(float*)prev.addr;
            if(std::isnan(cur)||std::isinf(cur)) continue;
            bool m = false;
            if(a->filter=="changed")   m = cur != prev.fval;
            if(a->filter=="unchanged") m = cur == prev.fval;
            if(a->filter=="increased") m = cur >  prev.fval;
            if(a->filter=="decreased") m = cur <  prev.fval;
            if(m) { g_results.push_back({prev.addr,cur,*(int*)prev.addr,true}); found++; }
            prev.fval = cur;
        }
        snprintf(g_statusMsg, sizeof(g_statusMsg), "Filter '%s': %d results", a->filter.c_str(), found);
    }

    pthread_mutex_unlock(&g_mutex);
    g_scanning = false;
    delete a;
    return nullptr;
}

void StartScan(const std::string& filter="")
{
    if(g_scanning) return;
    g_scanning = true;
    snprintf(g_statusMsg, sizeof(g_statusMsg), "Scanning...");
    ScanArgs* a = new ScanArgs();
    a->fval   = atof(g_searchVal);
    a->ival   = atoi(g_searchVal);
    a->maxR   = g_maxResults;
    a->sf     = g_typeFloat;
    a->si     = g_typeInt;
    a->filter = filter;
    pthread_t t;
    pthread_create(&t, nullptr, ScanThread, a);
    pthread_detach(t);
}

// ── Lock thread ───────────────────────────────────────
void* LockThread(void*)
{
    while(true)
    {
        pthread_mutex_lock(&g_mutex);
        for(auto& lk : g_locks)
        {
            if(!lk.active) continue;
            if(lk.isFloat) *(float*)lk.addr = lk.fval;
            else           *(int*)lk.addr   = lk.ival;
        }
        pthread_mutex_unlock(&g_mutex);
        usleep(50000);
    }
    return nullptr;
}

// ── Render GUI ────────────────────────────────────────
void RenderGUI()
{
    ImGui::SetNextWindowSize(ImVec2(400, 520), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(10, 50),    ImGuiCond_FirstUseEver);
    ImGui::Begin("MemTool v2.0 | brruham", &g_showWindow);

    // Value + Max
    ImGui::Text("Value:"); ImGui::SameLine();
    ImGui::SetNextItemWidth(110);
    ImGui::InputText("##v", g_searchVal, 32);
    ImGui::SameLine(); ImGui::Text("Max:"); ImGui::SameLine();
    ImGui::SetNextItemWidth(55);
    ImGui::InputInt("##m", &g_maxResults);
    if(g_maxResults < 1)   g_maxResults = 1;
    if(g_maxResults > 500) g_maxResults = 500;

    // Type
    ImGui::Text("Type: ");
    ImGui::SameLine(); ImGui::Checkbox("Float", &g_typeFloat);
    ImGui::SameLine(); ImGui::Checkbox("Int",   &g_typeInt);

    // Range
    ImGui::Text("Range:");
    ImGui::SameLine(); ImGui::Checkbox("GTASA", &g_rangeGTASA);
    ImGui::SameLine(); ImGui::Checkbox("SAMP",  &g_rangeSAMP);
    ImGui::SameLine(); ImGui::Checkbox("Heap",  &g_rangeHeap);

    ImGui::Spacing();

    // Search / Rescan
    if(g_scanning)
    {
        ImGui::TextColored(ImVec4(1,1,0,1), "Scanning...");
    }
    else
    {
        if(ImGui::Button("Search"))  StartScan();
        if(!g_results.empty()) { ImGui::SameLine(); if(ImGui::Button("Rescan")) StartScan(); }
    }

    ImGui::Separator();

    // Unknown scan
    ImGui::TextColored(ImVec4(0.9f,0.9f,0.2f,1), "Unknown Scan:");
    if(!g_scanning)
    {
        if(ImGui::Button("First Scan")) StartScan("first");
    }
    if(g_unknownMode && !g_scanning)
    {
        ImGui::SameLine(); if(ImGui::Button("Changed"))   StartScan("changed");
        ImGui::SameLine(); if(ImGui::Button("Unchanged")) StartScan("unchanged");
        if(ImGui::Button("Increased")) StartScan("increased");
        ImGui::SameLine(); if(ImGui::Button("Decreased")) StartScan("decreased");
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.4f,1,0.4f,1), "%s", g_statusMsg);
    ImGui::Separator();

    // Results
    ImGui::Text("Results (%zu):", g_results.size());
    ImGui::BeginChild("##res", ImVec2(0,160), true);
    pthread_mutex_lock(&g_mutex);
    for(int i = 0; i < (int)g_results.size(); i++)
    {
        auto& r = g_results[i];
        char label[64];
        if(r.isFloat) snprintf(label, sizeof(label), "[%d] 0x%X = %.3f (float)", i+1, r.addr, r.fval);
        else          snprintf(label, sizeof(label), "[%d] 0x%X = %d (int)",     i+1, r.addr, r.ival);
        ImGui::Text("%s", label);
        ImGui::SameLine();
        char btnS[16]; snprintf(btnS, sizeof(btnS), "Set##s%d", i);
        if(ImGui::SmallButton(btnS))
        {
            float v = atof(g_searchVal);
            if(r.isFloat) *(float*)r.addr = v;
            else          *(int*)r.addr   = (int)v;
        }
        ImGui::SameLine();
        char btnL[16]; snprintf(btnL, sizeof(btnL), "Lock##l%d", i);
        if(ImGui::SmallButton(btnL))
        {
            float v = atof(g_searchVal);
            LockEntry lk;
            lk.addr = r.addr; lk.fval = v; lk.ival = (int)v;
            lk.isFloat = r.isFloat; lk.active = true;
            lk.label = label;
            g_locks.push_back(lk);
        }
    }
    pthread_mutex_unlock(&g_mutex);
    ImGui::EndChild();

    ImGui::Separator();
    ImGui::Text("Locked (%zu):", g_locks.size());
    ImGui::BeginChild("##lks", ImVec2(0,90), true);
    int removeIdx = -1;
    for(int i = 0; i < (int)g_locks.size(); i++)
    {
        auto& lk = g_locks[i];
        ImGui::TextColored(lk.active ? ImVec4(1,0.8f,0.2f,1) : ImVec4(0.5f,0.5f,0.5f,1),
            "[%d] 0x%X = %s", i+1, lk.addr, lk.active ? "LOCKED" : "OFF");
        ImGui::SameLine();
        char btnU[16]; snprintf(btnU, sizeof(btnU), "Unlock##u%d", i);
        if(ImGui::SmallButton(btnU)) { lk.active = false; removeIdx = i; }
    }
    if(removeIdx >= 0) g_locks.erase(g_locks.begin() + removeIdx);
    ImGui::EndChild();

    ImGui::End();
}

// ── EGL hooks ─────────────────────────────────────────
void (*origEGLSwapBuffers)()  = nullptr;
void (*origEGLMakeCurrent)()  = nullptr;

void HookedEGLSwapBuffers()
{
    if(!g_imguiInited)
    {
        if(origEGLSwapBuffers) origEGLSwapBuffers();
        return;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    if(g_showWindow) RenderGUI();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if(origEGLSwapBuffers) origEGLSwapBuffers();
}

void HookedEGLMakeCurrent()
{
    if(origEGLMakeCurrent) origEGLMakeCurrent();

    if(!g_imguiInited)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(g_screenW, g_screenH);
        io.IniFilename = nullptr;

        ImGui::StyleColorsDark();
        ImGui::GetStyle().ScaleAllSizes(2.5f); // scale untuk mobile
        io.FontGlobalScale = 2.0f;

        ImGui_ImplOpenGL3_Init("#version 100");
        g_imguiInited = true;
        LOGI("ImGui initialized!");
    }
}

// ── Entry point ───────────────────────────────────────
extern "C" __attribute__((visibility("default"))) ModInfo* __GetModInfo() { return modinfo; }

extern "C" __attribute__((visibility("default"))) void OnModLoad()
{
    aml = (IAML*)GetInterface("AMLInterface");
    if(!aml) return;

    uintptr_t pGTASA = aml->GetLib("libGTASA.so");
    if(!pGTASA) return;

    LOGI("MemTool v2.0 loaded, base: 0x%X", pGTASA);

    // hook EGL
    aml->Hook((void*)(pGTASA + 0x268f4c), (void*)HookedEGLSwapBuffers, (void**)&origEGLSwapBuffers);
    aml->Hook((void*)(pGTASA + 0x268dd0), (void*)HookedEGLMakeCurrent,  (void**)&origEGLMakeCurrent);

    // hook touch input
    aml->Hook((void*)(pGTASA + 0x2696b4), (void*)HookedGetNextEvent, (void**)&origGetNextEvent);

    // start lock thread
    pthread_t t;
    pthread_create(&t, nullptr, LockThread, nullptr);
    pthread_detach(t);

    aml->ShowToast(true, "MemTool v2.0 Pure AML!");
}
