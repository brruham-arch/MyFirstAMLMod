#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>
#include <cstdint>

#define LOG_TAG "BurhanCore"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static ModInfo g_modinfo("com.burhan.coremod", "BurhanCoreMod", "1.0", "Burhanudin");
ModInfo* modinfo = &g_modinfo;

IAML* aml = nullptr;

uintptr_t libGTASA = 0;
uintptr_t libSAMP = 0;

void DetectLibraries()
{
    libGTASA = (uintptr_t)aml->GetLib("libGTASA.so");
    libSAMP  = (uintptr_t)aml->GetLib("libsamp.so");

    if(libGTASA)
        LOGI("libGTASA base: %p", (void*)libGTASA);
    else
        LOGE("libGTASA not found");

    if(libSAMP)
        LOGI("libsamp base: %p", (void*)libSAMP);
}

void InitMod()
{
    LOGI("InitMod start");

    DetectLibraries();

    if(libGTASA)
        aml->ShowToast(true, "GTASA detected");

    LOGI("InitMod complete");
}

extern "C"
__attribute__((visibility("default")))
ModInfo* __GetModInfo()
{
    return modinfo;
}

extern "C"
__attribute__((visibility("default")))
void OnModLoad()
{
    LOGI("=== BurhanCoreMod loaded ===");

    aml = (IAML*)GetInterface("AMLInterface");

    if(!aml)
    {
        LOGE("AML interface not found");
        return;
    }

    LOGI("AML interface acquired");

    aml->ShowToast(true, "BurhanCoreMod aktif");

    InitMod();
}
