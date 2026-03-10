#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>

#define LOG_TAG "AntiPause"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static ModInfo g_modinfo("com.burhan.antipause", "AntiPause", "3.1", "Burhanudin");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;

void (*origSetAndroidPaused)(int) = nullptr;

void HookedSetAndroidPaused(int paused)
{
    LOGI("SetAndroidPaused(%d) blocked!", paused);
    // tidak panggil original = game tidak pause
}

extern "C" __attribute__((visibility("default"))) ModInfo* __GetModInfo() { return modinfo; }

extern "C" __attribute__((visibility("default"))) void OnModLoad()
{
    aml = (IAML*)GetInterface("AMLInterface");
    if(!aml) return;

    uintptr_t pGTASA = aml->GetLib("libGTASA.so");
    if(!pGTASA) return;

    LOGI("libGTASA base: 0x%X", pGTASA);

    aml->Hook(
        (void*)(pGTASA + 0x269af4),
        (void*)HookedSetAndroidPaused,
        (void**)&origSetAndroidPaused
    );

    LOGI("Hook SetAndroidPaused OK");
    aml->ShowToast(true, "AntiPause v3.1 aktif!");
}
