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
  Contributed by Bjorn Stahl <contact@arcan-fe.com>
*/

#include "../../SDL_internal.h"

#if SDL_VIDEO_DRIVER_ARCAN

#include "SDL_arcangl.h"

#include "SDL_arcandyn.h"

void
ARCAN_GL_SwapWindow(_THIS, SDL_Window* window)
{
    ARCAN_Window* wnd = window->driverdata;

    SDL_EGL_SwapBuffers(_this, wnd->surface);
/* TODO: run signal-handle here */
}

int
ARCAN_GL_MakeCurrent(_THIS, SDL_Window* window, SDL_GLContext context)
{
    return SDL_EGL_MakeCurrent(_this,
        window ? ((ARCAN_Window*)window->driverdata)->surface : NULL, context);
}

SDL_GLContext
ARCAN_GL_CreateContext(_THIS, SDL_Window* window)
{
    ARCAN_Window* wnd = window->driverdata;

    SDL_GLContext context;
    context = SDL_EGL_CreateContext(_this, wnd->surface);

    return context;
}

int
ARCAN_GL_LoadLibrary(_THIS, const char* path)
{
/* TODO: what is actually needed here */
    SDL_EGL_LoadLibrary(_this, path, NULL);

    SDL_EGL_ChooseConfig(_this);

    return 0;
}

void
ARCAN_GL_UnloadLibrary(_THIS)
{
    SDL_EGL_UnloadLibrary(_this);
}

void*
ARCAN_GL_GetProcAddress(_THIS, const char* proc)
{
    void* proc_addr = SDL_EGL_GetProcAddress(_this, proc);

    if (!proc_addr) {
        SDL_SetError("Failed to find proc address!");
    }

    return proc_addr;
}

#endif /* SDL_VIDEO_DRIVER_MIR */

/* vi: set ts=4 sw=4 expandtab: */
