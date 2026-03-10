#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>

#define LOG_TAG "AntiPause"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static ModInfo g_modinfo("com.burhan.antipause", "AntiPause", "1.6", "Burhanudin");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;
uintptr_t pGTASA = 0;

void (*origNvOnPause)(void*, void*) = nullptr;

void HookedNvOnPause(void* env, void* thiz)
{
    LOGI("NvOnPause intercepted - calling original then resetting pause state");

    // biarkan engine handle lifecycle (EGL, audio, dll)
    origNvOnPause(env, thiz);

    // langsung reset IsAndroidPaused = 0 supaya menu pause tidak muncul
    if(pGTASA)
    {
        *(int*)(pGTASA + 0x6855bc) = 0; // IsAndroidPaused = false
        *(int*)(pGTASA + 0x6d7048) = 0; // WasAndroidPaused = false
        LOGI("IsAndroidPaused reset to 0");
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
    aml->ShowToast(true, "AntiPause v1.6 aktif!");
}
