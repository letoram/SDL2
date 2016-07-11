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

#if SDL_VIDEO_ARCAN

#include "../SDL_egl_c.h"
#include "../SDL_sysvideo.h"
#include "arcanwindow.h"

int
ARCAN_CreateWindow(_THIS, SDL_Window* window)
{
/* TODO if #window == 0, ok, else request subsegment
 * and wait-poll-queue */
    return 0;
}

void
ARCAN_DestroyWindow(_THIS, SDL_Window* window)
{
/* TODO: wrap to shmif_drop */
}

ARCAN_SetWindowFullscreen(_THIS, SDL_Window* window,
                          SDL_VideoDisplay* display,
                          SDL_bool fullscreen)
{
/* not our decision */
}

void
ARCAN_MaximizeWindow(_THIS, SDL_Window* window)
{
}

void
ARCAN_MinimizeWindow(_THIS, SDL_Window* window)
{
}

void
ARCAN_RestoreWindow(_THIS, SDL_Window* window)
{
/* viewport hint on visible */
}

void
ARCAN_HideWindow(_THIS, SDL_Window* window)
{
/* viewport hint on invisible */
}

SDL_bool
ARCAN_GetWindowWMInfo(_THIS, SDL_Window* window, SDL_SysWMinfo* info)
{
    return false;
}

void
ARCAN_SetWindowSize(_THIS, SDL_Window* window)
{
/* wrap to resize event */
}

void
ARCAN_SetWindowMinimumSize(_THIS, SDL_Window* window)
{
/* NOOP */
}

void
ARCAN_SetWindowMaximumSize(_THIS, SDL_Window* window)
{
/* NOOP */
}

void
ARCAN_SetWindowTitle(_THIS, SDL_Window* window)
{
/* TODO: just encode ident event */
}

#endif

/* vi: set ts=4 sw=4 expandtab: */
