#include <mod/amlmod.h>
#include <mod/iaml.h>
#include <android/log.h>
#include <dlfcn.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define LOG(fmt, ...) __android_log_print(ANDROID_LOG_INFO, "MyFirstMod", fmt, ##__VA_ARGS__)

static ModInfo g_modinfo("com.burhan.myfirstmod", "GLOverlay", "1.0", "Burhan");
ModInfo* modinfo = &g_modinfo;
IAML* aml = nullptr;

static int  g_fps      = 0;
static int  g_fpsCount = 0;
static long g_startTime = 0;
static int  g_frameCount = 0; // total frame sejak start

static GLuint g_prog  = 0;
static GLuint g_vbo   = 0;
static bool   g_ready = false;

static const char* VS = R"(
attribute vec2 pos;
void main() { gl_Position = vec4(pos, 0.0, 1.0); }
)";
static const char* FS = R"(
precision mediump float;
uniform vec4 color;
void main() { gl_FragColor = color; }
)";

GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    return s;
}

void initGL() {
    if(g_ready) return;
    GLuint vs = compileShader(GL_VERTEX_SHADER, VS);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, FS);
    g_prog = glCreateProgram();
    glAttachShader(g_prog, vs);
    glAttachShader(g_prog, fs);
    glLinkProgram(g_prog);
    glGenBuffers(1, &g_vbo);
    g_startTime = time(nullptr);
    g_ready = true;
    LOG("GL overlay init OK");
}

void drawRect(float x, float y, float w, float h,
              float r, float g, float b, float a)
{
    float verts[] = { x, y, x+w, y, x, y-h, x+w, y-h };
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
    GLint pos = glGetAttribLocation(g_prog, "pos");
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(pos, 2, GL_FLOAT, GL_FALSE, 0, 0);
    GLint col = glGetUniformLocation(g_prog, "color");
    glUniform4f(col, r, g, b, a);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(pos);
}

static const uint8_t DIGITS[10][5] = {
    {0b111,0b101,0b101,0b101,0b111},
    {0b010,0b110,0b010,0b010,0b111},
    {0b111,0b001,0b111,0b100,0b111},
    {0b111,0b001,0b111,0b001,0b111},
    {0b101,0b101,0b111,0b001,0b001},
    {0b111,0b100,0b111,0b001,0b111},
    {0b111,0b100,0b111,0b101,0b111},
    {0b111,0b001,0b001,0b001,0b001},
    {0b111,0b101,0b111,0b101,0b111},
    {0b111,0b101,0b111,0b001,0b111},
};

void drawDigit(int d, float x, float y, float sz, float r, float g, float b) {
    if(d < 0 || d > 9) return;
    float px = sz * 0.4f, py = sz * 0.4f;
    for(int row = 0; row < 5; row++)
        for(int col = 0; col < 3; col++)
            if(DIGITS[d][row] & (1 << (2-col)))
                drawRect(x+col*px, y-row*py, px*0.85f, py*0.85f, r, g, b, 1.0f);
}

void drawNumber(int num, float x, float y, float sz, float r, float g, float b) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", num < 0 ? 0 : num);
    for(int i = 0; buf[i]; i++, x += sz*0.4f*4)
        drawDigit(buf[i]-'0', x, y, sz, r, g, b);
}

typedef EGLBoolean (*eglSwapBuffers_t)(EGLDisplay, EGLSurface);
eglSwapBuffers_t origSwap = nullptr;

EGLBoolean HookedSwap(EGLDisplay dpy, EGLSurface surface)
{
    g_frameCount++;

    // Hitung FPS
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    static long lastNs = 0;
    static float fpsTimer = 0;
    long nowNs = ts.tv_sec * 1000000000LL + ts.tv_nsec;
    if(lastNs) {
        float dt = (nowNs - lastNs) / 1e9f;
        fpsTimer += dt;
        g_fpsCount++;
        if(fpsTimer >= 1.0f) {
            g_fps      = g_fpsCount;
            g_fpsCount = 0;
            fpsTimer   = 0.0f;
        }
    }
    lastNs = nowNs;

    // Tunggu 300 frame dulu (loading selesai) baru render overlay
    if(g_frameCount > 300)
    {
        initGL();

        if(g_ready)
        {
            // Simpan state
            GLint prevProg;
            glGetIntegerv(GL_CURRENT_PROGRAM, &prevProg);
            GLboolean depthTest, blend, scissor;
            glGetBooleanv(GL_DEPTH_TEST,  &depthTest);
            glGetBooleanv(GL_BLEND,       &blend);
            glGetBooleanv(GL_SCISSOR_TEST,&scissor);

            glDisable(GL_DEPTH_TEST);
            glDisable(GL_SCISSOR_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glUseProgram(g_prog);

            float px = -1.0f + 0.02f;
            float py =  1.0f - 0.02f;
            float sz =  0.022f;

            // Background
            drawRect(px-0.01f, py+0.01f, 0.22f, 0.14f, 0.0f, 0.0f, 0.0f, 0.55f);

            // FPS - kuning
            drawNumber(g_fps, px, py, sz, 1.0f, 1.0f, 0.0f);

            // Timer - hijau
            long elapsed = time(nullptr) - g_startTime;
            drawNumber((int)(elapsed/60), px,          py-0.06f, sz, 0.0f, 1.0f, 0.4f);
            drawRect(px+0.062f, py-0.065f, 0.005f, 0.005f, 0.0f,1.0f,0.4f,1.0f);
            drawRect(px+0.062f, py-0.075f, 0.005f, 0.005f, 0.0f,1.0f,0.4f,1.0f);
            drawNumber((int)(elapsed%60), px+0.072f,   py-0.06f, sz, 0.0f, 1.0f, 0.4f);

            // Restore state
            glUseProgram(prevProg);
            if(depthTest)  glEnable(GL_DEPTH_TEST);
            if(!blend)     glDisable(GL_BLEND);
            if(scissor)    glEnable(GL_SCISSOR_TEST);
        }
    }

    return origSwap(dpy, surface);
}

extern "C" __attribute__((visibility("default"))) ModInfo* __GetModInfo() { return modinfo; }

extern "C" __attribute__((visibility("default"))) void OnModLoad() {
    aml = (IAML*)GetInterface("AMLInterface");
    if(!aml) return;

    void* fnSwap = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if(fnSwap) {
        aml->Hook(fnSwap, (void*)HookedSwap, (void**)&origSwap);
        LOG("eglSwapBuffers hooked!");
        aml->ShowToast(true, "GL Overlay aktif!");
    }
}
