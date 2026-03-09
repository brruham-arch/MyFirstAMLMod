#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>

#define LOG(fmt, ...) __android_log_print(ANDROID_LOG_INFO, "MyFirstMod", fmt, ##__VA_ARGS__)

static ModInfo g_modinfo("com.burhan.myfirstmod", "MyFirstMod", "1.0", "Burhan");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;

// Sync packet IDs yang mau di-block
#define PACKET_PLAYER_SYNC       207
#define PACKET_VEHICLE_SYNC      208
#define PACKET_PASSENGER_SYNC    209
#define PACKET_SPECTATING_SYNC   210
#define PACKET_AIM_SYNC          203
#define PACKET_UNOCCUPIED_SYNC   209

// Tipe fungsi ProcessPacket
typedef bool (*ProcessPacket_t)(void* pSAMP, unsigned char packetID, void* pData, int nLen);
ProcessPacket_t origProcessPacket = nullptr;

// Hook function
bool HookedProcessPacket(void* pSAMP, unsigned char packetID, void* pData, int nLen)
{
    // Block sync dari player lain, pass semua yang lain
    if(packetID == PACKET_PLAYER_SYNC ||
       packetID == PACKET_VEHICLE_SYNC ||
       packetID == PACKET_PASSENGER_SYNC ||
       packetID == PACKET_SPECTATING_SYNC ||
       packetID == PACKET_AIM_SYNC)
    {
        return true; // drop packet, jangan proses
    }
    return origProcessPacket(pSAMP, packetID, pData, nLen);
}

extern "C" __attribute__((visibility("default"))) ModInfo* __GetModInfo() {
    return modinfo;
}

extern "C" __attribute__((visibility("default"))) void OnModLoad() {
    aml = (IAML*)GetInterface("AMLInterface");
    if(!aml) return;

    uintptr_t pSAMP = aml->GetLib("libsamp.so");
    LOG("libsamp.so = 0x%X", pSAMP);

    // Pattern scan ProcessPacket di libsamp.so
    // Pattern ini umum untuk SA-MP Android 0.3.7
    uintptr_t fnProcessPacket = aml->PatternScan(
        "?? ?? ?? ?? 00 00 00 00 CF 00 00 00",
        "libsamp.so"
    );
    LOG("ProcessPacket pattern = 0x%X", fnProcessPacket);

    // Coba juga scan di libSAMP_ORIG.so
    uintptr_t fnProcessPacketORIG = aml->PatternScan(
        "?? ?? ?? ?? 00 00 00 00 CF 00 00 00",
        "libSAMP_ORIG.so"
    );
    LOG("ProcessPacket ORIG pattern = 0x%X", fnProcessPacketORIG);
}
