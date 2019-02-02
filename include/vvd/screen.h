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

// get a reference to the current display screen and create an egl context
Screen *screen_get();

// frees the given screen from memory
void screen_free(Screen *screen);

// updates the given screens egl context
void screen_update(Screen *screen);
