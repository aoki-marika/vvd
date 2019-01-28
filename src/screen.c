#include "screen.h"

#include <assert.h>
#include <bcm_host.h>
#include <GLES2/gl2.h>

void assert_gl()
{
    assert(glGetError() == 0);
}

Screen *create_screen()
{
    // init bcm for getting a frambuffer output
    bcm_host_init();

    // dispman handles
    static EGL_DISPMANX_WINDOW_T window;
    DISPMANX_ELEMENT_HANDLE_T dispman_element;
    DISPMANX_DISPLAY_HANDLE_T dispman_display;
    DISPMANX_UPDATE_HANDLE_T dispman_update;

    // rects for screen and output resolution
    VC_RECT_T screen_rect;
    VC_RECT_T display_rect;

    // egl attributes
    static const EGLint attributes[] =
    {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 16, // needed for depth buffering
        EGL_SURFACE_TYPE,
        EGL_WINDOW_BIT,
        EGL_NONE,
    };

    // egl context attributes
    static const EGLint context_attributes[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE,
    };

    EGLBoolean result;
    Screen *screen = malloc(sizeof(Screen));
    EGLConfig config;

    // get an egl display connection
    screen->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    assert(screen->display != EGL_NO_DISPLAY);
    assert_gl();

    // initialize the egl display connection
    result = eglInitialize(screen->display, NULL, NULL);
    assert(result != EGL_FALSE);
    assert_gl();

    // get an appropriate egl framebuffer configuration
    EGLint num_config;
    result = eglChooseConfig(screen->display, attributes, &config, 1, &num_config);
    assert(result != EGL_FALSE);
    assert_gl();

    // get an approiate egl api
    result = eglBindAPI(EGL_OPENGL_ES_API);
    assert(result != EGL_FALSE);
    assert_gl();

    // create an egl rendering context
    screen->context = eglCreateContext(screen->display, config, EGL_NO_CONTEXT, context_attributes);
    assert(screen->context != EGL_NO_CONTEXT);
    assert_gl();

    // create an egl window surface
    // set display_rect to the resolution of the screen
    int32_t success = 0;

    display_rect.x = 0;
    display_rect.y = 0;
    success = graphics_get_display_size(SCREEN_NUMBER, &display_rect.width, &display_rect.height);
    assert(success >= 0);

    // set screen_rect to the resolution specified by SCREEN_WIDTH and SCREEN_HEIGHT
    screen_rect.x = 0;
    screen_rect.y = 0;
    screen_rect.width = SCREEN_WIDTH << 16;
    screen_rect.height = SCREEN_HEIGHT << 16;

    // get dispman handles
    dispman_display = vc_dispmanx_display_open(SCREEN_NUMBER);
    dispman_update = vc_dispmanx_update_start(0);
    dispman_element = vc_dispmanx_element_add(dispman_update,
                                              dispman_display,
                                              0,
                                              &display_rect,
                                              0,
                                              &screen_rect,
                                              DISPMANX_PROTECTION_NONE,
                                              0,
                                              0,
                                              (DISPMANX_TRANSFORM_T)0);

    // create the dispman window
    window.element = dispman_element;
    window.width = SCREEN_WIDTH;
    window.height = SCREEN_HEIGHT;
    vc_dispmanx_update_submit_sync(dispman_update);

    // create the egl surface
    screen->surface = eglCreateWindowSurface(screen->display, config, &window, NULL);
    assert(screen->surface != EGL_NO_SURFACE);
    assert_gl();

    // connect the context to the surface
    result = eglMakeCurrent(screen->display, screen->surface, screen->surface, screen->context);
    assert(result != EGL_FALSE);
    assert_gl();

    // return the screen
    return screen;
}

void update_screen(Screen *screen)
{
    // swap the screen buffers
    eglSwapBuffers(screen->display, screen->surface);
}
