#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>

#define LOG_TAG "AntiPause"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static ModInfo g_modinfo("com.burhan.antipause", "AntiPause", "1.8", "Burhanudin");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;
uintptr_t pGTASA = 0;

void (*origNvOnPause)(void*, void*) = nullptr;

void HookedNvOnPause(void* env, void* thiz)
{
    LOGI("onPause intercepted");

    // panggil original dengan alamat genap = EGL cleanup berjalan normal
    if(origNvOnPause)
        origNvOnPause(env, thiz);

    // reset pause variable setelah original selesai
    if(pGTASA)
    {
        *(int*)(pGTASA + 0x6855bc) = 0; // IsAndroidPaused
        *(int*)(pGTASA + 0x6d7048) = 0; // WasAndroidPaused
        *(int*)(pGTASA + 0x6d7058) = 1; // AndroidResume = true
        LOGI("Pause vars reset OK");
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

    // 0x274000 (genap) bukan 0x274001 — supaya orig tersimpan benar
    aml->Hook(
        (void*)(pGTASA + 0x274000),
        (void*)HookedNvOnPause,
        (void**)&origNvOnPause
    );

    LOGI("origNvOnPause ptr: %p", (void*)origNvOnPause);
    LOGI("Hook onPause OK");
    aml->ShowToast(true, "AntiPause v1.8 aktif!");
}
