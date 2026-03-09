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

    // Coba semua nama interface yang mungkin ada
    const char* names[] = {
        "AMLInterface",
        "AXLInterface", 
        "SAMPInterface",
        "MoNetInterface",
        "LuaInterface",
        "RakNetInterface",
        "ChatInterface",
        "PlayerInterface",
        "VehicleInterface",
        "SAMP",
        "AXL",
        "RakNet",
        "Chat",
        nullptr
    };

    for(int i = 0; names[i]; i++) {
        void* iface = GetInterface(names[i]);
        LOG("GetInterface(\"%s\") = %p", names[i], iface);
    }
}
