#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>
#include <dlfcn.h>
#include <sys/socket.h>
#include <string.h>

#define LOG(fmt, ...) __android_log_print(ANDROID_LOG_INFO, "MyFirstMod", fmt, ##__VA_ARGS__)

static ModInfo g_modinfo("com.burhan.myfirstmod", "SyncBlocker", "1.0", "Burhan");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;

// Statistik
static int g_blocked = 0;
static int g_passed  = 0;
static int g_toastTimer = 0;

// Packet IDs sync player lain
#define PACKET_PLAYER_SYNC      207  // 0xCF
#define PACKET_VEHICLE_SYNC     208  // 0xD0  
#define PACKET_PASSENGER_SYNC   209  // 0xD1
#define PACKET_AIM_SYNC         203  // 0xCB
#define PACKET_UNOCCUPIED_SYNC  210  // 0xD2
#define PACKET_TRAILER_SYNC     211  // 0xD3

typedef ssize_t (*recv_t)(int sockfd, void* buf, size_t len, int flags);
recv_t origRecv = nullptr;

ssize_t HookedRecv(int sockfd, void* buf, size_t len, int flags)
{
    ssize_t result = origRecv(sockfd, buf, len, flags);
    
    if(result > 0 && buf)
    {
        unsigned char* data = (unsigned char*)buf;
        unsigned char packetID = data[0];
        
        bool shouldBlock = (
            packetID == PACKET_PLAYER_SYNC     ||
            packetID == PACKET_VEHICLE_SYNC    ||
            packetID == PACKET_PASSENGER_SYNC  ||
            packetID == PACKET_AIM_SYNC        ||
            packetID == PACKET_UNOCCUPIED_SYNC ||
            packetID == PACKET_TRAILER_SYNC
        );
        
        if(shouldBlock)
        {
            g_blocked++;
            // Kosongkan buffer agar SA-MP tidak proses
            memset(buf, 0, result);
            
            // Toast setiap 100 packet di-block
            if(aml && g_blocked % 100 == 0)
            {
                aml->ShowToast(false, "SyncBlocker: %d blocked / %d passed", 
                    g_blocked, g_passed);
                LOG("SyncBlocker: blocked=%d passed=%d lastID=%d", 
                    g_blocked, g_passed, packetID);
            }
        }
        else
        {
            g_passed++;
        }
    }
    
    return result;
}

extern "C" __attribute__((visibility("default"))) ModInfo* __GetModInfo() {
    return modinfo;
}

extern "C" __attribute__((visibility("default"))) void OnModLoad() {
    aml = (IAML*)GetInterface("AMLInterface");
    if(!aml) return;
    
    // Dapat alamat recv() via dlsym - TANPA OFFSET!
    void* fnRecv = dlsym(RTLD_DEFAULT, "recv");
    LOG("recv() addr = %p", fnRecv);
    
    if(fnRecv)
    {
        aml->Hook(fnRecv, (void*)HookedRecv, (void**)&origRecv);
        LOG("SyncBlocker: Hook recv() terpasang!");
        aml->ShowToast(true, "SyncBlocker aktif!\nrecv() hooked!");
    }
    else
    {
        LOG("SyncBlocker: GAGAL - recv() tidak ditemukan!");
        aml->ShowToast(true, "SyncBlocker GAGAL!");
    }
}
