#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>
#include <pthread.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>

#define LOG_TAG "MemTool"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static ModInfo g_modinfo("com.burhan.memtool", "MemTool", "1.1", "Burhanudin");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;

#define CMD_FILE    "/storage/emulated/0/Download/mem_cmd.txt"
#define RESULT_FILE "/storage/emulated/0/Download/mem_result.txt"

struct ScanResult {
    uintptr_t address;
    float     fval;
    int       ival;
    bool      isFloat;
};

struct LockEntry {
    uintptr_t address;
    float     fval;
    int       ival;
    bool      isFloat;
    bool      active;
};

struct MemRegion {
    uintptr_t start;
    uintptr_t end;
    char      perms[8];
    char      name[128];
};

static std::vector<ScanResult> g_results;
static std::vector<ScanResult> g_unknown_prev;
static std::vector<LockEntry>  g_locks;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

// ── Baca region READABLE dari /proc/self/maps ─────────
std::vector<MemRegion> GetReadableRegions(bool doGTASA, bool doSAMP, bool doHeap)
{
    std::vector<MemRegion> regions;
    FILE* maps = fopen("/proc/self/maps", "r");
    if(!maps) return regions;

    char line[512];
    while(fgets(line, sizeof(line), maps))
    {
        MemRegion r;
        memset(&r, 0, sizeof(r));
        char extra[256] = "";

        // format: start-end perms offset dev inode name
        sscanf(line, "%x-%x %4s %*s %*s %*s %127s",
               &r.start, &r.end, r.perms, r.name);

        // harus readable
        if(r.perms[0] != 'r') continue;
        // jangan executable pure (code section) — bisa crash
        // tapi boleh r--, r-p, rw-
        if(r.perms[2] == 'x') continue;
        // skip ukuran terlalu besar (>64MB)
        if(r.end - r.start > 0x4000000) continue;
        // skip region kosong
        if(r.start == r.end) continue;

        bool isGTASA = strstr(r.name, "libGTASA") != nullptr;
        bool isSAMP  = strstr(r.name, "libsamp")  != nullptr;
        bool isHeap  = strstr(r.name, "[heap]")   != nullptr ||
                       (r.name[0] == '\0' && strstr(r.perms, "rw") != nullptr);

        if(doGTASA && isGTASA) { regions.push_back(r); continue; }
        if(doSAMP  && isSAMP)  { regions.push_back(r); continue; }
        if(doHeap  && isHeap)  { regions.push_back(r); continue; }
    }
    fclose(maps);
    LOGI("Readable regions: %zu", regions.size());
    return regions;
}

// ── Search ────────────────────────────────────────────
void DoSearch(float fval, int ival, bool sf, bool si,
              bool doGTASA, bool doSAMP, bool doHeap, int maxR)
{
    pthread_mutex_lock(&g_mutex);
    g_results.clear();

    auto regions = GetReadableRegions(doGTASA, doSAMP, doHeap);
    int found = 0;

    for(auto& r : regions)
    {
        for(uintptr_t addr = r.start; addr <= r.end - 4 && found < maxR; addr += 4)
        {
            if(sf)
            {
                float v = *(float*)addr;
                if(!std::isnan(v) && !std::isinf(v))
                {
                    bool match = (fval == 0.0f) ? (v == 0.0f) :
                                 (fabsf(v - fval) / fabsf(fval) < 0.001f);
                    if(match)
                    {
                        g_results.push_back({addr, v, 0, true});
                        found++;
                        continue;
                    }
                }
            }
            if(si && found < maxR)
            {
                int v = *(int*)addr;
                if(v == ival)
                {
                    g_results.push_back({addr, 0.0f, v, false});
                    found++;
                }
            }
        }
        if(found >= maxR) break;
    }

    LOGI("Search done: %d results", found);
    pthread_mutex_unlock(&g_mutex);
}

