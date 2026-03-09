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

static int g_blocked = 0;
static int g_passed  = 0;

#define PACKET_PLAYER_SYNC      207
#define PACKET_VEHICLE_SYNC     208
#define PACKET_PASSENGER_SYNC   209
#define PACKET_AIM_SYNC         203
#define PACKET_UNOCCUPIED_SYNC  210
#define PACKET_TRAILER_SYNC     211

bool isSyncPacket(unsigned char id) {
    return id == PACKET_PLAYER_SYNC    ||
           id == PACKET_VEHICLE_SYNC   ||
           id == PACKET_PASSENGER_SYNC ||
           id == PACKET_AIM_SYNC       ||
           id == PACKET_UNOCCUPIED_SYNC||
           id == PACKET_TRAILER_SYNC;
}

// Hook recvfrom
typedef ssize_t (*recvfrom_t)(int, void*, size_t, int, struct sockaddr*, socklen_t*);
recvfrom_t origRecvfrom = nullptr;

ssize_t HookedRecvfrom(int sockfd, void* buf, size_t len, int flags, struct sockaddr* addr, socklen_t* addrlen)
{
    ssize_t result = origRecvfrom(sockfd, buf, len, flags, addr, addrlen);
    if(result > 1 && buf)
    {
        unsigned char* data = (unsigned char*)buf;
        // SA-MP paket biasanya byte pertama = ID
        // Tapi RakNet punya header 1-2 byte sebelum packet ID
        // Coba offset 0 dan 1
        unsigned char id0 = data[0];
        unsigned char id1 = (result > 1) ? data[1] : 0;
        
        LOG("recvfrom: id0=0x%02X id1=0x%02X len=%d", id0, id1, (int)result);
        
        if(isSyncPacket(id0) || isSyncPacket(id1))
        {
            g_blocked++;
            memset(buf, 0, result);
            if(g_blocked % 50 == 0)
            {
                aml->ShowToast(false, "Blocked:%d Passed:%d", g_blocked, g_passed);
            }
            return result;
        }
        g_passed++;
    }
    return result;
}

extern "C" __attribute__((visibility("default"))) ModInfo* __GetModInfo() { return modinfo; }

extern "C" __attribute__((visibility("default"))) void OnModLoad() {
    aml = (IAML*)GetInterface("AMLInterface");
    if(!aml) return;

    void* fnRecvfrom = dlsym(RTLD_DEFAULT, "recvfrom");
    LOG("recvfrom() = %p", fnRecvfrom);

    if(fnRecvfrom) {
        aml->Hook(fnRecvfrom, (void*)HookedRecvfrom, (void**)&origRecvfrom);
        LOG("recvfrom hooked!");
        aml->ShowToast(true, "SyncBlocker v2!\nrecvfrom() hooked!");
    }
}
