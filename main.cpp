#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>

#define LOG_TAG "AntiPause"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static ModInfo g_modinfo("com.burhan.antipause", "AntiPause", "2.6", "Burhanudin");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;

extern "C" __attribute__((visibility("default"))) ModInfo* __GetModInfo() { return modinfo; }

extern "C" __attribute__((visibility("default"))) void OnModLoad()
{
    aml = (IAML*)GetInterface("AMLInterface");
    if(!aml) return;

    uintptr_t pGTASA = aml->GetLib("libGTASA.so");
    if(!pGTASA) return;

    LOGI("libGTASA base: 0x%X", pGTASA);

    // hanya NOP InnerPause, tidak ada hook sama sekali
    aml->PlaceNOP(pGTASA + 0x27c741, 4);
    LOGI("InnerPause NOP'd - no hooks");

    aml->ShowToast(true, "AntiPause v2.6 aktif!");
}
