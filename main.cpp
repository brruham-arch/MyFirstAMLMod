#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>

#define LOG(fmt, ...) __android_log_print(ANDROID_LOG_INFO, "MyFirstMod", fmt, ##__VA_ARGS__)

static ModInfo g_modinfo("com.burhan.myfirstmod", "AntiPause", "1.0", "Burhan");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;

typedef void (*SetAndroidPaused_t)(int paused);
SetAndroidPaused_t origSetAndroidPaused = nullptr;

void HookedSetAndroidPaused(int paused)
{
    if(paused) {
        LOG("AntiPause: block SetAndroidPaused!");
        return;
    }
    origSetAndroidPaused(paused);
}

typedef void (*AndroidPause_t)();
AndroidPause_t origAndroidPause = nullptr;

void HookedAndroidPause()
{
    LOG("AntiPause: block AndroidPause!");
}

extern "C" __attribute__((visibility("default"))) ModInfo* __GetModInfo() { return modinfo; }

extern "C" __attribute__((visibility("default"))) void OnModLoad() {
    aml = (IAML*)GetInterface("AMLInterface");
    if(!aml) return;

    uintptr_t pGTASA = aml->GetLib("libGTASA.so");
    LOG("libGTASA = 0x%X", pGTASA);

    uintptr_t fnSetPaused = pGTASA + 0x00269ae4;
    uintptr_t fnAndPause  = pGTASA + 0x00269af4;

    aml->Hook((void*)fnSetPaused, (void*)HookedSetAndroidPaused,
              (void**)&origSetAndroidPaused);
    LOG("SetAndroidPaused hooked!");

    aml->Hook((void*)fnAndPause, (void*)HookedAndroidPause,
              (void**)&origAndroidPause);
    LOG("AndroidPause hooked!");

    aml->ShowToast(true, "AntiPause aktif!");
}
