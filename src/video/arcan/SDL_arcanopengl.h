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

#ifndef _SDL_arcangl_h
#define _SDL_arcangl_h

#include "SDL_arcanwindow.h"

#include "../SDL_egl_c.h"

#define Arcan_EGL_GetSwapInterval SDL_EGL_GetSwapInterval
#define Arcan_EGL_SetSwapInterval SDL_EGL_SetSwapInterval

extern int
Arcan_GL_SwapWindow(_THIS, SDL_Window* window);

enum arcan_fboop {
	ARCAN_FBOOP_CREATE,
	ARCAN_FBOOP_DESTROY,
	ARCAN_FBOOP_RESIZE
};

extern
struct arcan_shmifext_setup Arcan_GL_cfg(_THIS, SDL_Window* window);

extern void
Arcan_GL_SetupFBO(_THIS, Arcan_WindowData* window, enum arcan_fboop);

extern void
Arcan_EGL_DeleteContext(_THIS, SDL_GLContext context);

extern int
Arcan_EGL_MakeCurrent(_THIS, SDL_Window* window, SDL_GLContext context);

extern SDL_GLContext
Arcan_EGL_CreateContext(_THIS, SDL_Window* window);

extern int
Arcan_EGL_LoadLibrary(_THIS, const char* path);

extern void
Arcan_EGL_UnloadLibrary(_THIS);

extern void*
Arcan_EGL_GetProcAddress(_THIS, const char* proc);

#endif /* _SDL_arcangl_h */

/* vi: set ts=4 sw=4 expandtab: */

