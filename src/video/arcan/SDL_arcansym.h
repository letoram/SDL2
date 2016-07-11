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

/* *INDENT-OFF* */

#ifndef SDL_ARCAN_MODULE
#define SDL_ARCAN_MODULE(modname)
#endif

#ifndef SDL_ARCAN_SYM
#define SDL_ARCAN_SYM(rc,fn,params)
#endif

#ifndef SDL_ARCAN_SYM_CONST
#define SDL_ARCAN_SYM_CONST(type, name)
#endif

SDL_ARCAN_MODULE(ARCAN_CLIENT)
/* TODO: just map shmif- symbols from here iwth
 * SDL_ARCAN_SYM and SDL_ARCAN_SYM_CONST */

#undef SDL_ARCAN_MODULE
#undef SDL_ARCAN_SYM
#undef SDL_ARCAN_SYM_CONST

/* *INDENT-ON* */

/* vi: set ts=4 sw=4 expandtab: */
