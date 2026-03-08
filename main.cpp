#include <mod/amlmod.h>
#include <mod/logger.h>
#include <android/log.h>
#include <stdio.h>

MYMOD(com.burhan.myfirstmod, MyFirstMod, 1.0, Burhan)

static Logger loggerLocal;
Logger* logger = &loggerLocal;

__attribute__((constructor))
static void onDlOpen()
{
    __android_log_print(ANDROID_LOG_ERROR, "MYFIRSTMOD", "=== CONSTRUCTOR FIRED ===");
    // Tulis ke internal app data (pasti bisa ditulis)
    FILE* f = fopen("/data/data/com.sampmobilerp.game/modtest.txt", "w");
    if(f) { fprintf(f, "constructor OK\n"); fclose(f); }
}

ON_MOD_PRELOAD()
{
    __android_log_print(ANDROID_LOG_ERROR, "MYFIRSTMOD", "=== PRELOAD ===");
}

ON_MOD_LOAD()
{
    __android_log_print(ANDROID_LOG_ERROR, "MYFIRSTMOD", "=== LOAD ===");
}
