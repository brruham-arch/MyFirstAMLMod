#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>

#define LOG(fmt, ...) __android_log_print(ANDROID_LOG_INFO, "MyFirstMod", fmt, ##__VA_ARGS__)

struct ModInfo {
    char szGUID[48];
    char szName[48];
    char szVersion[24];
    char szAuthor[48];
};

static ModInfo g_modinfo = {
    "com.burhan.myfirstmod",
    "MyFirstMod",
    "1.0",
    "Burhan"
};

IAML* aml = nullptr;

extern "C" __attribute__((visibility("default"))) ModInfo* __GetModInfo() {
    return &g_modinfo;
}

extern "C" __attribute__((visibility("default"))) void OnModLoad() {
    aml = (IAML*)GetInterface("AMLInterface");
    
    LOG("=== MyFirstMod v0.3 ===");
    LOG("aml = %p", aml);
    
    if(aml) {
        uintptr_t pGTASA = aml->GetLib("libGTASA.so");
        LOG("libGTASA.so base: 0x%X", pGTASA);
        aml->ShowToast(true, "MyFirstMod loaded!");
    }
}
