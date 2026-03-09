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
            LOG("%s+0x%03X: %s", label, i-15, buf);
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

    // Coba berbagai pattern untuk ReceivePacket / ProcessPacket
    struct { const char* name; const char* pattern; } pats[] = {
        {"F0B5_83B0",    "F0 B5 83 B0"},
        {"F0B5_85B0",    "F0 B5 85 B0"},
        {"F0B5_86B0",    "F0 B5 86 B0"},
        {"F0B5_87B0",    "F0 B5 87 B0"},
        {"2DE9_F04F",    "2D E9 F0 4F"},
        {"2DE9_F84F",    "2D E9 F8 4F"},
        {"70B5_05AF",    "70 B5 05 AF"},
        {"F8B5_05AF",    "F8 B5 05 AF"},
        {nullptr, nullptr}
    };

    for(int i = 0; pats[i].name; i++) {
        uintptr_t r = aml->PatternScan(pats[i].pattern, "libsamp.so");
        if(r) LOG("%-15s offset=0x%X", pats[i].name, r - pSAMP);
    }
}
