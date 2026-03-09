#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>

#define LOG(fmt, ...) __android_log_print(ANDROID_LOG_INFO, "MyFirstMod", fmt, ##__VA_ARGS__)

static ModInfo g_modinfo("com.burhan.myfirstmod", "MyFirstMod", "1.0", "Burhan");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;

typedef void (*Pause_t)(bool bPause);
Pause_t origPause = nullptr;

void HookedPause(bool bPause)
{
    if(bPause)
    {
        LOG("=== Game mau pause, DIBLOCK! ===");
        return; // skip pause
    }
    origPause(bPause);
}

extern "C" __attribute__((visibility("default"))) ModInfo* __GetModInfo() {
    return modinfo;
}

extern "C" __attribute__((visibility("default"))) void OnModLoad() {
    aml = (IAML*)GetInterface("AMLInterface");
    if(!aml) return;

    uintptr_t pGTASA = aml->GetLib("libGTASA.so");
    LOG("libGTASA = 0x%X", pGTASA);

    // CGame::Pause pattern di GTA SA Android
    uintptr_t fnPause = aml->PatternScan(
        "10 B5 ?? 4C ?? 4B 23 78 01 2B",
        "libGTASA.so"
    );
    LOG("CGame::Pause = 0x%X", fnPause ? fnPause - pGTASA : 0);

    if(fnPause)
    {
        aml->Hook((void*)fnPause, (void*)HookedPause, (void**)&origPause);
        LOG("Anti-pause aktif!");
        aml->ShowToast(true, "Anti-Pause aktif!");
    }
}
