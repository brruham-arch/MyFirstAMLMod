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

bool isSyncPacket(unsigned char id) {
    return id == 203 || id == 207 || id == 208 ||
           id == 209 || id == 210 || id == 211;
}

typedef ssize_t (*recvfrom_t)(int, void*, size_t, int, struct sockaddr*, socklen_t*);
recvfrom_t origRecvfrom = nullptr;

ssize_t HookedRecvfrom(int sockfd, void* buf, size_t len, int flags, struct sockaddr* addr, socklen_t* addrlen)
{
    ssize_t result = origRecvfrom(sockfd, buf, len, flags, addr, addrlen);
    if(result > 4 && buf)
    {
        unsigned char* data = (unsigned char*)buf;
        unsigned char rakID = data[0];

        if(rakID == 0x68 || rakID == 0x6E)
        {
            for(int i = 4; i < (int)result - 1; i++)
            {
                if(isSyncPacket(data[i]))
                {
                    // Ganti ID saja dengan 0xFF (unknown packet, SA-MP skip)
                    data[i] = 0xFF;
                    g_blocked++;

                    if(g_blocked % 50 == 0)
                    {
                        LOG("BLOCK sampID was %d offset=%d blocked=%d passed=%d",
                            data[i], i, g_blocked, g_passed);
                        aml->ShowToast(false, "Blocked:%d Passed:%d", g_blocked, g_passed);
                    }
                    break;
                }
            }
            g_passed++;
        }
    }
    return result;
}

extern "C" __attribute__((visibility("default"))) ModInfo* __GetModInfo() { return modinfo; }

extern "C" __attribute__((visibility("default"))) void OnModLoad() {
    aml = (IAML*)GetInterface("AMLInterface");
    if(!aml) return;

    void* fnRecvfrom = dlsym(RTLD_DEFAULT, "recvfrom");
    if(fnRecvfrom) {
        aml->Hook(fnRecvfrom, (void*)HookedRecvfrom, (void**)&origRecvfrom);
        LOG("SyncBlocker: hooked!");
        aml->ShowToast(true, "SyncBlocker aktif!");
    }
}
