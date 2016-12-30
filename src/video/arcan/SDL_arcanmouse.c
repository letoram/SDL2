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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>

#include "../SDL_sysvideo.h"

#include "SDL_mouse.h"
#include "../../events/SDL_mouse_c.h"
#include "SDL_arcanvideo.h"

#include "SDL_assert.h"


typedef struct {
    int hot_x, hot_y;
    int w, h;
    shmif_pixel* buffer;
    char cursor_type[64];
} Arcan_CursorData;

static SDL_Cursor* allocCursor(const char* system,
                               SDL_Surface* surf, int hot_x, int hot_y)
{
    SDL_Cursor *cursor = calloc(1, sizeof (*cursor));
    Arcan_CursorData *data = calloc (1, sizeof (Arcan_CursorData));
/*   SDL_VideoDevice *vd = SDL_GetVideoDevice(); */
/*    Arcan_SDL_Meta *wd = (Arcan_SDL_Meta *) vd->driverdata; */

    if (!cursor){
        SDL_OutOfMemory();
        return NULL;
    }

    if (!data){
        SDL_OutOfMemory();
        free(cursor);
        return NULL;
    }

    cursor->driverdata = (void *) data;

    if (surf){
    }
    else {
        snprintf(data->cursor_type, 64, "%s", system);
    }

    return cursor;
}

static SDL_Cursor *
Arcan_CreateCursor(SDL_Surface *surface, int hot_x, int hot_y)
{
    return allocCursor(NULL, surface, hot_x, hot_y);
}

static SDL_Cursor *
Arcan_CreateSystemCursor(SDL_SystemCursor id)
{

/* translate from the SDL SystemCursor id, and just set that as the cursorhint
    SDL_SYSTEM_CURSOR_ARROW
    SDL_SYSTEM_CURSOR_IBEAM
    SDL_SYSTEM_CURSOR_WAIT
    SDL_SYSTEM_CURSOR_CROSSHAIR
    SDL_SYSTEM_CURSOR_WAITARROW
    SDL_SYSTEM_CURSOR_SIZENWSE
    SDL_SYSTEM_CURSOR_SIZENESW
    SDL_SYSTEM_CURSOR_SIZEWE
    SDL_SYSTEM_CURSOR_SIZENS
    SDL_SYSTEM_CURSOR_SIZEALL
    SDL_SYSTEM_CURSOR_NO
    SDL_SYSTEM_CURSOR_HAND
 */

    return allocCursor("default", NULL, 0, 0);
}

static void
Arcan_FreeCursor(SDL_Cursor *cursor)
{
    if (!cursor || !cursor->driverdata)
        return;

    free (cursor->driverdata);
    SDL_free(cursor);
}

static int
Arcan_ShowCursor(SDL_Cursor *cursor)
{
/*
 *  SDL_VideoDevice *vd = SDL_GetVideoDevice();
    Arcan_SDL_Meta *d = vd->driverdata;
 */

    if (cursor)
    {
/* FIXME: send visibility hint for cursor segment */
    }
    else
    {
    }

    return 0;
}

static void
Arcan_WarpMouse(SDL_Window *window, int x, int y)
{
/* update the global cursor tracking */
}

static int
Arcan_WarpMouseGlobal(int x, int y)
{
    return SDL_Unsupported();
}

static int
Arcan_SetRelativeMouseMode(SDL_bool enabled)
{
/* send a hint as to what we prefer */
    return 0;
}

static Uint32
Arcan_GetGlobalMouseState(int *x, int *y)
{
    *x = 0;
    *y = 0;
    return 0;
}

void
Arcan_InitMouse(void)
{
    SDL_Mouse *mouse = SDL_GetMouse();

    mouse->CreateCursor = Arcan_CreateCursor;
    mouse->CreateSystemCursor = Arcan_CreateSystemCursor;
    mouse->ShowCursor = Arcan_ShowCursor;
    mouse->FreeCursor = Arcan_FreeCursor;
    mouse->WarpMouse = Arcan_WarpMouse;
    mouse->WarpMouseGlobal = Arcan_WarpMouseGlobal;
    mouse->SetRelativeMouseMode = Arcan_SetRelativeMouseMode;
    mouse->GetGlobalMouseState = Arcan_GetGlobalMouseState;
}

void
Arcan_FiniMouse(void)
{
    SDL_Mouse *mouse = SDL_GetMouse();

    if (mouse->def_cursor != mouse->cur_cursor)
        Arcan_FreeCursor(mouse->cur_cursor);

    Arcan_FreeCursor (mouse->def_cursor);
    mouse->def_cursor = mouse->cur_cursor = NULL;
    mouse->CreateCursor =  NULL;
    mouse->CreateSystemCursor = NULL;
    mouse->ShowCursor = NULL;
    mouse->FreeCursor = NULL;
    mouse->WarpMouse = NULL;
    mouse->SetRelativeMouseMode = NULL;
}
#endif
/* vi: set ts=4 sw=4 expandtab: */
