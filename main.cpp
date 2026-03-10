#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>

#define LOG_TAG "AntiPause"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static ModInfo g_modinfo("com.burhan.antipause", "AntiPause", "2.3", "Burhanudin");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;

void (*origNvOnPause)(void*, void*)   = nullptr;
void (*origInnerPause)(void*, void*)  = nullptr;

void HookedNvOnPause(void* env, void* thiz)
{
    LOGI("NvOnPause - pass through (EGL handled)");
    // tidak block, biarkan EGL lifecycle jalan normal
    if(origNvOnPause) origNvOnPause(env, thiz);
}

void HookedInnerPause(void* a, void* b)
{
    LOGI("InnerPause blocked!");
    // block fungsi dalam yang trigger pause menu
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
        (void*)(pGTASA + 0x274001),
        (void*)HookedNvOnPause,
        (void**)&origNvOnPause
    );
    LOGI("origNvOnPause: %p", (void*)origNvOnPause);

    aml->Hook(
        (void*)(pGTASA + 0x27c741),
        (void*)HookedInnerPause,
        (void**)&origInnerPause
    );
    LOGI("origInnerPause: %p", (void*)origInnerPause);

    aml->ShowToast(true, "AntiPause v2.3 aktif!");
}
