#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>

#define LOG_TAG "AntiPause"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static ModInfo g_modinfo("com.burhan.antipause", "AntiPause", "1.5b", "Burhanudin");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;
uintptr_t pGTASA = 0;

void (*origNvOnPause)(void*, void*) = nullptr;

void HookedNvOnPause(void* env, void* thiz)
{
    LOGI("NvOnPause blocked - game runs in background");

    // set paused = 1 supaya saat resume, game restore EGL + tampil pause menu
    if(pGTASA)
    {
        *(int*)(pGTASA + 0x6855bc) = 1; // IsAndroidPaused = true
        LOGI("IsAndroidPaused set to 1 for proper resume");
    }
}

extern "C" __attribute__((visibility("default"))) ModInfo* __GetModInfo() { return modinfo; }

extern "C" __attribute__((visibility("default"))) void OnModLoad()
{
    aml = (IAML*)GetInterface("AMLInterface");
    if(!aml) return;

    pGTASA = aml->GetLib("libGTASA.so");
    if(!pGTASA) return;

    LOGI("libGTASA base: 0x%X", pGTASA);

    aml->Hook(
        (void*)(pGTASA + 0x274001),
        (void*)HookedNvOnPause,
        (void**)&origNvOnPause
    );

    LOGI("Hook OK");
    aml->ShowToast(true, "AntiPause v1.5b aktif!");
}
