#include <mod/amlmod.h>
#include <mod/logger.h>
#include <android/log.h>
#include <stdio.h>

MYMOD(com.burhan.myfirstmod, MyFirstMod, 1.0, Burhan)

static Logger loggerLocal;
Logger* logger = &loggerLocal;

ON_MOD_PRELOAD()
{
    __android_log_print(ANDROID_LOG_ERROR, "MYFIRSTMOD", "PRELOAD! aml=%p", aml);
    FILE* f = fopen("/storage/emulated/0/Android/data/com.sampmobilerp.game/modtest.txt", "w");
    if(f) { fprintf(f, "PRELOAD\naml=%p\n", aml); fclose(f); }
}

ON_MOD_LOAD()
{
    __android_log_print(ANDROID_LOG_ERROR, "MYFIRSTMOD", "LOAD! aml=%p", aml);
    FILE* f = fopen("/storage/emulated/0/Android/data/com.sampmobilerp.game/modtest.txt", "a");
    if(f) { fprintf(f, "LOAD\naml=%p\n", aml); fclose(f); }
}
