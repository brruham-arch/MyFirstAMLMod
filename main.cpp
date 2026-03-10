#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>

#define LOG_TAG "AntiPause"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static ModInfo g_modinfo("com.burhan.antipause", "AntiPause", "2.5", "Burhanudin");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;

void (*origNvOnPause)(void*, void*) = nullptr;

void HookedNvOnPause(void* env, void* thiz)
{
    LOGI("NvOnPause pass through");
    if(origNvOnPause) origNvOnPause(env, thiz);
}

extern "C" __attribute__((visibility("default"))) ModInfo* __GetModInfo() { return modinfo; }

extern "C" __attribute__((visibility("default"))) void OnModLoad()
{
    aml = (IAML*)GetInterface("AMLInterface");
    if(!aml) return;

    uintptr_t pGTASA = aml->GetLib("libGTASA.so");
    if(!pGTASA) return;

    LOGI("libGTASA base: 0x%X", pGTASA);

    // hook onPause untuk pass through
    aml->Hook(
        (void*)(pGTASA + 0x274001),
        (void*)HookedNvOnPause,
        (void**)&origNvOnPause
    );
    LOGI("origNvOnPause: %p", (void*)origNvOnPause);

    // NOP InnerPause — tidak perlu hook, langsung patch
    aml->PlaceNOP(pGTASA + 0x27c741, 4);
    LOGI("InnerPause NOP'd");

    aml->ShowToast(true, "AntiPause v2.5 aktif!");
}
