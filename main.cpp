#include <mod/amlmod.h>
#include <mod/logger.h>
#include <android/log.h>

MYMOD(com.burhan.myfirstmod, MyFirstMod, 1.0, Burhan)

static Logger loggerLocal;
Logger* logger = &loggerLocal;

ON_MOD_LOAD()
{
    __android_log_print(ANDROID_LOG_ERROR, "MYFIRSTMOD", "=== OnModLoad dipanggil! aml=%p ===", aml);
    
    if(aml)
    {
        aml->ShowToast(true, "MyFirstMod loaded!");
        logger->SetTag("MyFirstMod");
        logger->Info("=== berhasil! ===");
    }
}
