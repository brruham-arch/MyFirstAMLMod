#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>

#define LOG(fmt, ...) __android_log_print(ANDROID_LOG_INFO, "MyFirstMod", fmt, ##__VA_ARGS__)

static ModInfo g_modinfo("com.burhan.myfirstmod", "MyFirstMod", "1.0", "Burhan");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;

extern "C" __attribute__((visibility("default"))) ModInfo* __GetModInfo() {
    return modinfo;
}

extern "C" __attribute__((visibility("default"))) void OnModLoad() {
    aml = (IAML*)GetInterface("AMLInterface");
    if(!aml) return;

    // Cari base address libsamp.so dan libSAMP_ORIG.so
    uintptr_t pSAMP = aml->GetLib("libsamp.so");
    uintptr_t pSAMPORIG = aml->GetLib("libSAMP_ORIG.so");
    uintptr_t pGTASA = aml->GetLib("libGTASA.so");

    LOG("libGTASA.so    = 0x%X", pGTASA);
    LOG("libsamp.so     = 0x%X", pSAMP);
    LOG("libSAMP_ORIG.so= 0x%X", pSAMPORIG);
}
