#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>
#include <pthread.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <string>

#define LOG_TAG "MemTool"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static ModInfo g_modinfo("com.burhan.memtool", "MemTool", "1.0", "Burhanudin");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;

#define CMD_FILE    "/storage/emulated/0/Download/mem_cmd.txt"
#define RESULT_FILE "/storage/emulated/0/Download/mem_result.txt"
#define LOCK_FILE   "/storage/emulated/0/Download/mem_lock.txt"

// ── Struktur ──────────────────────────────────────────
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

static std::vector<ScanResult> g_results;
static std::vector<ScanResult> g_unknown_prev; // untuk unknown scan
static std::vector<LockEntry>  g_locks;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

// ── Scan range ────────────────────────────────────────
struct MemRange {
    uintptr_t start;
    uintptr_t end;
};

std::vector<MemRange> GetScanRanges(bool doGTASA, bool doSAMP, bool doHeap)
{
    std::vector<MemRange> ranges;
    if(doGTASA && aml->GetLib("libGTASA.so"))
    {
        uintptr_t base = aml->GetLib("libGTASA.so");
        ranges.push_back({base, base + 0xC00000}); // ~12MB
    }
    if(doSAMP && aml->GetLib("libsamp.so"))
    {
        uintptr_t base = aml->GetLib("libsamp.so");
        ranges.push_back({base, base + 0x200000}); // ~2MB
    }
    if(doHeap)
    {
        // baca /proc/self/maps untuk heap ranges
        FILE* maps = fopen("/proc/self/maps", "r");
        if(maps)
        {
            char line[256];
            while(fgets(line, sizeof(line), maps))
            {
                if(strstr(line, "[heap]") || strstr(line, "anon"))
                {
                    uintptr_t start, end;
                    if(sscanf(line, "%x-%x", &start, &end) == 2)
                    {
                        if(end - start < 0x4000000) // max 64MB per range
                            ranges.push_back({start, end});
                    }
                }
            }
            fclose(maps);
        }
    }
    return ranges;
}

// ── Safe memory read ──────────────────────────────────
bool SafeReadFloat(uintptr_t addr, float& out)
{
    // cek alignment
    if(addr % 4 != 0) return false;
    out = *(float*)addr;
    return true;
}

bool SafeReadInt(uintptr_t addr, int& out)
{
    if(addr % 4 != 0) return false;
    out = *(int*)addr;
    return true;
}

// ── Search ────────────────────────────────────────────
void DoSearch(float fval, int ival, bool searchFloat, bool searchInt,
              bool doGTASA, bool doSAMP, bool doHeap)
{
    pthread_mutex_lock(&g_mutex);
    g_results.clear();

    auto ranges = GetScanRanges(doGTASA, doSAMP, doHeap);
    int found = 0;

    for(auto& r : ranges)
    {
        for(uintptr_t addr = r.start; addr < r.end - 4 && found < 50; addr += 4)
        {
            if(searchFloat)
            {
                float v;
                if(SafeReadFloat(addr, v))
                {
                    if(v == fval || (fval != 0 && fabsf(v - fval) / fabsf(fval) < 0.001f))
                    {
                        ScanResult sr;
                        sr.address = addr;
                        sr.fval = v;
                        sr.ival = 0;
                        sr.isFloat = true;
                        g_results.push_back(sr);
                        found++;
                    }
                }
            }
            if(searchInt && found < 50)
            {
                int v;
                if(SafeReadInt(addr, v))
                {
                    if(v == ival)
                    {
                        ScanResult sr;
                        sr.address = addr;
                        sr.fval = 0;
                        sr.ival = v;
                        sr.isFloat = false;
                        g_results.push_back(sr);
                        found++;
                    }
                }
            }
        }
        if(found >= 50) break;
    }

    LOGI("Search done, found %d results", found);
    pthread_mutex_unlock(&g_mutex);
}

// ── Unknown scan (first scan) ─────────────────────────
void DoUnknownFirstScan(bool doGTASA, bool doSAMP, bool doHeap)
{
    pthread_mutex_lock(&g_mutex);
    g_unknown_prev.clear();

    auto ranges = GetScanRanges(doGTASA, doSAMP, doHeap);
    for(auto& r : ranges)
    {
        for(uintptr_t addr = r.start; addr < r.end - 4; addr += 4)
        {
            float v;
            if(SafeReadFloat(addr, v))
            {
                ScanResult sr;
                sr.address = addr;
                sr.fval = v;
                sr.ival = *(int*)addr;
                sr.isFloat = true;
                g_unknown_prev.push_back(sr);
            }
        }
    }

    LOGI("Unknown first scan done, stored %zu values", g_unknown_prev.size());
    pthread_mutex_unlock(&g_mutex);
}

// ── Unknown filter ────────────────────────────────────
void DoUnknownFilter(const char* condition) // "changed","unchanged","increased","decreased"
{
    pthread_mutex_lock(&g_mutex);
    g_results.clear();
    int found = 0;

    for(auto& prev : g_unknown_prev)
    {
        if(found >= 50) break;
        float cur;
        if(!SafeReadFloat(prev.address, cur)) continue;

        bool match = false;
        if(strcmp(condition, "changed")   == 0) match = (cur != prev.fval);
        if(strcmp(condition, "unchanged") == 0) match = (cur == prev.fval);
        if(strcmp(condition, "increased") == 0) match = (cur > prev.fval);
        if(strcmp(condition, "decreased") == 0) match = (cur < prev.fval);

        if(match)
        {
            ScanResult sr;
            sr.address = prev.address;
            sr.fval    = cur;
            sr.ival    = *(int*)prev.address;
            sr.isFloat = true;
            g_results.push_back(sr);
            found++;
        }
        // update prev value untuk scan berikutnya
        prev.fval = cur;
        prev.ival = *(int*)prev.address;
    }

    LOGI("Unknown filter '%s' done, %d results", condition, found);
    pthread_mutex_unlock(&g_mutex);
}