// ── Unknown first scan ────────────────────────────────
void DoUnknownFirstScan(bool doGTASA, bool doSAMP, bool doHeap)
{
    pthread_mutex_lock(&g_mutex);
    g_unknown_prev.clear();

    auto regions = GetReadableRegions(doGTASA, doSAMP, doHeap);
    for(auto& r : regions)
    {
        for(uintptr_t addr = r.start; addr <= r.end - 4; addr += 4)
        {
            float v = *(float*)addr;
            if(!std::isnan(v) && !std::isinf(v))
            {
                g_unknown_prev.push_back({addr, v, *(int*)addr, true});
            }
        }
    }
    LOGI("Unknown first scan: %zu values", g_unknown_prev.size());
    pthread_mutex_unlock(&g_mutex);
}

// ── Unknown filter ────────────────────────────────────
void DoUnknownFilter(const char* cond, int maxR)
{
    pthread_mutex_lock(&g_mutex);
    g_results.clear();
    int found = 0;

    for(auto& prev : g_unknown_prev)
    {
        if(found >= maxR) break;
        float cur = *(float*)prev.address;
        if(std::isnan(cur) || std::isinf(cur)) continue;

        bool match = false;
        if(strcmp(cond, "changed")   == 0) match = (cur != prev.fval);
        if(strcmp(cond, "unchanged") == 0) match = (cur == prev.fval);
        if(strcmp(cond, "increased") == 0) match = (cur >  prev.fval);
        if(strcmp(cond, "decreased") == 0) match = (cur <  prev.fval);

        if(match)
        {
            g_results.push_back({prev.address, cur, *(int*)prev.address, true});
            found++;
        }
        prev.fval = cur;
        prev.ival = *(int*)prev.address;
    }
    LOGI("Unknown filter '%s': %d results", cond, found);
    pthread_mutex_unlock(&g_mutex);
}

// ── Tulis hasil ───────────────────────────────────────
void WriteResults()
{
    FILE* f = fopen(RESULT_FILE, "w");
    if(!f) { LOGI("Cannot open result file!"); return; }

    pthread_mutex_lock(&g_mutex);
    int cnt = (int)g_results.size();
    fprintf(f, "count=%d\n", cnt);
    for(int i = 0; i < cnt; i++)
    {
        auto& r = g_results[i];
        if(r.isFloat)
            fprintf(f, "%d=0x%X,float,%.4f\n", i+1, r.address, r.fval);
        else
            fprintf(f, "%d=0x%X,int,%d\n",     i+1, r.address, r.ival);
    }
    pthread_mutex_unlock(&g_mutex);
    fclose(f);
    LOGI("Results written: %d", cnt);
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
            if(lk.isFloat) *(float*)lk.address = lk.fval;
            else           *(int*)lk.address   = lk.ival;
        }
        pthread_mutex_unlock(&g_mutex);
        usleep(50000);
    }
    return nullptr;
}

