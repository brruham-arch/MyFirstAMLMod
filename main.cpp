#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>
#include <pthread.h>
#include <unistd.h>
#include <cstdio>

#define LOG_TAG "AntiPause"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static ModInfo g_modinfo("com.burhan.antipause", "AntiPause", "4.0", "Burhanudin");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;

// file ini kalau ADA = AntiPause ON, kalau TIDAK ADA = OFF
#define TRIGGER_FILE "/storage/emulated/0/Download/antipause_on"

bool isActive = false;

void (*origNvOnPause)() = nullptr;

void HookedNvOnPause()
{
    if(isActive) return; // block pause
}

void* WatcherThread(void*)
{
    while(true)
    {
        FILE* f = fopen(TRIGGER_FILE, "r");
        bool fileExists = (f != nullptr);
        if(f) fclose(f);

        if(fileExists != isActive)
        {
            isActive = fileExists;
            LOGI("AntiPause: %s", isActive ? "ON" : "OFF");
            aml->ShowToast(false, "AntiPause: %s", isActive ? "ON" : "OFF");
        }
        sleep(1);
    }
    return nullptr;
}

extern "C" __attribute__((visibility("default"))) ModInfo* __GetModInfo() { return modinfo; }

extern "C" __attribute__((visibility("default"))) void OnModLoad()
{
    aml = (IAML*)GetInterface("AMLInterface");
    if(!aml) return;

    uintptr_t pGTASA = aml->GetLib("libGTASA.so");
    if(!pGTASA) return;

    aml->Hook(
        (void*)(pGTASA + 0x274001),
        (void*)HookedNvOnPause,
        (void**)&origNvOnPause
    );

    pthread_t thread;
    pthread_create(&thread, nullptr, WatcherThread, nullptr);
    pthread_detach(thread);

    LOGI("AntiPause v4.0 ready");
    aml->ShowToast(true, "AntiPause v4.0 ready!");
}
