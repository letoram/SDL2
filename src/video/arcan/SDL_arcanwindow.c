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

/*
  Contributed by Bjorn Stahl, <contact@arcan-fe.com>
*/

#include "../../SDL_internal.h"

#if SDL_VIDEO_DRIVER_ARCAN

#include "../SDL_egl_c.h"
#include "../SDL_sysvideo.h"
#include "../../events/SDL_keyboard_c.h"
#include "../../events/SDL_mouse_c.h"
#include "SDL_arcanvideo.h"
#include "SDL_arcanwindow.h"
#include "SDL_arcanopengl.h"

#define TRACE(...)
//#define TRACE(...) {fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n");}

int
Arcan_CreateWindow(_THIS, SDL_Window* window)
{
    Arcan_SDL_Meta *meta = _this->driverdata;
    Arcan_WindowData *data;
    struct arcan_shmif_cont *con;
    TRACE("Create Window");
/*
 * FIXME: secondary windows not yet working, need to request
 * a new one, map it and so on
 */
/*    if (meta->main == NULL){ */
        meta->main = window;
        con = &meta->mcont;
/*    }
    else { */
//        printf("need to create secondary window, missing\n");
 //       return -1;
 //
//    }

    data = (Arcan_WindowData *) SDL_calloc(sizeof(Arcan_WindowData), 1);
    window->driverdata = data;
    data->con = con;

    /* Need this to be a critical section since we have audio running
     * in a separate thread */
    SDL_LockMutex(meta->av_sync);
    arcan_shmif_resize(con, window->w, window->h);
    SDL_UnlockMutex(meta->av_sync);

    if (window->flags & SDL_WINDOW_OPENGL){
        con->hints = SHMIF_RHINT_ORIGO_LL;
        Arcan_GL_SetupFBO(_this, data, ARCAN_FBOOP_CREATE);
    }
    else
        ;

    SDL_SetMouseFocus(window);
    SDL_SetKeyboardFocus(window);

    return 0;
}

void
Arcan_DestroyWindow(_THIS, SDL_Window* window)
{
    TRACE("DestroyWindow");
/*
    glDeleteFramebuffers(1, &wnd->fbo_id);
    wnd->fbo_id = GL_NONE;
    Arcan_GL_SetupFBO(_thi
*/
/* TODO: wrap to shmif_drop */
}

void
Arcan_SetWindowFullscreen(_THIS, SDL_Window* window,
                          SDL_VideoDisplay* display,
                          SDL_bool fullscreen)
{
    TRACE("SetWindowFullscreen(%d)", fullscreen);
/* not our decision */
}

void
Arcan_MaximizeWindow(_THIS, SDL_Window* window)
{
    TRACE("MaximizeWindow");
/*
 * "Maximize" / "Minimize" doesn't make sense here, the only
 * thing a client will know about the supposed display is if
 * a scanout- capable buffer is desired, the density and the
 * suggested dimensions the window will be displayed as. The
 * actual client do not control window ordering or position.
 */
}

void
Arcan_MinimizeWindow(_THIS, SDL_Window* window)
{
    TRACE("Minimize Window");
}

void
Arcan_RestoreWindow(_THIS, SDL_Window* window)
{
/* viewport hint on visible */
    TRACE("Restore Window");
}

void
Arcan_HideWindow(_THIS, SDL_Window* window)
{
/* viewport hint on invisible */
    TRACE("Hide Window");
}

SDL_bool
Arcan_GetWindowWMInfo(_THIS, SDL_Window* window, SDL_SysWMinfo* info)
{
    TRACE("GetWindowWMInfo()");
    return true;
}

void
Arcan_SetWindowSize(_THIS, SDL_Window* window)
{
/* wrap to resize event */
    TRACE("SetWindowSize()");
}

void
Arcan_SetWindowMinimumSize(_THIS, SDL_Window* window)
{
/* NOOP */
    TRACE("SetWindowMinimumSize");
}

void
Arcan_SetWindowMaximumSize(_THIS, SDL_Window* window)
{
/* NOOP */
    TRACE("SetWindowMaximumSize");
}

void
Arcan_SetWindowTitle(_THIS, SDL_Window* window)
{
    Arcan_WindowData *data = window->driverdata;
    struct arcan_event ev = {0};
    size_t lim=sizeof(ev.ext.message.data)/sizeof(ev.ext.message.data[1]);

    ev.ext.kind = ARCAN_EVENT(IDENT);
    snprintf((char*)ev.ext.message.data, lim, "%s",
             window->title ? window->title : "");
    arcan_shmif_enqueue(data->con, &ev);
}

#endif

/* vi: set ts=4 sw=4 expandtab: */
