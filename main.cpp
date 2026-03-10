#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>

#define LOG_TAG "AntiPause"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static ModInfo g_modinfo("com.burhan.antipause", "AntiPause", "1.7", "Burhanudin");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;
uintptr_t pGTASA = 0;

void (*origNvOnPause)(void*, void*)  = nullptr;
void (*origNvOnResume)(void*, void*) = nullptr;

void HookedNvOnPause(void* env, void* thiz)
{
    LOGI("onPause blocked");
    // tidak panggil original, tidak reset variable
    // biarkan game pikir tidak ada pause sama sekali
}

void HookedNvOnResume(void* env, void* thiz)
{
    LOGI("onResume - calling original");
    if(origNvOnResume)
        origNvOnResume(env, thiz);

    // reset pause state setelah resume
    if(pGTASA)
    {
        *(int*)(pGTASA + 0x6855bc) = 0;
        *(int*)(pGTASA + 0x6d7048) = 0;
        LOGI("Pause state cleared on resume");
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

    // hook onPause (offset ganjil = Thumb, tidak panggil original)
    aml->Hook(
        (void*)(pGTASA + 0x274001),
        (void*)HookedNvOnPause,
        (void**)&origNvOnPause
    );
    LOGI("Hook onPause OK");

    // hook onResume — offset dari crash log stack sebelumnya + 0x8
    // cari offset onResume dulu via nm
    aml->Hook(
        (void*)(pGTASA + 0x274041),
        (void*)HookedNvOnResume,
        (void**)&origNvOnResume
    );
    LOGI("Hook onResume OK");

    aml->ShowToast(true, "AntiPause v1.7 aktif!");
}