// ── Write result ke file ──────────────────────────────
void WriteResults()
{
    FILE* f = fopen(RESULT_FILE, "w");
    if(!f) return;

    pthread_mutex_lock(&g_mutex);
    fprintf(f, "count=%d\n", (int)g_results.size());
    for(int i = 0; i < (int)g_results.size(); i++)
    {
        auto& r = g_results[i];
        if(r.isFloat)
            fprintf(f, "%d=0x%X,float,%.4f\n", i+1, r.address, r.fval);
        else
            fprintf(f, "%d=0x%X,int,%d\n", i+1, r.address, r.ival);
    }
    pthread_mutex_unlock(&g_mutex);
    fclose(f);
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
            if(lk.isFloat)
                *(float*)lk.address = lk.fval;
            else
                *(int*)lk.address = lk.ival;
        }
        pthread_mutex_unlock(&g_mutex);
        usleep(50000); // 50ms
    }
    return nullptr;
}

// ── Command parser ────────────────────────────────────
void ParseCommand(const char* cmd)
{
    LOGI("CMD: %s", cmd);

    // search <value> <float|int|both> <gtasa|samp|heap|all>
    if(strncmp(cmd, "search ", 7) == 0)
    {
        float fval = 0; int ival = 0;
        char typeStr[16] = "float";
        char rangeStr[16] = "gtasa";
        sscanf(cmd + 7, "%f %s %s", &fval, typeStr, rangeStr);
        ival = (int)fval;

        bool sf = strstr(typeStr, "float") || strstr(typeStr, "both");
        bool si = strstr(typeStr, "int")   || strstr(typeStr, "both");
        bool rg = strstr(rangeStr, "gtasa") || strstr(rangeStr, "all");
        bool rs = strstr(rangeStr, "samp")  || strstr(rangeStr, "all");
        bool rh = strstr(rangeStr, "heap")  || strstr(rangeStr, "all");

        DoSearch(fval, ival, sf, si, rg, rs, rh);
        WriteResults();
    }
    // unknown_first <range>
    else if(strncmp(cmd, "unknown_first", 13) == 0)
    {
        char rangeStr[16] = "gtasa";
        sscanf(cmd + 13, " %s", rangeStr);
        bool rg = strstr(rangeStr, "gtasa") || strstr(rangeStr, "all");
        bool rs = strstr(rangeStr, "samp")  || strstr(rangeStr, "all");
        bool rh = strstr(rangeStr, "heap")  || strstr(rangeStr, "all");
        DoUnknownFirstScan(rg, rs, rh);

        FILE* f = fopen(RESULT_FILE, "w");
        if(f) { fprintf(f, "status=first_scan_done\n"); fclose(f); }
    }
    // unknown_filter <changed|unchanged|increased|decreased>
    else if(strncmp(cmd, "unknown_filter ", 15) == 0)
    {
        DoUnknownFilter(cmd + 15);
        WriteResults();
    }
    // set <index> <value>
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
    // lock <index> <value>
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
            LOGI("Locked [%d] 0x%X = %.2f", idx+1, r.address, val);
        }
        pthread_mutex_unlock(&g_mutex);
    }
    // unlock <index>
    else if(strncmp(cmd, "unlock ", 7) == 0)
    {
        int idx = 0;
        sscanf(cmd + 7, "%d", &idx);
        idx--;
        pthread_mutex_lock(&g_mutex);
        if(idx >= 0 && idx < (int)g_locks.size())
        {
            g_locks[idx].active = false;
            LOGI("Unlocked [%d]", idx+1);
        }
        pthread_mutex_unlock(&g_mutex);
    }
}

// ── Command watcher thread ────────────────────────────
void* CmdWatcher(void*)
{
    char lastCmd[256] = "";
    while(true)
    {
        FILE* f = fopen(CMD_FILE, "r");
        if(f)
        {
            char cmd[256] = "";
            fgets(cmd, sizeof(cmd), f);
            fclose(f);

            // trim newline
            int len = strlen(cmd);
            if(len > 0 && cmd[len-1] == '\n') cmd[len-1] = '\0';

            // hanya proses kalau command baru
            if(strlen(cmd) > 0 && strcmp(cmd, lastCmd) != 0)
            {
                strcpy(lastCmd, cmd);
                ParseCommand(cmd);
                // clear cmd file setelah diproses
                FILE* fc = fopen(CMD_FILE, "w");
                if(fc) fclose(fc);
            }
        }
        usleep(200000); // cek setiap 200ms
    }
    return nullptr;
}

// ── Entry point ───────────────────────────────────────
extern "C" __attribute__((visibility("default"))) ModInfo* __GetModInfo() { return modinfo; }

extern "C" __attribute__((visibility("default"))) void OnModLoad()
{
    aml = (IAML*)GetInterface("AMLInterface");
    if(!aml) return;

    LOGI("MemTool v1.0 loaded!");

    // start threads
    pthread_t t1, t2;
    pthread_create(&t1, nullptr, CmdWatcher, nullptr);
    pthread_create(&t2, nullptr, LockThread, nullptr);
    pthread_detach(t1);
    pthread_detach(t2);

    aml->ShowToast(true, "MemTool v1.0 ready!");
}
