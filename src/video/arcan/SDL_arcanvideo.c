/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2016 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include "../../SDL_internal.h"

#if SDL_VIDEO_DRIVER_ARCAN
#define TRACE(...)
//#define TRACE(...) {fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n");}

#include "SDL_arcanwindow.h"
#include "SDL_video.h"
#include "SDL_stdinc.h"

#include "SDL_arcanframebuffer.h"
#include "SDL_arcanopengl.h"
#include "SDL_arcanvideo.h"
#include "SDL_arcanevent.h"
#include "SDL_arcanmouse.h"

// #include "SDL_arcandyn.h"

static int
Arcan_VideoInit(_THIS);

static void
Arcan_VideoQuit(_THIS);

static SDL_WindowShaper*
Arcan_CreateShaper(SDL_Window* window)
{
    TRACE("CreateShaper");
    return NULL;
}

static int
Arcan_SetWindowShape(SDL_WindowShaper* shaper, SDL_Surface* shape, SDL_WindowShapeMode* shape_mode)
{
    TRACE("SetWindowShape");
    return SDL_Unsupported();
}

static int
Arcan_ResizeWindowShape(SDL_Window* window)
{
    TRACE("ResizeWindowShape");
    return SDL_Unsupported();
}

static int
Arcan_Available()
{
    return getenv("ARCAN_CONNPATH") != NULL;
}

/*
 * All display- related properties should be deferred until we get a valid
 * DISPLAYHINT event in the Event pump
 */
static void
Arcan_DeleteDevice(SDL_VideoDevice* device)
{
    TRACE("DeleteDevice");
    SDL_free(device);
//    SDL_Arcan_UnloadSymbols();
}

static int
Arcan_GetDisplayBounds(_THIS, SDL_VideoDisplay * display, SDL_Rect * rect)
{
    Arcan_SDL_Meta *arcan_data = _this->driverdata;
    struct arcan_shmif_initial* initial = NULL;
    arcan_shmif_initial(&arcan_data->mcont, &initial);
    TRACE("GetDisplayBounds");

    rect->x = 0;
    rect->y = 0;
    rect->w = arcan_data->disp_w;
    rect->h = arcan_data->disp_h;

    return 0;
}

static int
Arcan_GetDisplayDPI(_THIS, SDL_VideoDisplay * display, float * ddpi, float * hdpi, float *vdpi)
{
    Arcan_SDL_Meta *arcan_data = _this->driverdata;
    struct arcan_shmif_initial* initial;
    arcan_shmif_initial(&arcan_data->mcont, &initial);

    TRACE("GetDisplayDPI");
    if (initial && initial->density > 0){
        float nd = initial->density * 2.54;
        if (*ddpi){
            *ddpi = nd;
        }
        if (*hdpi){
            *hdpi = nd;
        }
        if (*vdpi){
            *vdpi = nd;
        }
        return 0;
    }

    return SDL_SetError("Couldn't get DPI");
}

int
Arcan_VideoInit(_THIS)
{
    Arcan_SDL_Meta *arcan_data = _this->driverdata;
    struct arcan_shmif_initial* initial;
    SDL_VideoDisplay display;
    SDL_DisplayMode mode;
    TRACE("VideoInit");

    SDL_zero(mode);
    SDL_zero(display);

    arcan_shmif_initial(&arcan_data->mcont, &initial);
    if (initial && initial->display_width_px && initial->display_height_px){
        mode.w = initial->display_width_px;
        mode.h = initial->display_height_px;
        mode.refresh_rate = initial->rate;
    }
    else {
        mode.w = arcan_data->mcont.w;
        mode.h = arcan_data->mcont.h;
    }

/* FIXME:
 *  this is not technically correct, should be probed against the static
 *  build format (sizeof + checking packing macro for mask should help)
 */
    mode.format = SDL_PIXELFORMAT_ABGR8888;
    mode.driverdata = NULL;
    arcan_data->format = SDL_PIXELFORMAT_ABGR8888;
    arcan_data->disp_w = mode.w;
    arcan_data->disp_h = mode.h;
    display.desktop_mode = mode;
    display.current_mode = mode;
    SDL_AddVideoDisplay(&display);
    SDL_AddDisplayMode(&display, &mode);
    mode.w = arcan_data->mcont.w;
    mode.h = arcan_data->mcont.h;
    SDL_AddDisplayMode(&display, &mode);

    Arcan_InitMouse();
    return 1;
}

