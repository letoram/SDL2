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

#include "SDL_arcanwindow.h"
#include "SDL_video.h"

#include "SDL_arcanframebuffer.h"
#include "SDL_arcanmouse.h"
#include "SDL_arcanopengl.h"
#include "SDL_arcanvideo.h"

#include "SDL_arcandyn.h"

#define ARCAN_DRIVER_NAME "arcan"

static int
ARCAN_VideoInit(_THIS);

static void
ARCAN_VideoQuit(_THIS);

static int
ARCAN_GetDisplayBounds(_THIS, SDL_VideoDisplay* display, SDL_Rect* rect);

static void
ARCAN_GetDisplayModes(_THIS, SDL_VideoDisplay* sdl_display);

static int
ARCAN_SetDisplayMode(_THIS, SDL_VideoDisplay* sdl_display, SDL_DisplayMode* mode);

static SDL_WindowShaper*
ARCAN_CreateShaper(SDL_Window* window)
{
    return NULL;
}

static int
ARCAN_SetWindowShape(SDL_WindowShaper* shaper, SDL_Surface* shape, SDL_WindowShapeMode* shape_mode)
{
    return SDL_Unsupported();
}

static int
ARCAN_ResizeWindowShape(SDL_Window* window)
{
    return SDL_Unsupported();
}

static int
ARCAN_Available()
{
    return getenv("ARCAN_CONNPATH") != NULL;
}

static void
ARCAN_DeleteDevice(SDL_VideoDevice* device)
{
    SDL_free(device);
    SDL_ARCAN_UnloadSymbols();
}

void
ARCAN_PumpEvents(_THIS)
{
/* grab window-data and run its event loop, translate */
}

