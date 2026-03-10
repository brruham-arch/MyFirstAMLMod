#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>

#define LOG_TAG "AntiPause"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static ModInfo g_modinfo("com.burhan.antipause", "AntiPause", "1.2", "Burhanudin");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;

void (*origMenuPauseGame)(bool) = nullptr;

void HookedMenuPauseGame(bool pause)
{
    LOGI("Menu_PauseGame(%d) blocked", pause);
    // tidak panggil original = pause menu tidak muncul
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
        (void*)(pGTASA + 0x2a93e0),
        (void*)HookedMenuPauseGame,
        (void**)&origMenuPauseGame
    );
    LOGI("Hook Menu_PauseGame OK");

    aml->ShowToast(true, "AntiPause v1.2 aktif!");
}
