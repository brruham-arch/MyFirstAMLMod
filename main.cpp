#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>

#define LOG(fmt, ...) __android_log_print(ANDROID_LOG_INFO, "BurhanAML", fmt, ##__VA_ARGS__)

static ModInfo g_modinfo("com.burhan.testmod", "BurhanTestMod", "1.0", "Burhan");
ModInfo* modinfo = &g_modinfo;

IAML* aml = nullptr;

extern "C"
__attribute__((visibility("default")))
ModInfo* __GetModInfo()
{
    return modinfo;
}

extern "C"
__attribute__((visibility("default")))
void OnModLoad()
{
    LOG("=== BurhanTestMod loaded ===");

    aml = (IAML*)GetInterface("AMLInterface");
    if(!aml)
    {
        LOG("AML interface not found");
        return;
    }

    LOG("AML interface acquired");

    aml->ShowToast(true, "BurhanTestMod aktif");
}
