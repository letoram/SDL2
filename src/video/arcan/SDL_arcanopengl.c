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
//#define TRACE(...) {fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n");}

#include "SDL_arcanopengl.h"
#include "SDL_arcanvideo.h"
#include "SDL_opengl.h"

/*
 * only GL functions we need
 */
typedef void (*PFNGLFLUSHPROC)();
static PFNGLBINDFRAMEBUFFERPROC glocBindFramebuffer;
void (*glocGetIntegerv)(GLenum, GLint*);
static PFNGLFLUSHPROC glocFlush;

static struct arcan_shmif_cont* current;

/*
 * Since we rely on a surfaceless context, we NEED the caller to
 * render into an FBO where we control the color attachment. Havn't
 * found a good place in SDL to enforce this, hence why we override
 * the BindFramebuffer call.
 *
 * Though somewhat slower than swapping FBOs around, some games cache
 * the FBO bound before they get control, so we will have to settle for
 * swapping color attachment (shmif builtin_fbo can do both)
 */
int
Arcan_GL_SwapWindow(_THIS, SDL_Window* window)
{
    Arcan_WindowData* wnd = window->driverdata;

    glocFlush();
    arcan_shmifext_signal(wnd->con, 0, SHMIF_SIGVID, SHMIFEXT_BUILTIN);
    arcan_shmifext_bind(wnd->con);
    current = wnd->con;
    return 0;
}

/* shmifext links to the related EGL/GL library already */
int
Arcan_EGL_LoadLibrary(_THIS, const char *path)
{
    _this->gl_config.driver_loaded = 1;

    if (SDL_GL_LoadLibrary(NULL) < 0){
        return -1;
    }

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
    TRACE("bind framebuffer: (%d:%d)", tgt, fbo);
    if (0 == fbo){
        arcan_shmifext_bind(current);
        return;
    }
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

struct arcan_shmifext_setup Arcan_GL_cfg(_THIS, SDL_Window* window)
{
    Arcan_WindowData* wnd = window->driverdata;
    struct arcan_shmifext_setup defs = arcan_shmifext_defaults(wnd->con);
    defs.red = _this->gl_config.red_size;
    defs.green = _this->gl_config.green_size;
    defs.blue = _this->gl_config.blue_size;
    defs.stencil = _this->gl_config.stencil_size;
    defs.builtin_fbo = 3;
    defs.depth = _this->gl_config.depth_size;
    defs.major = _this->gl_config.major_version;
    defs.minor = _this->gl_config.minor_version;
    defs.api = _this->gl_config.profile_mask == SDL_GL_CONTEXT_PROFILE_ES ?
        API_GLES : API_OPENGL;
    return defs;
}

struct GLContext {
    struct arcan_shmif_cont* con;
    Arcan_WindowData* wnd;
    unsigned ind;
};

SDL_GLContext
Arcan_EGL_CreateContext(_THIS, SDL_Window* window)
{
    Arcan_WindowData* wnd = window->driverdata;
    struct GLContext* context = SDL_calloc(sizeof(struct GLContext), 1);
    TRACE("CreateContext");

    if (!context)
        return NULL;

#define LOOKUP(X) arcan_shmifext_lookup(wnd->con, X);
    glocBindFramebuffer = LOOKUP("glBindFramebuffer");
    glocFlush = LOOKUP("glFlush");
    glocGetIntegerv = LOOKUP("glocGetIntegerv");
#undef LOOKUP

    context->con = wnd->con;
    context->wnd = wnd;
    if (!wnd->got_context){
        wnd->got_context = true;
        context->ind = 1;
    }
    else
        context->ind = arcan_shmifext_add_context(wnd->con,
                                                  Arcan_GL_cfg(_this, window));
    return context;
}

void
Arcan_EGL_DeleteContext(_THIS, SDL_GLContext context)
{
    TRACE("Delete Context\n");
    if (context){
        struct GLContext* glctx = (struct GLContext*) context;

/* Special case, in Arcan, converting to an accelerated connection implies
 * creating a context - they are not separate. It's possible to create new
 * ones and attach/swap but there's the one. SDL does some fugly probing by
 * creating and destroying contexts, which affects the 'builtin-' one. */

        if (glctx->ind == 1){
            glctx->wnd->got_context = false;
        }
        else {
            arcan_shmifext_swap_context(glctx->con, glctx->ind);
            arcan_shmifext_drop_context(glctx->con);
        }

        SDL_free(glctx);
    }
}

int
Arcan_EGL_MakeCurrent(_THIS, SDL_Window* window, SDL_GLContext context)
{
    struct GLContext* glctx = (struct GLContext*) context;
    if (glctx){
        arcan_shmifext_swap_context(glctx->con, glctx->ind);
        arcan_shmifext_make_current(glctx->con);

        current = glctx->con;
    }
    return 0;
}
#endif /* SDL_VIDEO_DRIVER_Arcan */

/* vi: set ts=4 sw=4 expandtab: */
