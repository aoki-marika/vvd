#pragma once

#include <EGL/egl.h>

// following standards from sdvx
#define SCREEN_NUMBER 0
#define SCREEN_WIDTH 720
#define SCREEN_HEIGHT 1280
#define SCREEN_RATE 60

typedef struct
{
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
} Screen;

Screen *create_screen();
void update_screen(Screen *screen);