void
Arcan_VideoQuit(_THIS)
{
    struct arcan_shmif_cont *cont = arcan_shmif_primary(SHMIF_INPUT);
    Arcan_SDL_Meta *ameta = (Arcan_SDL_Meta *) cont->user;
    TRACE("VideoQuit");

    SDL_LockMutex(ameta->av_sync);
    ameta->refc--;
    SDL_UnlockMutex(ameta->av_sync);

    if (0 == ameta->refc){
        arcan_shmif_drop(cont);
        SDL_DestroyMutex(ameta->av_sync);
        SDL_free(ameta);
        arcan_shmif_setprimary(SHMIF_INPUT, NULL);
    }

    SDL_EGL_UnloadLibrary(_this);

    _this->driverdata = NULL;
}

static void
Arcan_GetDisplayModes(_THIS, SDL_VideoDisplay* sdl_display)
{
    TRACE("GetDisplayModes");
}

static int
Arcan_SetDisplayMode(_THIS, SDL_VideoDisplay* sdl_display, SDL_DisplayMode* mode)
{
    TRACE("SetDisplayMode");
    return SDL_Unsupported();
}

/*
 * need to cover:
 *  1. no connection
 *  2. connection, but no SDL metadata
 *  3. connection and SDL metdata, return OK
 */
int arcan_av_setup_primary()
{
    Arcan_SDL_Meta *arcan_data = NULL;
    bool local_cont = false;
    struct arcan_shmif_cont *cont = arcan_shmif_primary(SHMIF_INPUT);
    if (!cont){
        cont = (struct arcan_shmif_cont *) SDL_calloc(1,
                                            sizeof(struct arcan_shmif_cont));
        if (!cont){
            return SDL_OutOfMemory();
        }
        *cont = arcan_shmif_open(SEGID_GAME, SHMIF_ACQUIRE_FATALFAIL, NULL);
        local_cont = true;
    }

/* no SDL metadata */
    arcan_data = (Arcan_SDL_Meta *) cont->user;
    if (!arcan_data){
        arcan_data = SDL_calloc(1, sizeof(Arcan_SDL_Meta));
        if (!arcan_data){
            if (local_cont){
                TRACE("dropping");
                arcan_shmif_drop(cont);
            }
            return SDL_OutOfMemory();
        }
        arcan_data->av_sync = SDL_CreateMutex();
        if (!arcan_data->av_sync){
            SDL_free(arcan_data);
            return SDL_OutOfMemory();
        }

        arcan_data->mcont = *cont;
        arcan_shmif_setprimary(SHMIF_INPUT, &arcan_data->mcont);
        arcan_data->mcont.user = arcan_data;
        if (SDL_GetRelativeMouseMode()){
            arcan_shmif_enqueue(cont, &(struct arcan_event){
                .ext.kind = ARCAN_EVENT(CURSORHINT),
                .ext.message.data = "hidden-rel"
            });
        }
    }

    return 0;
}

