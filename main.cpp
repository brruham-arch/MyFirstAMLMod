#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>
#include <pthread.h>
#include <unistd.h>

#define LOG_TAG "AntiPause"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static ModInfo g_modinfo("com.burhan.antipause", "AntiPause", "2.0", "Burhanudin");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;
uintptr_t pGTASA = 0;

void* AntiPauseThread(void*)
{
    LOGI("AntiPause thread started");
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
        usleep(100000); // 100ms
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

    LOGI("AntiPause thread launched");
    aml->ShowToast(true, "AntiPause v2.0 aktif!");
}
