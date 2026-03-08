#include <jni.h>
#include <android/log.h>
#include <stdio.h>

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

extern "C" __attribute__((visibility("default"))) void OnModPreLoad() {
    __android_log_print(ANDROID_LOG_ERROR, "MYFIRSTMOD", "=== PRELOAD ===");
}

extern "C" __attribute__((visibility("default"))) void OnModLoad() {
    __android_log_print(ANDROID_LOG_ERROR, "MYFIRSTMOD", "=== LOAD ===");
}

__attribute__((constructor))
static void onDlOpen() {
    __android_log_print(ANDROID_LOG_ERROR, "MYFIRSTMOD", "=== CONSTRUCTOR ===");
}