// ── Parse command ─────────────────────────────────────
void ParseCommand(const char* cmd)
{
    LOGI("CMD: %s", cmd);

    if(strncmp(cmd, "search ", 7) == 0)
    {
        float fval = 0; int ival = 0;
        char typeStr[16]  = "float";
        char rangeStr[16] = "gtasa";
        int  maxR = 50;
        sscanf(cmd + 7, "%f %15s %15s %d", &fval, typeStr, rangeStr, &maxR);
        ival = (int)fval;

        bool sf = strstr(typeStr,  "float") || strstr(typeStr,  "both");
        bool si = strstr(typeStr,  "int")   || strstr(typeStr,  "both");
        bool rg = strstr(rangeStr, "gtasa") || strstr(rangeStr, "all");
        bool rs = strstr(rangeStr, "samp")  || strstr(rangeStr, "all");
        bool rh = strstr(rangeStr, "heap")  || strstr(rangeStr, "all");

        DoSearch(fval, ival, sf, si, rg, rs, rh, maxR);
        WriteResults();
    }
    else if(strncmp(cmd, "unknown_first", 13) == 0)
    {
        char rangeStr[16] = "gtasa";
        sscanf(cmd + 13, " %15s", rangeStr);
        bool rg = strstr(rangeStr, "gtasa") || strstr(rangeStr, "all");
        bool rs = strstr(rangeStr, "samp")  || strstr(rangeStr, "all");
        bool rh = strstr(rangeStr, "heap")  || strstr(rangeStr, "all");
        DoUnknownFirstScan(rg, rs, rh);
        FILE* f = fopen(RESULT_FILE, "w");
        if(f) { fprintf(f, "status=first_scan_done\n"); fclose(f); }
    }
    else if(strncmp(cmd, "unknown_filter ", 15) == 0)
    {
        DoUnknownFilter(cmd + 15, 50);
        WriteResults();
    }
    else if(strncmp(cmd, "set ", 4) == 0)
    {
        int idx = 0; float val = 0;
        sscanf(cmd + 4, "%d %f", &idx, &val);
        idx--;
        pthread_mutex_lock(&g_mutex);
        if(idx >= 0 && idx < (int)g_results.size())
        {
            auto& r = g_results[idx];
            if(r.isFloat) *(float*)r.address = val;
            else          *(int*)r.address   = (int)val;
            LOGI("Set [%d] 0x%X = %.2f", idx+1, r.address, val);
        }
        pthread_mutex_unlock(&g_mutex);
    }
    else if(strncmp(cmd, "lock ", 5) == 0)
    {
        int idx = 0; float val = 0;
        sscanf(cmd + 5, "%d %f", &idx, &val);
        idx--;
        pthread_mutex_lock(&g_mutex);
        if(idx >= 0 && idx < (int)g_results.size())
        {
            auto& r = g_results[idx];
            LockEntry lk;
            lk.address = r.address;
            lk.fval    = val;
            lk.ival    = (int)val;
            lk.isFloat = r.isFloat;
            lk.active  = true;
            g_locks.push_back(lk);
            LOGI("Locked 0x%X = %.2f", r.address, val);
        }
        pthread_mutex_unlock(&g_mutex);
    }
    else if(strncmp(cmd, "unlock ", 7) == 0)
    {
        int idx = 0;
        sscanf(cmd + 7, "%d", &idx);
        pthread_mutex_lock(&g_mutex);
        if(idx >= 0 && idx < (int)g_locks.size())
            g_locks[idx].active = false;
        pthread_mutex_unlock(&g_mutex);
    }
}

// ── Command watcher ───────────────────────────────────
void* CmdWatcher(void*)
{
    char lastCmd[512] = "";
    while(true)
    {
        FILE* f = fopen(CMD_FILE, "r");
        if(f)
        {
            char cmd[512] = "";
            fgets(cmd, sizeof(cmd), f);
            fclose(f);

            int len = strlen(cmd);
            if(len > 0 && cmd[len-1] == '\n') cmd[len-1] = '\0';

            if(strlen(cmd) > 0 && strcmp(cmd, lastCmd) != 0)
            {
                strncpy(lastCmd, cmd, sizeof(lastCmd)-1);
                ParseCommand(cmd);
                FILE* fc = fopen(CMD_FILE, "w");
                if(fc) fclose(fc);
            }
        }
        usleep(200000);
    }
    return nullptr;
}

// ── Entry point ───────────────────────────────────────
extern "C" __attribute__((visibility("default"))) ModInfo* __GetModInfo() { return modinfo; }

extern "C" __attribute__((visibility("default"))) void OnModLoad()
{
    aml = (IAML*)GetInterface("AMLInterface");
    if(!aml) return;

    LOGI("MemTool v1.1 loaded!");

    pthread_t t1, t2;
    pthread_create(&t1, nullptr, CmdWatcher,  nullptr);
    pthread_create(&t2, nullptr, LockThread,  nullptr);
    pthread_detach(t1);
    pthread_detach(t2);

    aml->ShowToast(true, "MemTool v1.1 ready!");
}
