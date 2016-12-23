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
 * Contributed by Bjorn Stahl, <contact@arcan-fe.com>
*/

#ifndef _SDL_arcanvideo_h_
#define _SDL_arcanvideo_h_

#include <EGL/egl.h>
#define WANT_ARCAN_SHMIF_HELPER
#include <arcan_shmif.h>
#include "SDL_mutex.h"

/*
 * This is shared between audio and video implementations as any negotiated
 * connection support both, and some operations on the connection need
 * synchronization (i.e. resize)
 */
typedef struct {
    SDL_Window *main;
    SDL_mutex* av_sync;
    int refc;
    int mx, my, mrel;
    bool dirty_mouse;
    size_t n_windows;
    struct arcan_shmif_cont clip_in, clip_out, popup, cursor, mcont;
    uint8_t wndalloc;
    struct arcan_shmif_cont windows[8];
} Arcan_SDL_Meta;

typedef struct {
    int index;
    struct arcan_shmif_cont *con;
    struct arcan_event* pqueue;
    ssize_t pqueue_sz;
} Arcan_WindowData;

/*
 * Since we have no guarantee that audio or video will be created first,
 * we rely on a shared implementation and whoever gets there first sets
 * things up
 */
extern int arcan_av_setup_primary();

#endif /* _SDL_arcanvideo_h_ */

/* vi: set ts=4 sw=4 expandtab: */
