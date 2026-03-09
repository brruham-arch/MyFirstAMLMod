#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>

#define LOG(fmt, ...) __android_log_print(ANDROID_LOG_INFO, "MyFirstMod", fmt, ##__VA_ARGS__)

static ModInfo g_modinfo("com.burhan.myfirstmod", "AntiPause", "1.0", "Burhan");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;

// _Z13AndroidPausedv  = 0x269ad4 → void AndroidPaused()
typedef void (*AndroidPaused_t)();
AndroidPaused_t origAndroidPaused = nullptr;
void HookedAndroidPaused() {
    LOG("BLOCK: AndroidPaused()");
}

// _Z16SetAndroidPausedi = 0x269ae4 → void SetAndroidPaused(int)
typedef void (*SetAndroidPaused_t)(int);
SetAndroidPaused_t origSetAndroidPaused = nullptr;
void HookedSetAndroidPaused(int paused) {
    if(paused) { LOG("BLOCK: SetAndroidPaused(%d)", paused); return; }
    origSetAndroidPaused(paused);
}

// _Z12AndroidPausev = 0x269af4 → void AndroidPause()
typedef void (*AndroidPause_t)();
AndroidPause_t origAndroidPause = nullptr;
void HookedAndroidPause() {
    LOG("BLOCK: AndroidPause()");
}

// IsAndroidPaused (data symbol 0x6855bc) → selalu return 0
// Kita patch langsung via PlaceNOP atau tulis 0 ke alamatnya

extern "C" __attribute__((visibility("default"))) ModInfo* __GetModInfo() { return modinfo; }

extern "C" __attribute__((visibility("default"))) void OnModLoad() {
    aml = (IAML*)GetInterface("AMLInterface");
    if(!aml) return;

    uintptr_t pGTASA = aml->GetLib("libGTASA.so");
    LOG("libGTASA = 0x%X", pGTASA);

    // Hook semua fungsi pause
    aml->Hook((void*)(pGTASA + 0x269ad4), (void*)HookedAndroidPaused,
              (void**)&origAndroidPaused);
    LOG("AndroidPaused hooked!");

    aml->Hook((void*)(pGTASA + 0x269ae4), (void*)HookedSetAndroidPaused,
              (void**)&origSetAndroidPaused);
    LOG("SetAndroidPaused hooked!");

    aml->Hook((void*)(pGTASA + 0x269af4), (void*)HookedAndroidPause,
              (void**)&origAndroidPause);
    LOG("AndroidPause hooked!");

    // Patch IsAndroidPaused variable = 0 (selalu tidak pause)
    uintptr_t pIsPaused = pGTASA + 0x6855bc;
    int zero = 0;
    aml->Write(pIsPaused, (uintptr_t)&zero, sizeof(int));
    LOG("IsAndroidPaused patched!");

    // Patch WasAndroidPaused juga
    uintptr_t pWasPaused = pGTASA + 0x6d7048;
    aml->Write(pWasPaused, (uintptr_t)&zero, sizeof(int));
    LOG("WasAndroidPaused patched!");

    aml->ShowToast(true, "AntiPause aktif!");
}
