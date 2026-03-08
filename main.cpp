#include <mod/amlmod.h>
#include <mod/logger.h>
#include <android/log.h>
#include <stdio.h>

MYMOD(com.burhan.myfirstmod, MyFirstMod, 1.0, Burhan)

static Logger loggerLocal;
Logger* logger = &loggerLocal;

ON_MOD_PRELOAD()
{
    __android_log_print(ANDROID_LOG_ERROR, "MYFIRSTMOD", "=== PRELOAD dipanggil! ===");
    FILE* f = fopen("/sdcard/modtest.txt", "w");
    if(f) { fprintf(f, "PRELOAD OK\n"); fclose(f); }
}

ON_MOD_LOAD()
{
    __android_log_print(ANDROID_LOG_ERROR, "MYFIRSTMOD", "=== LOAD dipanggil! ===");
    FILE* f = fopen("/sdcard/modtest.txt", "a");
    if(f) { fprintf(f, "LOAD OK\naml=%p\n", aml); fclose(f); }
}
