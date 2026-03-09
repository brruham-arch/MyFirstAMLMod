#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>

#define LOG(fmt, ...) __android_log_print(ANDROID_LOG_INFO, "MyFirstMod", fmt, ##__VA_ARGS__)

static ModInfo g_modinfo("com.burhan.myfirstmod", "MyFirstMod", "1.0", "Burhan");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;

// Sama seperti onReceivePacket di Lua
typedef bool (*ReceivePacket_t)(void* rakpeer, unsigned char* data, int len, void* addr);
ReceivePacket_t origReceivePacket = nullptr;

bool HookedReceivePacket(void* rakpeer, unsigned char* data, int len, void* addr)
{
    if(data && len > 0)
    {
        unsigned char packetID = data[0];
        
        // Block sync packet dari player lain
        if(packetID == 207 || packetID == 208 || 
           packetID == 209 || packetID == 203)
        {
            return true; // drop
        }
    }
    return origReceivePacket(rakpeer, data, len, addr);
}

extern "C" __attribute__((visibility("default"))) ModInfo* __GetModInfo() {
    return modinfo;
}

extern "C" __attribute__((visibility("default"))) void OnModLoad() {
    aml = (IAML*)GetInterface("AMLInterface");
    if(!aml) return;

    // Hook RakNet::Receive di libsamp.so
    // Pattern: fungsi yang menerima packet dari network
    uintptr_t fnReceive = aml->PatternScan(
        "2D E9 F0 4F ?? ?? ?? ?? ?? ?? ?? ?? 04 46",
        "libsamp.so"
    );
    
    LOG("RakNet Receive = 0x%X", fnReceive);
    
    if(fnReceive)
    {
        aml->Hook(fnReceive, (void*)HookedReceivePacket, (void**)&origReceivePacket);
        LOG("Hook terpasang!");
        aml->ShowToast(true, "SyncBlocker aktif!");
    }
    else
    {
        LOG("Fungsi tidak ditemukan, perlu cari pattern lain");
    }
}
