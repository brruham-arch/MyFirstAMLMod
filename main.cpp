#include <android/log.h>
#include <stdint.h>
#include <stdio.h>

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

extern "C" __attribute__((visibility("default"))) ModInfo* __GetModInfo() {
    return &g_modinfo;
}

extern "C" __attribute__((visibility("default"))) void OnModLoad() {
    LOG("=== MyFirstMod berhasil di-load! ===");
    LOG("aml interface tidak digunakan dulu");
}
