#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>
#include <dlfcn.h>
#include <unistd.h>
#include <cstring>
#include <cstdint>

#define LOG_TAG "BurhanAMLMod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static ModInfo g_modinfo("com.burhan.amlnewmod", "BurhanAMLNewMod", "1.0", "Burhanudin");
ModInfo* modinfo = &g_modinfo;

IAML* aml = nullptr;

uintptr_t libGTASA = 0;

void PrintGameInfo()
{
    if(!aml) return;

    libGTASA = (uintptr_t)aml->GetLib("libGTASA.so");

    if(libGTASA)
    {
        LOGI("libGTASA base address: %p", (void*)libGTASA);
        aml->ShowToast(true, "libGTASA base: %p", (void*)libGTASA);
    }
    else
    {
        LOGE("libGTASA not found");
        aml->ShowToast(true, "libGTASA not found");
    }
}

void TestPatternScan()
{
    if(!aml) return;

    void* result = aml->PatternScan("00 00 80 3F ?? ?? ?? ??", "libGTASA.so");

    if(result)
    {
        LOGI("Pattern found at: %p", result);
        aml->ShowToast(true, "Pattern found: %p", result);
    }
    else
    {
        LOGE("Pattern not found");
        aml->ShowToast(true, "Pattern not found");
    }
}

void TestNOPPatch()
{
    if(!aml) return;
    if(!libGTASA) return;

    uintptr_t testAddress = libGTASA + 0x1000;

    aml->PlaceNOP(testAddress, 4);

    LOGI("NOP placed at: %p", (void*)testAddress);
    aml->ShowToast(true, "NOP placed");
}

typedef void (*GameTickFn)();
GameTickFn origGameTick = nullptr;

void HookedGameTick()
{
    if(origGameTick)
        origGameTick();

    static int counter = 0;
    counter++;

    if(counter == 600)
    {
        LOGI("GameTick hook working");
        aml->ShowToast(true, "Hook aktif");
    }
}

void InstallHookExample()
{
    if(!aml) return;
    if(!libGTASA) return;

    uintptr_t hookAddress = libGTASA + 0x2000;

    aml->Hook((void*)hookAddress, (void*)HookedGameTick, (void**)&origGameTick);

    LOGI("Hook installed at: %p", (void*)hookAddress);
}

void RunModInit()
{
    PrintGameInfo();
    TestPatternScan();
    TestNOPPatch();
    InstallHookExample();
}

extern "C" __attribute__((visibility("default"))) ModInfo* __GetModInfo()
{
    return modinfo;
}

extern "C" __attribute__((visibility("default"))) void OnModLoad()
{
    aml = (IAML*)GetInterface("AMLInterface");

    if(!aml)
    {
        LOGE("AML interface not found");
        return;
    }

    LOGI("=== Burhan AML Mod Loaded ===");

    aml->ShowToast(true, "Burhan AML Mod aktif");

    RunModInit();
}
