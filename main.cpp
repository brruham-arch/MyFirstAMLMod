#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>

#define LOG_TAG "AntiPause"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static ModInfo g_modinfo("com.burhan.antipause", "AntiPause", "1.9", "Burhanudin");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;
uintptr_t pGTASA = 0;

void (*origNvOnPause)(void*, void*) = nullptr;

void HookedNvOnPause(void* env, void* thiz)
{
    LOGI("onPause intercepted");

    if(origNvOnPause)
        origNvOnPause(env, thiz);

    LOGI("origNvOnPause ptr after call: %p", (void*)origNvOnPause);

    // hanya reset IsAndroidPaused — jangan sentuh yang lain
    if(pGTASA)
    {
        *(int*)(pGTASA + 0x6855bc) = 0;
        LOGI("IsAndroidPaused = 0");
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

    LOGI("origNvOnPause: %p", (void*)origNvOnPause);
    aml->ShowToast(true, "AntiPause v1.9 aktif!");
}
