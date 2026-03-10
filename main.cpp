#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>

#define LOG_TAG "AntiPause"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static ModInfo g_modinfo("com.burhan.antipause", "AntiPause", "2.8", "Burhanudin");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;

// kandidat onResume — hook semua, lihat mana yang terpanggil saat resume
void (*orig1)(void*,void*) = nullptr;
void (*orig2)(void*,void*) = nullptr;
void (*orig3)(void*,void*) = nullptr;
void (*orig4)(void*,void*) = nullptr;

void Hook1(void* e,void* t){ LOGI("0x273F01 called!"); if(orig1)orig1(e,t); }
void Hook2(void* e,void* t){ LOGI("0x274101 called!"); if(orig2)orig2(e,t); }
void Hook3(void* e,void* t){ LOGI("0x274201 called!"); if(orig3)orig3(e,t); }
void Hook4(void* e,void* t){ LOGI("0x274301 called!"); if(orig4)orig4(e,t); }

extern "C" __attribute__((visibility("default"))) ModInfo* __GetModInfo() { return modinfo; }

extern "C" __attribute__((visibility("default"))) void OnModLoad()
{
    aml = (IAML*)GetInterface("AMLInterface");
    if(!aml) return;

    uintptr_t pGTASA = aml->GetLib("libGTASA.so");
    if(!pGTASA) return;

    LOGI("libGTASA base: 0x%X", pGTASA);

    aml->Hook((void*)(pGTASA + 0x273F01),(void*)Hook1,(void**)&orig1);
    aml->Hook((void*)(pGTASA + 0x274101),(void*)Hook2,(void**)&orig2);
    aml->Hook((void*)(pGTASA + 0x274201),(void*)Hook3,(void**)&orig3);
    aml->Hook((void*)(pGTASA + 0x274301),(void*)Hook4,(void**)&orig4);

    LOGI("Probe hooks installed");
    aml->ShowToast(true, "AntiPause v2.8 probe");
}
