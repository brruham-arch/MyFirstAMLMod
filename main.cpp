#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>

#define LOG_TAG "AntiPause"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static ModInfo g_modinfo("com.burhan.antipause", "AntiPause", "3.5", "Burhanudin");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;

void (*origStopThread)() = nullptr;

void HookedStopThread()
{
    LOGI("StopThread blocked!");
    // jangan stop thread SA-MP saat pause
}

extern "C" __attribute__((visibility("default"))) ModInfo* __GetModInfo() { return modinfo; }

extern "C" __attribute__((visibility("default"))) void OnModLoad()
{
    aml = (IAML*)GetInterface("AMLInterface");
    if(!aml) return;

    uintptr_t pSAMP = aml->GetLib("libsamp.so");
    if(!pSAMP) { LOGI("libsamp not found"); return; }

    LOGI("libsamp base: 0x%X", pSAMP);

    aml->Hook(
        (void*)(pSAMP + 0x1e252c),
        (void*)HookedStopThread,
        (void**)&origStopThread
    );

    LOGI("origStopThread: %p", (void*)origStopThread);
    aml->ShowToast(true, "AntiPause v3.5 aktif!");
}
