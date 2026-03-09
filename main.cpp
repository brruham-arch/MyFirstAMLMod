#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>
#include <dlfcn.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>

#define LOG(fmt, ...) __android_log_print(ANDROID_LOG_INFO, "MyFirstMod", fmt, ##__VA_ARGS__)

static ModInfo g_modinfo("com.burhan.myfirstmod", "SyncBlocker", "1.0", "Burhan");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;

static bool  g_enabled  = false; // default OFF, toggle via /sb
static int   g_blocked  = 0;
static int   g_passed   = 0;

bool isSyncPacket(unsigned char id) {
    return id == 203 || id == 207 || id == 208 ||
           id == 209 || id == 210 || id == 211;
}

// ── recvfrom hook ──────────────────────────────────────────────
typedef ssize_t (*recvfrom_t)(int,void*,size_t,int,struct sockaddr*,socklen_t*);
recvfrom_t origRecvfrom = nullptr;

ssize_t HookedRecvfrom(int sockfd, void* buf, size_t len,
                       int flags, struct sockaddr* addr, socklen_t* addrlen)
{
    ssize_t result = origRecvfrom(sockfd, buf, len, flags, addr, addrlen);

    if(!g_enabled || result <= 4 || !buf) return result;

    unsigned char* data = (unsigned char*)buf;
    if(data[0] != 0x68 && data[0] != 0x6E) return result;

    for(int i = 4; i < (int)result - 1; i++)
    {
        if(isSyncPacket(data[i]))
        {
            g_blocked++;
            // Kembalikan 0 agar SA-MP anggap tidak ada data
            // TANPA modifikasi buffer asli
            return 0;
        }
    }
    g_passed++;
    return result;
}

// ── sendto hook (untuk intercept chat command /sb) ─────────────
typedef ssize_t (*sendto_t)(int,const void*,size_t,int,
                            const struct sockaddr*,socklen_t);
sendto_t origSendto = nullptr;

ssize_t HookedSendto(int sockfd, const void* buf, size_t len,
                     int flags, const struct sockaddr* addr, socklen_t addrlen)
{
    // Cek apakah ini chat message yang mengandung "/sb"
    if(buf && len > 2)
    {
        const char* data = (const char*)buf;
        // SA-MP chat command biasanya diawali dengan byte ID lalu string
        for(int i = 0; i < (int)len - 3 && i < 10; i++)
        {
            if(data[i] == '/' && data[i+1] == 's' && data[i+2] == 'b')
            {
                // Toggle SyncBlocker
                g_enabled = !g_enabled;
                g_blocked = 0;
                g_passed  = 0;

                LOG("SyncBlocker: %s", g_enabled ? "ON" : "OFF");
                aml->ShowToast(true, "SyncBlocker: %s", 
                    g_enabled ? "ON 🟢" : "OFF 🔴");

                // Jangan kirim command /sb ke server
                return len;
            }
        }
    }
    return origSendto(sockfd, buf, len, flags, addr, addrlen);
}

extern "C" __attribute__((visibility("default"))) ModInfo* __GetModInfo() { return modinfo; }

extern "C" __attribute__((visibility("default"))) void OnModLoad() {
    aml = (IAML*)GetInterface("AMLInterface");
    if(!aml) return;

    // Hook recvfrom
    void* fnRecv = dlsym(RTLD_DEFAULT, "recvfrom");
    if(fnRecv) {
        aml->Hook(fnRecv, (void*)HookedRecvfrom, (void**)&origRecvfrom);
        LOG("recvfrom hooked!");
    }

    // Hook sendto untuk intercept /sb command
    void* fnSend = dlsym(RTLD_DEFAULT, "sendto");
    if(fnSend) {
        aml->Hook(fnSend, (void*)HookedSendto, (void**)&origSendto);
        LOG("sendto hooked!");
    }

    aml->ShowToast(true, "SyncBlocker siap!\nKetik /sb untuk toggle");
    LOG("SyncBlocker ready! Ketik /sb untuk toggle");
}
