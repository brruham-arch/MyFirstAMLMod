#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>
#include <pthread.h>
#include <unistd.h>

#define LOG_TAG "AntiPause"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static ModInfo g_modinfo("com.burhan.antipause", "AntiPause", "2.1", "Burhanudin");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;
uintptr_t pGTASA = 0;

void* AntiPauseThread(void*)
{
    // tunggu 15 detik — biarkan game loading selesai dulu
    LOGI("AntiPause thread waiting for game to load...");
    sleep(15);
    LOGI("AntiPause thread active!");

    while(true)
    {
        if(pGTASA)
        {
            int* isPaused = (int*)(pGTASA + 0x6855bc);
            if(*isPaused != 0)
            {
                *isPaused = 0;
                LOGI("IsAndroidPaused reset!");
            }
        }
        usleep(100000); // cek setiap 100ms
    }
    return nullptr;
}

extern "C" __attribute__((visibility("default"))) ModInfo* __GetModInfo() { return modinfo; }

extern "C" __attribute__((visibility("default"))) void OnModLoad()
{
    aml = (IAML*)GetInterface("AMLInterface");
    if(!aml) return;

    pGTASA = aml->GetLib("libGTASA.so");
    if(!pGTASA) return;

    LOGI("libGTASA base: 0x%X", pGTASA);

    pthread_t thread;
    pthread_create(&thread, nullptr, AntiPauseThread, nullptr);
    pthread_detach(thread);

    LOGI("AntiPause thread launched, delay 15s");
    aml->ShowToast(true, "AntiPause v2.1 aktif!");
}
