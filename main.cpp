#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>

#define LOG_TAG "AntiPause"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static ModInfo g_modinfo("com.burhan.antipause", "AntiPause", "3.6", "Burhanudin");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;

void (*origAFKHandler)() = nullptr;

void HookedAFKHandler()
{
    LOGI("AFK handler blocked!");
}

extern "C" __attribute__((visibility("default"))) ModInfo* __GetModInfo() { return modinfo; }

extern "C" __attribute__((visibility("default"))) void OnModLoad()
{
    aml = (IAML*)GetInterface("AMLInterface");
    if(!aml) return;

    uintptr_t pSAMP = aml->GetLib("libsamp.so");
    if(!pSAMP) return;

    LOGI("libsamp base: 0x%X", pSAMP);

    aml->Hook(
        (void*)(pSAMP + 0x1514dc),
        (void*)HookedAFKHandler,
        (void**)&origAFKHandler
    );

    LOGI("origAFKHandler: %p", (void*)origAFKHandler);
    aml->ShowToast(true, "AntiPause v3.6 aktif!");
}
