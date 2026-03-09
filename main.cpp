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

// ── FPS counter ────────────────────────────────────────────────
static int   g_fps       = 0;
static int   g_fpsCount  = 0;
static float g_fpsTimer  = 0.0f;
static long  g_startTime = 0;

// ── OpenGL resources ───────────────────────────────────────────
static GLuint g_prog  = 0;
static GLuint g_vbo   = 0;
static bool   g_ready = false;

// Simple vertex + fragment shader
static const char* VS = R"(
attribute vec2 pos;
void main() {
    gl_Position = vec4(pos, 0.0, 1.0);
}
)";

static const char* FS = R"(
precision mediump float;
uniform vec4 color;
void main() {
    gl_FragColor = color;
}
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
    LOG("OpenGL overlay initialized!");
}

// Gambar rectangle: x,y,w,h dalam NDC (-1 sampai 1)
void drawRect(float x, float y, float w, float h,
              float r, float g, float b, float a)
{
    float verts[] = {
        x,     y,
        x+w,   y,
        x,     y-h,
        x+w,   y-h,
    };

    glUseProgram(g_prog);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);

    GLint posLoc = glGetAttribLocation(g_prog, "pos");
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);

    GLint colLoc = glGetUniformLocation(g_prog, "color");
    glUniform4f(colLoc, r, g, b, a);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(posLoc);
}

// Gambar digit 0-9 sebagai bar sederhana (pixel art style)
// Setiap digit = 3x5 grid of rectangles
static const uint8_t DIGITS[10][5] = {
    {0b111, 0b101, 0b101, 0b101, 0b111}, // 0
    {0b010, 0b110, 0b010, 0b010, 0b111}, // 1
    {0b111, 0b001, 0b111, 0b100, 0b111}, // 2
    {0b111, 0b001, 0b111, 0b001, 0b111}, // 3
    {0b101, 0b101, 0b111, 0b001, 0b001}, // 4
    {0b111, 0b100, 0b111, 0b001, 0b111}, // 5
    {0b111, 0b100, 0b111, 0b101, 0b111}, // 6
    {0b111, 0b001, 0b001, 0b001, 0b001}, // 7
    {0b111, 0b101, 0b111, 0b101, 0b111}, // 8
    {0b111, 0b101, 0b111, 0b001, 0b111}, // 9
};

void drawDigit(int digit, float x, float y, float size,
               float r, float g, float b)
{
    if(digit < 0 || digit > 9) return;
    float px = size * 0.4f; // pixel width
    float py = size * 0.4f; // pixel height
    for(int row = 0; row < 5; row++) {
        for(int col = 0; col < 3; col++) {
            if(DIGITS[digit][row] & (1 << (2-col))) {
                drawRect(x + col*px, y - row*py, px*0.9f, py*0.9f, r, g, b, 1.0f);
            }
        }
    }
}

void drawNumber(int num, float x, float y, float size,
                float r, float g, float b)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", num);
    float cx = x;
    for(int i = 0; buf[i]; i++) {
        drawDigit(buf[i] - '0', cx, y, size, r, g, b);
        cx += size * 0.4f * 4; // advance per digit
    }
}

// ── eglSwapBuffers hook ────────────────────────────────────────
typedef EGLBoolean (*eglSwapBuffers_t)(EGLDisplay, EGLSurface);
eglSwapBuffers_t origSwap = nullptr;

EGLBoolean HookedSwap(EGLDisplay dpy, EGLSurface surface)
{
    // Hitung FPS
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    static long lastNs = 0;
    long nowNs = ts.tv_sec * 1000000000LL + ts.tv_nsec;
    if(lastNs != 0) {
        float dt = (nowNs - lastNs) / 1e9f;
        g_fpsTimer += dt;
        g_fpsCount++;
        if(g_fpsTimer >= 1.0f) {
            g_fps      = g_fpsCount;
            g_fpsCount = 0;
            g_fpsTimer = 0.0f;
        }
    }
    lastNs = nowNs;

    // Init GL jika belum
    initGL();

    if(g_ready && g_prog)
    {
        // Simpan GL state
        GLboolean depthTest, blend;
        glGetBooleanv(GL_DEPTH_TEST, &depthTest);
        glGetBooleanv(GL_BLEND, &blend);

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // ── Panel background (kiri atas) ──
        // NDC: kiri=-1, kanan=1, atas=1, bawah=-1
        float px = -1.0f + 0.02f;  // kiri
        float py =  1.0f - 0.02f;  // atas
        float pw =  0.18f;
        float ph =  0.12f;

        // Background semi-transparan
        drawRect(px - 0.01f, py + 0.01f, pw + 0.02f, ph + 0.02f,
                 0.0f, 0.0f, 0.0f, 0.5f);

        // ── FPS (kuning) ──
        float sz = 0.022f;
        drawNumber(g_fps, px, py, sz, 1.0f, 1.0f, 0.0f);

        // Label "FPS" - 3 bar pendek
        float lx = px + 0.10f;
        drawRect(lx,        py,        0.005f, 0.02f, 1.0f, 1.0f, 0.0f, 1.0f);
        drawRect(lx+0.008f, py,        0.005f, 0.02f, 1.0f, 1.0f, 0.0f, 1.0f);
        drawRect(lx+0.016f, py,        0.005f, 0.02f, 1.0f, 1.0f, 0.0f, 1.0f);

        // ── Online time (hijau) ──
        long elapsed = time(nullptr) - g_startTime;
        int  minutes = (int)(elapsed / 60);
        int  seconds = (int)(elapsed % 60);

        float py2 = py - 0.055f;
        drawNumber(minutes, px,          py2, sz, 0.0f, 1.0f, 0.4f);
        drawNumber(seconds, px + 0.075f, py2, sz, 0.0f, 1.0f, 0.4f);
        // Titik dua pemisah
        drawRect(px+0.068f, py2-0.005f, 0.005f, 0.005f, 0.0f, 1.0f, 0.4f, 1.0f);
        drawRect(px+0.068f, py2-0.015f, 0.005f, 0.005f, 0.0f, 1.0f, 0.4f, 1.0f);

        // Restore GL state
        if(depthTest) glEnable(GL_DEPTH_TEST);
        if(!blend)    glDisable(GL_BLEND);
    }

    return origSwap(dpy, surface);
}

extern "C" __attribute__((visibility("default"))) ModInfo* __GetModInfo() { return modinfo; }

extern "C" __attribute__((visibility("default"))) void OnModLoad() {
    aml = (IAML*)GetInterface("AMLInterface");
    if(!aml) return;

    void* fnSwap = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    LOG("eglSwapBuffers = %p", fnSwap);

    if(fnSwap) {
        aml->Hook(fnSwap, (void*)HookedSwap, (void**)&origSwap);
        LOG("OpenGL overlay hook terpasang!");
        aml->ShowToast(true, "GL Overlay aktif!\nFPS + Timer online");
    }
}
