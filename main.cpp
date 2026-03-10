#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>

#define LOG_TAG "AntiPause"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static ModInfo g_modinfo("com.burhan.antipause", "AntiPause", "3.3", "Burhanudin");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;

// NVEventGetNextEvent(NVEvent* ev, int waitMS) → returns bool
bool (*origGetNextEvent)(void*, int) = nullptr;

bool HookedGetNextEvent(void* ev, int waitMS)
{
    bool result = origGetNextEvent(ev, waitMS);
    if(result && ev)
    {
        int* eventType = (int*)ev;
        // NV_EVENT_PAUSE = 7, NV_EVENT_RESUME = 8 (standard NVEvent values)
        if(*eventType == 7)
        {
            LOGI("NV_EVENT_PAUSE filtered!");
            *eventType = 0; // ubah ke event kosong
        }
        else if(*eventType == 8)
        {
            LOGI("NV_EVENT_RESUME passed through");
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

    // hook NVEventGetNextEvent di offset 0x2696b4
    aml->Hook(
        (void*)(pGTASA + 0x2696b4),
        (void*)HookedGetNextEvent,
        (void**)&origGetNextEvent
    );

    LOGI("origGetNextEvent: %p", (void*)origGetNextEvent);
    LOGI("Hook NVEventGetNextEvent OK");
    aml->ShowToast(true, "AntiPause v3.3 aktif!");
}
