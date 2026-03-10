#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>
#include <pthread.h>
#include <unistd.h>

#define LOG_TAG "AntiPause"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static ModInfo g_modinfo("com.burhan.antipause", "AntiPause", "2.7", "Burhanudin");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;
uintptr_t pGTASA = 0;

void* AntiPauseThread(void*)
{
    // tunggu game selesai loading
    sleep(20);
    LOGI("AntiPause thread active!");

    while(true)
    {
        if(pGTASA)
        {
            // reset kedua variable sekaligus
            *(int*)(pGTASA + 0x6855bc) = 0; // IsAndroidPaused
            *(int*)(pGTASA + 0x6d7048) = 0; // WasAndroidPaused
        }
        usleep(50000); // 50ms — lebih agresif dari sebelumnya
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

    LOGI("Thread launched");
    aml->ShowToast(true, "AntiPause v2.7 aktif!");
}
