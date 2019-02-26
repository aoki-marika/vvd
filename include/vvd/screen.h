#pragma once

#include <EGL/egl.h>

// following standards from sdvx
#define SCREEN_NUMBER 0
#define SCREEN_WIDTH 720
#define SCREEN_HEIGHT 1280
#define SCREEN_RATE 60
#define SCREEN_FRAME_DURATION 1000.0 / SCREEN_RATE

typedef struct
{
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
} Screen;

// create a display screen and egl context
Screen *screen_create();

// frees the given screen from memory
void screen_free(Screen *screen);

// updates the given screens egl context
void screen_update(Screen *screen);
