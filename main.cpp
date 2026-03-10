#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>

#define LOG_TAG "AntiPause"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static ModInfo g_modinfo("com.burhan.antipause", "AntiPause", "3.4", "Burhanudin");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;

int (*origGetNextEvent)(void*, int) = nullptr;

int HookedGetNextEvent(void* ev, int waitMS)
{
    int result = origGetNextEvent(ev, waitMS);
    if(result && ev)
    {
        int* eventType = (int*)ev;
        if(*eventType == 7)
        {
            LOGI("NV_EVENT_PAUSE filtered!");
            *eventType = 0;
        }
    }
    return result;
}

extern "C" __attribute__((visibility("default"))) ModInfo* __GetModInfo() { return modinfo; }

extern "C" __attribute__((visibility("default"))) void OnModLoad()
{
    aml = (IAML*)GetInterface("AMLInterface");
    if(!aml) return;

    uintptr_t pGTASA = aml->GetLib("libGTASA.so");
    if(!pGTASA) return;

    LOGI("libGTASA base: 0x%X", pGTASA);

    aml->Hook(
        (void*)(pGTASA + 0x2696b4),
        (void*)HookedGetNextEvent,
        (void**)&origGetNextEvent
    );

    LOGI("origGetNextEvent: %p", (void*)origGetNextEvent);
    aml->ShowToast(true, "AntiPause v3.4 aktif!");
}
