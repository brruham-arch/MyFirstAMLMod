#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>
#include <dlfcn.h>
#include <sys/socket.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

#define LOG(fmt, ...) __android_log_print(ANDROID_LOG_INFO, "MyFirstMod", fmt, ##__VA_ARGS__)
#define TOGGLE_FILE "/storage/emulated/0/Download/sb_toggle"

static ModInfo g_modinfo("com.burhan.myfirstmod", "SyncBlocker", "1.0", "Burhan");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;

static bool g_enabled    = false;
static int  g_blocked    = 0;
static int  g_toastPending = 0;

bool isSyncPacket(unsigned char id) {
    return id == 203 || id == 207 || id == 208 ||
           id == 209 || id == 210 || id == 211;
}

// Gunakan buffer lokal sendiri, bukan modifikasi buffer asli
static unsigned char g_fakeBuf[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

typedef ssize_t (*recvfrom_t)(int,void*,size_t,int,struct sockaddr*,socklen_t*);
recvfrom_t origRecvfrom = nullptr;

ssize_t HookedRecvfrom(int sockfd, void* buf, size_t len,
                       int flags, struct sockaddr* addr, socklen_t* addrlen)
{
    ssize_t result = origRecvfrom(sockfd, buf, len, flags, addr, addrlen);

    if(g_toastPending != 0 && aml) {
        aml->ShowToast(true, "SyncBlocker: %s",
            g_toastPending == 1 ? "ON" : "OFF");
        g_toastPending = 0;
    }

    if(!g_enabled || result <= 4 || !buf) return result;

    // Baca saja, JANGAN modifikasi buffer asli
    const unsigned char* data = (const unsigned char*)buf;
    if(data[0] != 0x68 && data[0] != 0x6E) return result;

    for(int i = 4; i < (int)result - 1; i++)
    {
        if(isSyncPacket(data[i]))
        {
            g_blocked++;
            if(g_blocked % 100 == 0) {
                aml->ShowToast(false, "[SB] Blocked:%d", g_blocked);
                LOG("blocked=%d", g_blocked);
            }
            // Kembalikan buffer fake 8 byte kosong
            // SA-MP terima "packet kosong" = tidak ada yang diproses
            if(len >= 8) {
                memcpy(buf, g_fakeBuf, 8);
                return 8;
            }
        }
    }
    return result;
}

void* toggleThread(void*)
{
    bool lastState = false;
    while(true)
    {
        sleep(2);
        FILE* f = fopen(TOGGLE_FILE, "r");
        bool fileExists = (f != nullptr);
        if(f) fclose(f);

        if(fileExists != lastState) {
            lastState      = fileExists;
            g_enabled      = fileExists;
            g_blocked      = 0;
            g_toastPending = fileExists ? 1 : 2;
            LOG("SyncBlocker: %s", g_enabled ? "ON" : "OFF");
        }
    }
    return nullptr;
}

extern "C" __attribute__((visibility("default"))) ModInfo* __GetModInfo() { return modinfo; }

extern "C" __attribute__((visibility("default"))) void OnModLoad() {
    aml = (IAML*)GetInterface("AMLInterface");
    if(!aml) return;

    void* fnRecv = dlsym(RTLD_DEFAULT, "recvfrom");
    if(fnRecv) {
        aml->Hook(fnRecv, (void*)HookedRecvfrom, (void**)&origRecvfrom);
        LOG("recvfrom hooked!");
    }

    pthread_t tid;
    pthread_create(&tid, nullptr, toggleThread, nullptr);
    pthread_detach(tid);

    aml->ShowToast(true, "SyncBlocker siap!\nBuat file sb_toggle di Download untuk ON");
    LOG("SyncBlocker ready!");
}
