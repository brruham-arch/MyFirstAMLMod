#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>
#include <string.h>

#define LOG(fmt, ...) __android_log_print(ANDROID_LOG_INFO, "MyFirstMod", fmt, ##__VA_ARGS__)

static ModInfo g_modinfo("com.burhan.myfirstmod", "MyFirstMod", "1.0", "Burhan");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;

void dumpBytes(const char* label, uintptr_t addr, int count) {
    char buf[256] = {0};
    char tmp[8];
    for(int i = 0; i < count; i++) {
        snprintf(tmp, sizeof(tmp), "%02X ", *(unsigned char*)(addr + i));
        strcat(buf, tmp);
        if((i+1) % 16 == 0) {
            LOG("%s+0x%X: %s", label, i-15, buf);
            buf[0] = 0;
        }
    }
    if(buf[0]) LOG("%s: %s", label, buf);
}

extern "C" __attribute__((visibility("default"))) ModInfo* __GetModInfo() {
    return modinfo;
}

extern "C" __attribute__((visibility("default"))) void OnModLoad() {
    aml = (IAML*)GetInterface("AMLInterface");
    if(!aml) return;

    uintptr_t pSAMP = aml->GetLib("libsamp.so");
    uintptr_t pORIG = aml->GetLib("libSAMP_ORIG.so");

    // Dump sekitar offset yang ketemu
    uintptr_t addr1 = pSAMP + 0xDD0D0;
    uintptr_t addr2 = pORIG + 0x19092E;

    LOG("=== libsamp.so+0xDD0D0 ===");
    dumpBytes("samp", addr1 - 32, 64); // dump sebelum dan sesudah

    LOG("=== libSAMP_ORIG.so+0x19092E ===");
    dumpBytes("orig", addr2 - 32, 64);
}
