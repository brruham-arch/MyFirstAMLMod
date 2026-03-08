#include <mod/amlmod.h>
#include <android/log.h>
#include <stdio.h>

MYMOD(com.burhan.myfirstmod, MyFirstMod, 1.0, Burhan)

// Ini dipanggil saat .so di-dlopen, sebelum apapun
__attribute__((constructor))
static void onDlOpen()
{
    __android_log_print(ANDROID_LOG_ERROR, "MYFIRSTMOD", "=== dlopen! ===");
    FILE* f = fopen("/storage/emulated/0/Android/data/com.sampmobilerp.game/modtest.txt", "w");
    if(f) { fprintf(f, "dlopen OK\n"); fclose(f); }
}

ON_MOD_PRELOAD() {
    __android_log_print(ANDROID_LOG_ERROR, "MYFIRSTMOD", "=== PRELOAD! ===");
}

ON_MOD_LOAD() {
    __android_log_print(ANDROID_LOG_ERROR, "MYFIRSTMOD", "=== LOAD! ===");
}
