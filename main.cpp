#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>

#define LOG_TAG "AntiPause"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static ModInfo g_modinfo("com.burhan.antipause", "AntiPause", "1.3", "Burhanudin");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;

void (*origPauseOpenSLES)()      = nullptr;
void (*origMenuPauseGame)(bool)  = nullptr;
void (*origSetAndroidPaused)(int)= nullptr;
void (*origAndroidPaused)()      = nullptr;

void HookedPauseOpenSLES()      { LOGI("PauseOpenSLES() called!"); }
void HookedMenuPauseGame(bool p) { LOGI("Menu_PauseGame(%d) called!", p); }
void HookedSetAndroidPaused(int p){ LOGI("SetAndroidPaused(%d) called!", p); }
void HookedAndroidPaused()       { LOGI("AndroidPaused() called!"); }

extern "C" __attribute__((visibility("default"))) ModInfo* __GetModInfo() { return modinfo; }

extern "C" __attribute__((visibility("default"))) void OnModLoad()
{
    aml = (IAML*)GetInterface("AMLInterface");
    if(!aml) return;

    uintptr_t pGTASA = aml->GetLib("libGTASA.so");
    if(!pGTASA) return;

    aml->Hook((void*)(pGTASA + 0x248190), (void*)HookedPauseOpenSLES,      (void**)&origPauseOpenSLES);
    aml->Hook((void*)(pGTASA + 0x2a93e0), (void*)HookedMenuPauseGame,      (void**)&origMenuPauseGame);
    aml->Hook((void*)(pGTASA + 0x269af4), (void*)HookedSetAndroidPaused,   (void**)&origSetAndroidPaused);
    aml->Hook((void*)(pGTASA + 0x269ae4), (void*)HookedAndroidPaused,      (void**)&origAndroidPaused);

    LOGI("All hooks installed — waiting for pause event...");
    aml->ShowToast(true, "AntiPause v1.3 - detective mode");
}
