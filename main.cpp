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

    uintptr_t pSAMP = aml->GetLib("libsamp.so");
    uintptr_t pORIG = aml->GetLib("libSAMP_ORIG.so");

    // Scan pattern yang lebih spesifik untuk ProcessPacket SA-MP
    // Coba beberapa pattern umum
    const char* patterns[] = {
        "CF 00 00 00 D0 00 00 00",
        "2D E9 ?? ?? 83 B0",
        "10 B5 ?? ?? ?? ?? ?? ?? 08 B1",
        "2D E9 F0 4F ?? ?? ?? ??",
        nullptr
    };

    for(int i = 0; patterns[i]; i++) {
        uintptr_t res1 = aml->PatternScan(patterns[i], "libsamp.so");
        uintptr_t res2 = aml->PatternScan(patterns[i], "libSAMP_ORIG.so");
        LOG("Pattern[%d] samp=0x%X orig=0x%X", i, 
            res1 ? res1 - pSAMP : 0,
            res2 ? res2 - pORIG : 0);
    }
}