static SDL_VideoDevice*
Arcan_CreateDevice(int device_index)
{
    SDL_VideoDevice *device = NULL;
    Arcan_SDL_Meta *arcan_data;
    TRACE("CreateDevice");

/*    if (!SDL_Arcan_LoadSymbols()) {
        return NULL;
    }
*/

/*
 * There may already be a global connection provided from Audio setup or
 * elsewhere (e.g. someone using X functions to setup GLX), so we need to
 * try with arcan_av_setup_primary
 */
    if (arcan_av_setup_primary() != 0){
        return NULL;
    }

    arcan_data = (Arcan_SDL_Meta *) arcan_shmif_primary(SHMIF_INPUT)->user;
    arcan_data->refc++;

    device = SDL_calloc(1, sizeof(SDL_VideoDevice));
    device->driverdata = arcan_data;

    /* arcanvideo */
    device->VideoInit        = Arcan_VideoInit;
    device->VideoQuit        = Arcan_VideoQuit;
    device->GetDisplayBounds = Arcan_GetDisplayBounds;
    device->GetDisplayModes  = Arcan_GetDisplayModes;
    device->SetDisplayMode   = Arcan_SetDisplayMode;
    device->GetDisplayDPI    = Arcan_GetDisplayDPI;
    device->free             = Arcan_DeleteDevice;

    /* arcanopengles */
    device->GL_SwapWindow      = Arcan_GL_SwapWindow;
    device->GL_MakeCurrent     = Arcan_EGL_MakeCurrent;
    device->GL_CreateContext   = Arcan_EGL_CreateContext;
    device->GL_DeleteContext   = Arcan_EGL_DeleteContext;
    device->GL_LoadLibrary     = Arcan_EGL_LoadLibrary;
    device->GL_UnloadLibrary   = Arcan_EGL_UnloadLibrary;
    device->GL_GetSwapInterval = Arcan_EGL_GetSwapInterval;
    device->GL_SetSwapInterval = Arcan_EGL_SetSwapInterval;
    device->GL_GetProcAddress  = Arcan_EGL_GetProcAddress;

    /* arcanwindow */
    device->CreateSDLWindow      = Arcan_CreateWindow;
    device->DestroyWindow        = Arcan_DestroyWindow;
    device->GetWindowWMInfo      = Arcan_GetWindowWMInfo;
    device->SetWindowFullscreen  = Arcan_SetWindowFullscreen;
    device->MaximizeWindow       = Arcan_MaximizeWindow;
    device->MinimizeWindow       = Arcan_MinimizeWindow;
    device->RestoreWindow        = Arcan_RestoreWindow;
    device->ShowWindow           = Arcan_RestoreWindow;
    device->HideWindow           = Arcan_HideWindow;
    device->SetWindowSize        = Arcan_SetWindowSize;
    device->SetWindowMinimumSize = Arcan_SetWindowMinimumSize;
    device->SetWindowMaximumSize = Arcan_SetWindowMaximumSize;
    device->SetWindowTitle       = Arcan_SetWindowTitle;

    device->CreateSDLWindowFrom  = NULL;
    /* set as pending, request subsegment, on accept, bind and draw */
    device->SetWindowIcon        = NULL;
    device->RaiseWindow          = NULL;
    device->SetWindowBordered    = NULL;
    device->SetWindowGammaRamp   = NULL;
    device->GetWindowGammaRamp   = NULL;
    device->SetWindowGrab        = NULL;
    device->OnWindowEnter        = NULL;

    /* we can do a viewport hint */
    device->SetWindowPosition    = NULL;

    device->CreateWindowFramebuffer  = Arcan_CreateWindowFramebuffer;
    device->UpdateWindowFramebuffer  = Arcan_UpdateWindowFramebuffer;
    device->DestroyWindowFramebuffer = Arcan_DestroyWindowFramebuffer;

    device->shape_driver.CreateShaper      = Arcan_CreateShaper;
    device->shape_driver.SetWindowShape    = Arcan_SetWindowShape;
    device->shape_driver.ResizeWindowShape = Arcan_ResizeWindowShape;

    device->PumpEvents = Arcan_PumpEvents;

    /* arcan does that to us based on registered archetype already */
    device->SuspendScreenSaver = NULL;

    /* can map to a simple cursor hint and a position */
    device->StartTextInput   = NULL;
    device->StopTextInput    = NULL;
    device->SetTextInputRect = NULL;

    /* we can only 'guess', i.e. hint that we want cursor for input */
    device->HasScreenKeyboardSupport = NULL;
    device->ShowScreenKeyboard       = NULL;
    device->HideScreenKeyboard       = NULL;
    device->IsScreenKeyboardShown    = NULL;

    /* this is 'on demand' in that we request a subseg. on first call,
     * and if that request goes through, we have one */
    device->SetClipboardText = NULL;
    device->GetClipboardText = NULL;
    device->HasClipboardText = NULL;

    device->ShowMessageBox = NULL;

    /* there is also audio and joystick data provided here,
     * should maybe set something like that up */
    return device;
}

VideoBootStrap Arcan_bootstrap = {
    "Arcan", "SDL Arcan video driver",
    Arcan_Available, Arcan_CreateDevice
};

#endif /* SDL_VIDEO_DRIVER_Arcan */

/* vi: set ts=4 sw=4 expandtab: */

