#include <mod/amlmod.h>
#include <mod/logger.h>

MYMOD(com.burhan.myfirstmod, MyFirstMod, 1.0, Burhan)
static Logger loggerLocal;
Logger* logger = &loggerLocal;
ON_MOD_LOAD()
{
aml->ShowToast(true, "MyFirstMod loaded!");
    logger->SetTag("MyFirstMod");
    logger->Info("=== MyFirstMod berhasil di-load! ===");

    uintptr_t pGTASA = aml->GetLib("libGTASA.so");
    if(pGTASA)
        logger->Info("libGTASA.so ditemukan di: 0x%X", pGTASA);
    else
        logger->Error("libGTASA.so tidak ditemukan!");
}
