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
#include "SDL_arcanevent.h"

#define TRACE(...)
//#define TRACE(...) {fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n");}

int
Arcan_CreateWindow(_THIS, SDL_Window* window)
{
    Arcan_SDL_Meta *meta = _this->driverdata;
    Arcan_WindowData *data = SDL_calloc(sizeof(Arcan_WindowData), 1);
    TRACE("Create Window");
    if (!data)
        return SDL_SetError("Out of Memory");

    if (meta->main){
        int index = 0;
        struct arcan_event acqev = {
            .category = EVENT_EXTERNAL,
            .ext.kind = ARCAN_EVENT(SEGREQ),
            .ext.segreq.width = window->w,
            .ext.segreq.height = window->h,
            .ext.segreq.kind = SEGID_GAME,
            .ext.segreq.id = 0xfeedface
        };

        if (meta->wndalloc == 255){
            return SDL_SetError("Out of Memory");
        }
        while (meta->wndalloc & (1 << index))
            index++;

        arcan_shmif_enqueue(&meta->mcont, &acqev);
/* FIXME: we must properly flush the pqueue in the event handler */
        if (arcan_shmif_acquireloop(&meta->mcont,
                                    &acqev, &meta->pqueue, &meta->pqueue_sz)){
            data->con = &meta->windows[index];
            *(data->con) = arcan_shmif_acquire(&meta->mcont,NULL,SEGID_GAME, 0);
            meta->wndalloc |= 1 << index;
            if (SDL_GetRelativeMouseMode()){
                arcan_shmif_enqueue(data->con, &(struct arcan_event){
                    .ext.kind = ARCAN_EVENT(CURSORHINT),
                    .ext.message.data = "hidden-rel"
                });
            }
        }
        else {
            if (!meta->pqueue){
                SDL_free(data);
                return SDL_SetError("Out of Memory");
            }
            if (meta->pqueue_sz < 0){ /* game over */
                SDL_free(data);
                return SDL_SetError("Shmif- state inconsistent\n");
            }
            return SDL_SetError("Arcan rejected window request\n");
        }
        Arcan_PumpEvents(_this);
    }
    else {
        meta->main = window;
        data->con = &meta->mcont;
    }

    window->driverdata = data;
    window->x = 0;
    window->y = 0;
    window->flags &= ~SDL_WINDOW_FULLSCREEN;

    if (window->flags & SDL_WINDOW_OPENGL){
        data->con->hints = SHMIF_RHINT_ORIGO_LL;
        arcan_shmif_resize(data->con, window->w, window->h);
        arcan_shmifext_setup(data->con, Arcan_GL_cfg(_this, window));
    }
    else{
        data->con->hints = 0;
        arcan_shmifext_drop(data->con);
        arcan_shmif_resize(data->con, window->w, window->h);
    }
    window->w = data->con->w;
    window->h = data->con->h;
    data->disp_w = meta->disp_w;
    data->disp_h = meta->disp_h;

    return 0;
}

void
Arcan_DestroyWindow(_THIS, SDL_Window* window)
{
    Arcan_SDL_Meta *meta = _this->driverdata;
    Arcan_WindowData *data = window->driverdata;
    TRACE("DestroyWindow");

    arcan_shmifext_drop(data->con);

/* FIXME: send viewport hint to hide main connection */
    if (meta->main == window){
        meta->main = NULL;
    }
    else {
    /* only need to clear pqueue on the mcont */
        if (data){
            meta->wndalloc &= ~(1 << data->index);
            arcan_shmif_drop(data->con);
            data->con = NULL;
        }
    }

    SDL_free(window->driverdata);
    window->driverdata = NULL;
}

void
Arcan_SetWindowFullscreen(_THIS, SDL_Window* window,
                          SDL_VideoDisplay* display,
                          SDL_bool fullscreen)
{
/*
 * Not our decision, we can try a viewport hint.  Some games actually track
 * this though, and repeatedly check displayBounds to see if we match.
 */
    Arcan_WindowData *data = window->driverdata;
    TRACE("SetWindowFullscreen(%d)", fullscreen);
    arcan_shmif_resize(data->con, data->disp_w, data->disp_h);
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
