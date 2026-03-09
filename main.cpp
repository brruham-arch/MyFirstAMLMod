#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>

#define LOG(fmt, ...) __android_log_print(ANDROID_LOG_INFO, "MyFirstMod", fmt, ##__VA_ARGS__)

static ModInfo g_modinfo("com.burhan.myfirstmod", "AntiPause", "1.0", "Burhan");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;

typedef void (*AndroidPaused_t)();
AndroidPaused_t origAndroidPaused = nullptr;
void HookedAndroidPaused() {
    LOG("BLOCK: AndroidPaused()");
}

typedef void (*SetAndroidPaused_t)(int);
SetAndroidPaused_t origSetAndroidPaused = nullptr;
void HookedSetAndroidPaused(int paused) {
    LOG("SetAndroidPaused(%d)", paused);
    if(paused) return;
    origSetAndroidPaused(paused);
}

typedef void (*AndroidPause_t)();
AndroidPause_t origAndroidPause = nullptr;
void HookedAndroidPause() {
    LOG("BLOCK: AndroidPause()");
}

extern "C" __attribute__((visibility("default"))) ModInfo* __GetModInfo() { return modinfo; }

extern "C" __attribute__((visibility("default"))) void OnModLoad() {
    aml = (IAML*)GetInterface("AMLInterface");
    if(!aml) return;

    uintptr_t pGTASA = aml->GetLib("libGTASA.so");
    LOG("libGTASA = 0x%X", pGTASA);

    aml->Hook((void*)(pGTASA + 0x269ad4), (void*)HookedAndroidPaused,
              (void**)&origAndroidPaused);
    LOG("AndroidPaused hooked!");

    aml->Hook((void*)(pGTASA + 0x269ae4), (void*)HookedSetAndroidPaused,
              (void**)&origSetAndroidPaused);
    LOG("SetAndroidPaused hooked!");

    aml->Hook((void*)(pGTASA + 0x269af4), (void*)HookedAndroidPause,
              (void**)&origAndroidPause);
    LOG("AndroidPause hooked!");

    aml->ShowToast(true, "AntiPause aktif!");
}
