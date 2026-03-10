#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>

#define LOG_TAG "AntiPause"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static ModInfo g_modinfo("com.burhan.antipause", "AntiPause", "1.0", "Burhanudin");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;

void (*origSetAndroidPaused)(int) = nullptr;

void HookedSetAndroidPaused(int paused)
{
    LOGI("SetAndroidPaused called: %d — ignored", paused);
}

extern "C" __attribute__((visibility("default"))) ModInfo* __GetModInfo() { return modinfo; }

extern "C" __attribute__((visibility("default"))) void OnModLoad()
{
    aml = (IAML*)GetInterface("AMLInterface");
    if(!aml) return;

    uintptr_t pGTASA = aml->GetLib("libGTASA.so");
    if(!pGTASA) { LOGI("libGTASA not found"); return; }

    LOGI("libGTASA base: 0x%X", pGTASA);

    aml->Hook(
        (void*)(pGTASA + 0x269af4),
        (void*)HookedSetAndroidPaused,
        (void**)&origSetAndroidPaused
    );

    LOGI("AntiPause hook installed!");
    aml->ShowToast(true, "AntiPause aktif!");
}
