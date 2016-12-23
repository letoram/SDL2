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
#include "../../render/SDL_sysrender.h"
#include <stdio.h>

#if SDL_VIDEO_DRIVER_ARCAN && SDL_VIDEO_OPENGL_EGL
#define TRACE(...)
//#define TRACE(...) {fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n");}

#include "SDL_arcanopengl.h"
#include "SDL_arcanvideo.h"
#include "SDL_opengl.h"

/*
 * only GL functions we need
 */
typedef void (*PFNGLFLUSHPROC)();
static PFNGLBINDFRAMEBUFFERPROC glocBindFramebuffer;
static PFNGLFLUSHPROC glocFlush;

static struct arcan_shmif_cont* current;

/*
 * Since we rely on a surfaceless context, we NEED the caller to
 * render into an FBO where we control the color attachment. Havn't
 * found a good place in SDL to enforce this, hence why we override
 * the BindFramebuffer call.
 *
 * Judging by the ios backend, which has similar behavior, the other
 * option is to have one FBO and swap out the color attachments. Both
 * strategies should probably be added to shmif (builtin_fbo modes)
 */
void
Arcan_GL_SwapWindow(_THIS, SDL_Window* window)
{
    Arcan_WindowData* wnd = window->driverdata;

    glocFlush();

    arcan_shmifext_signal(wnd->con, 0, SHMIF_SIGVID, SHMIFEXT_BUILTIN);
}

/* shmifext links to the related EGL/GL library already */
int
Arcan_EGL_LoadLibrary(_THIS, const char *path)
{
    Arcan_WindowData *wnd = _this->driverdata;

/*    if (!arcan_shmifext_egl(arcan_shmif_primary(SHMIF_INPUT),
        &display, (void*) arcan_shmifext_lookup, NULL))
        return -1;
*/
    _this->gl_config.driver_loaded = 1;

    if (SDL_GL_LoadLibrary(NULL) < 0){
        return -1;
    }

#define LOOKUP(X) arcan_shmifext_lookup(wnd->con, X);
   glocFlush = LOOKUP("glFlush");
#undef LOOKUP

    return 0;
}

void
Arcan_EGL_UnloadLibrary(_THIS)
{
    SDL_EGL_UnloadLibrary(_this);
}

/*
 * This is an unfortunate thing, we mimic the (iOS?) behavior of requesting
 * that the GL clients use displayless surfaces, and that we provide a FBO
 * for them to render into. Digging through the SDL source, there seem to be
 * no surefire- way to enforce this, so we have to intercept BindFramebuffer
 * calls and replace the 'default' with the one we have.
 */
static void redirectFBO(GLint tgt, GLint fbo)
{
    printf("bind framebuffer: %d\n", fbo);
    if (0 == fbo){
        arcan_shmifext_make_current(current);
        return;
    }

    TRACE("bind framebuffer: %d", fbo);
    glocBindFramebuffer(tgt, fbo);
}

void*
Arcan_EGL_GetProcAddress(_THIS, const char *proc)
{
    Arcan_WindowData* wnd = _this->driverdata;
    void* ret;

    if (!proc || strlen(proc) == 0)
        return NULL;

    if (strcmp(proc, "glBindFramebuffer") == 0 ||
        strcmp(proc, "glBindFramebufferARB") == 0 ||
        strcmp(proc, "glBindFramebufferEXT") == 0)
        return redirectFBO;

    ret = arcan_shmifext_lookup(wnd->con, proc);
    if (!ret){
        TRACE("arcan lookup fail on (%s)", proc);
    }

    return ret;
}

SDL_GLContext
Arcan_EGL_CreateContext(_THIS, SDL_Window* window)
{
    Arcan_WindowData* wnd = window->driverdata;
    struct arcan_shmifext_setup defs = arcan_shmifext_defaults(wnd->con);
    defs.red = _this->gl_config.red_size;
    defs.green = _this->gl_config.green_size;
    defs.blue = _this->gl_config.blue_size;
    defs.stencil = _this->gl_config.stencil_size;
    defs.depth = _this->gl_config.depth_size;
    defs.major = _this->gl_config.major_version;
    defs.minor = _this->gl_config.minor_version;
    defs.builtin_fbo = 2;
    defs.api = _this->gl_config.profile_mask == SDL_GL_CONTEXT_PROFILE_ES ?
        API_GLES : API_OPENGL;
    arcan_shmifext_setup(wnd->con, defs);
    arcan_shmifext_make_current(wnd->con);
    glocBindFramebuffer = arcan_shmifext_lookup(wnd->con, "glBindFramebuffer");
    return (SDL_GLContext) wnd;
}

void
Arcan_EGL_DeleteContext(_THIS, SDL_GLContext context)
{
    Arcan_WindowData* wnd = (Arcan_WindowData*) context;
    TRACE("Delete Context\n");
    arcan_shmifext_drop_context(wnd->con);
}

int
Arcan_EGL_MakeCurrent(_THIS, SDL_Window* window, SDL_GLContext context)
{
    Arcan_WindowData *wnd = (Arcan_WindowData*) context;
    if (!wnd)
        return SDL_SetError("bad context");

    arcan_shmifext_make_current(wnd->con);
    return 0;
}
#endif /* SDL_VIDEO_DRIVER_Arcan */

/* vi: set ts=4 sw=4 expandtab: */
