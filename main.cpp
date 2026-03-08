#include <mod/amlmod.h>
#include <mod/logger.h>
#include <android/log.h>
#include <stdio.h>

MYMOD(com.burhan.myfirstmod, MyFirstMod, 1.0, Burhan)

static Logger loggerLocal;
Logger* logger = &loggerLocal;

ON_MOD_LOAD()
{
    __android_log_print(ANDROID_LOG_ERROR, "MYFIRSTMOD", "OnModLoad called! aml=%p", aml);

    FILE* f = fopen("/storage/emulated/0/Download/modtest.txt", "w");
    if(f)
    {
        fprintf(f, "OnModLoad dipanggil!\naml pointer: %p\n", aml);
        fclose(f);
    }
}
