#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>

#define LOG_TAG "AntiPause"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static ModInfo g_modinfo("com.burhan.antipause", "AntiPause", "3.2", "Burhanudin");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;
uintptr_t pGTASA = 0;

void (*origNvOnPause)()          = nullptr;
void (*origEGLMakeCurrent)()     = nullptr;

void HookedNvOnPause()
{
    // block pause loop seperti v1.5
}

void HookedEGLMakeCurrent()
{
    LOGI("EGLMakeCurrent called - EGL restored!");
    if(origEGLMakeCurrent) origEGLMakeCurrent();

    // reset pause state setelah EGL restored
    if(pGTASA)
    {
        *(int*)(pGTASA + 0x6855bc) = 0; // IsAndroidPaused
        *(int*)(pGTASA + 0x6d7048) = 0; // WasAndroidPaused
        LOGI("Pause state cleared after EGL restore!");
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

    // block pause loop (v1.5)
    aml->Hook((void*)(pGTASA + 0x274001),
              (void*)HookedNvOnPause,
              (void**)&origNvOnPause);

    // hook EGLMakeCurrent untuk deteksi resume
    aml->Hook((void*)(pGTASA + 0x268dd0),
              (void*)HookedEGLMakeCurrent,
              (void**)&origEGLMakeCurrent);

    LOGI("Hooks installed");
    aml->ShowToast(true, "AntiPause v3.2 aktif!");
}
