#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>
#include <dlfcn.h>

#define LOG_TAG "AntiPause"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static ModInfo g_modinfo("com.burhan.antipause", "AntiPause", "1.4", "Burhanudin");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;

void (*origOnPauseNative)(void*, void*) = nullptr;

void HookedOnPauseNative(void* env, void* thiz)
{
    LOGI("onPauseNative blocked!");
    // tidak panggil original = pause tidak terjadi
}

extern "C" __attribute__((visibility("default"))) ModInfo* __GetModInfo() { return modinfo; }

extern "C" __attribute__((visibility("default"))) void OnModLoad()
{
    aml = (IAML*)GetInterface("AMLInterface");
    if(!aml) return;

    // cari via dlsym langsung, tidak perlu offset
    void* sym = dlsym(RTLD_DEFAULT, "Java_com_nvidia_devtech_NvEventQueueActivity_onPauseNative");
    if(!sym)
    {
        LOGI("onPauseNative symbol not found!");
        aml->ShowToast(true, "AntiPause: symbol not found");
        return;
    }

    LOGI("onPauseNative found at: %p", sym);

    aml->Hook(sym, (void*)HookedOnPauseNative, (void**)&origOnPauseNative);
    LOGI("Hook onPauseNative OK");
    aml->ShowToast(true, "AntiPause v1.4 aktif!");
}