static SDL_VideoDevice*
ARCAN_CreateDevice(int device_index)
{
    ARCAN_Data* arcan_data;
    SDL_VideoDevice* device = NULL;

    if (!SDL_ARCAN_LoadSymbols()) {
        return NULL;
    }

    device = SDL_calloc(1, sizeof(SDL_VideoDevice));
    if (!device) {
        SDL_ARCAN_UnloadSymbols();
        SDL_OutOfMemory();
        return NULL;
    }

    arcan_data = SDL_calloc(1, sizeof(ARCAN_Data));
    if (!arcan_data) {
        SDL_free(device);
        SDL_ARCAN_UnloadSymbols();
        SDL_OutOfMemory();
        return NULL;
    }

    device->driverdata = arcan_data;

    /* arcanvideo */
    device->VideoInit        = ARCAN_VideoInit;
    device->VideoQuit        = ARCAN_VideoQuit;
    device->GetDisplayBounds = ARCAN_GetDisplayBounds;
    device->GetDisplayModes  = ARCAN_GetDisplayModes;
    device->SetDisplayMode   = ARCAN_SetDisplayMode;
    device->free             = ARCAN_DeleteDevice;

    /* arcanopengles */
    device->GL_SwapWindow      = ARCAN_GL_SwapWindow;
    device->GL_MakeCurrent     = ARCAN_GL_MakeCurrent;
    device->GL_CreateContext   = ARCAN_GL_CreateContext;
    device->GL_DeleteContext   = ARCAN_GL_DeleteContext;
    device->GL_LoadLibrary     = ARCAN_GL_LoadLibrary;
    device->GL_UnloadLibrary   = ARCAN_GL_UnloadLibrary;
    device->GL_GetSwapInterval = ARCAN_GL_GetSwapInterval;
    device->GL_SetSwapInterval = ARCAN_GL_SetSwapInterval;
    device->GL_GetProcAddress  = ARCAN_GL_GetProcAddress;

    /* arcanwindow */
    device->CreateWindow         = ARCAN_CreateWindow;
    device->DestroyWindow        = ARCAN_DestroyWindow;
    device->GetWindowWMInfo      = ARCAN_GetWindowWMInfo;
    device->SetWindowFullscreen  = ARCAN_SetWindowFullscreen;
    device->MaximizeWindow       = ARCAN_MaximizeWindow;
    device->MinimizeWindow       = ARCAN_MinimizeWindow;
    device->RestoreWindow        = ARCAN_RestoreWindow;
    device->ShowWindow           = ARCAN_RestoreWindow;
    device->HideWindow           = ARCAN_HideWindow;
    device->SetWindowSize        = ARCAN_SetWindowSize;
    device->SetWindowMinimumSize = ARCAN_SetWindowMinimumSize;
    device->SetWindowMaximumSize = ARCAN_SetWindowMaximumSize;
    device->SetWindowTitle       = ARCAN_SetWindowTitle;

    device->CreateWindowFrom     = NULL;
    device->SetWindowIcon        = NULL;
    device->RaiseWindow          = NULL;
    device->SetWindowBordered    = NULL;
    device->SetWindowGammaRamp   = NULL;
    device->GetWindowGammaRamp   = NULL;
    device->SetWindowGrab        = NULL;
    device->OnWindowEnter        = NULL;
    device->SetWindowPosition    = NULL;

    /* arcanframebuffer */
    device->CreateWindowFramebuffer  = ARCAN_CreateWindowFramebuffer;
    device->UpdateWindowFramebuffer  = ARCAN_UpdateWindowFramebuffer;
    device->DestroyWindowFramebuffer = ARCAN_DestroyWindowFramebuffer;

    device->shape_driver.CreateShaper      = ARCAN_CreateShaper;
    device->shape_driver.SetWindowShape    = ARCAN_SetWindowShape;
    device->shape_driver.ResizeWindowShape = ARCAN_ResizeWindowShape;

    device->PumpEvents = ARCAN_PumpEvents;

    /* arcan does that to us based on registered archetype already */
    device->SuspendScreenSaver = NULL;

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

VideoBootStrap ARCAN_bootstrap = {
    ARCAN_DRIVER_NAME, "SDL Arcan video driver",
    ARCAN_Available, ARCAN_CreateDevice
};

static void
ARCAN_SetCurrentDisplayMode(arcanDisplayOutput const* out, SDL_VideoDisplay* display)
{
    SDL_DisplayMode mode = {
        .format = SDL_PIXELFORMAT_RGB888,
        .w = out->modes[out->current_mode].horizontal_resolution,
        .h = out->modes[out->current_mode].vertical_resolution,
        .refresh_rate = out->modes[out->current_mode].refresh_rate,
        .driverdata = NULL
    };

    display->desktop_mode = mode;
    display->current_mode = mode;
}

static void
ARCAN_AddAllModesFromDisplay(arcanDisplayOutput const* out, SDL_VideoDisplay* display)
{
    int n_mode;
    for (n_mode = 0; n_mode < out->num_modes; ++n_mode) {
        SDL_DisplayMode mode = {
            .format = SDL_PIXELFORMAT_RGB888,
            .w = out->modes[n_mode].horizontal_resolution,
            .h = out->modes[n_mode].vertical_resolution,
            .refresh_rate = out->modes[n_mode].refresh_rate,
            .driverdata = NULL
        };

        SDL_AddDisplayMode(display, &mode);
    }
}

static void
ARCAN_InitDisplays(_THIS)
{
}

int
ARCAN_VideoInit(_THIS)
{
    ARCAN_Data* arcan_data = _this->driverdata;
    arcan_data->conn = arcan_shmif_open(SEGID_GAME, SHMIF_ACQUIRE_FATALFAIL, &args);
    arcan_data->current_window = NULL;
    arcan_shmif_setprimary(SHMIF_INPUT, &arcan_data->conn);

/* we don't really care (unless we are forced to readback) */
    arcan_data->pixel_format   = arcan_pixel_format_invalid;

    ARCAN_InitDisplays(_this);
    ARCAN_InitMouse();

    return 0;
}

void
ARCAN_VideoQuit(_THIS)
{
    ARCAN_Data* arcan_data = _this->driverdata;

    ARCAN_GL_DeleteContext(_this, NULL);
    ARCAN_GL_UnloadLibrary(_this);

    arcan_shmif_drop(&arcan_data->conn);

    SDL_free(arcan_data);
    _this->driverdata = NULL;
}

static int
ARCAN_GetDisplayBounds(_THIS, SDL_VideoDisplay* display, SDL_Rect* rect)
{
    ARCAN_Data* arcan_data = _this->driverdata;
    int d;

/* set rect to width / height */

    return 0;
}

static void
ARCAN_GetDisplayModes(_THIS, SDL_VideoDisplay* sdl_display)
{
}

static int
ARCAN_SetDisplayMode(_THIS, SDL_VideoDisplay* sdl_display, SDL_DisplayMode* mode)
{
    return 0;
}

#endif /* SDL_VIDEO_DRIVER_ARCAN */

/* vi: set ts=4 sw=4 expandtab: */

