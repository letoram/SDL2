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

/* There is a similar mechanism in SDL_glfuncs, but the definitions
 * we want are not all there anyhow, and the only other video driver that
 * use this approach is the uikit one, where the GL environment is sane */
typedef void (*PFNGLGETTEXIMAGEPROC)(GLenum, GLint, GLenum, GLenum, GLvoid*);
typedef void (*PFNGLBINDTEXTUREPROC)(GLenum, GLuint);
typedef void (*PFNGLGENTEXTURESPROC)(GLsizei n, GLuint *);
typedef void (*PFNGLTEXIMAGE2DPROC)(GLenum, GLint, GLint, GLsizei, GLsizei,
                                    GLint, GLenum, GLenum, const GLvoid *);
typedef void (*PFNGLTEXPARAMETERIPROC)(GLenum, GLenum, GLint);
typedef void (*PFNGLFLUSHPROC)();

static PFNGLGENFRAMEBUFFERSPROC glocGenFramebuffers;
static PFNGLBINDFRAMEBUFFERPROC glocBindFramebuffer;
static PFNGLCHECKFRAMEBUFFERSTATUSPROC glocCheckFramebufferStatus;
static PFNGLFRAMEBUFFERTEXTURE2DPROC glocFramebufferTexture2D;
static PFNGLBINDRENDERBUFFERPROC glocBindRenderbuffer;
static PFNGLRENDERBUFFERSTORAGEPROC glocRenderbufferStorage;
static PFNGLFRAMEBUFFERRENDERBUFFERPROC glocFramebufferRenderbuffer;
/*
 * static PFNGLDELETEFRAMEBUFFERSPROC glocDeleteFramebuffers;
static PFNGLDELETERENDERBUFFERSPROC glocDeleteRenderbuffers;
 */
static PFNGLGENRENDERBUFFERSPROC glocGenRenderbuffers;
static PFNGLGETINTEGERI_VPROC glocGetIntegeri_v;
static PFNGLGETTEXIMAGEPROC glocGetTexImage;
static PFNGLBINDTEXTUREEXTPROC glocBindTexture;
static PFNGLGENTEXTURESPROC glocGenTextures;
static PFNGLTEXIMAGE2DPROC glocTexImage2D;
static PFNGLTEXPARAMETERIPROC glocTexParameteri;
static PFNGLFLUSHPROC glocFlush;

static int arcan_gl_fbo;

/*
 * Synchronization is similar to how iOS treats it, the client MUST
 * render to an offscreen buffer (KHR_Surfaceless_Context) and the
 * texture of the final color buffer is what we pass on to Arcan.
 */
void
Arcan_GL_SwapWindow(_THIS, SDL_Window* window)
{
    Arcan_WindowData* wnd = window->driverdata;
    uintptr_t display;
    GLint id;

    glocFlush();

/* this window has no working GL setup */
    if (!arcan_shmifext_egl_meta(wnd->con, &display, NULL, NULL))
        return;

    if (arcan_shmifext_eglsignal(wnd->con, display,
        SHMIF_SIGVID, wnd->tex_id) >= 0)
       return;

/* we can't pass textures accelerated, have to do it the slow way */
    glocGetIntegeri_v(GL_TEXTURE_BINDING_2D, 1, &id);
    glocBindTexture(GL_TEXTURE_2D, wnd->tex_id);
	glocGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA,
                  GL_UNSIGNED_BYTE, (void*) wnd->con->vidp);
	glocBindTexture(GL_TEXTURE_2D, id);

    arcan_shmif_signal(wnd->con, SHMIF_SIGVID);
}

void
Arcan_GL_SetupFBO(_THIS, Arcan_WindowData* window, enum arcan_fboop op)
{
/* FIXME: if the values are already there, drop the current fbo */
    GLenum status;
    if (op == ARCAN_FBOOP_DESTROY){
        window->tex_id = GL_NONE;
        window->fbo_id = GL_NONE;
        window->rbuf_id = GL_NONE;
        return;
    }

/* stup empty texture backing store */
	glocGenTextures(1, &window->tex_id);
    glocBindTexture(GL_TEXTURE_2D, window->tex_id);
    glocTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, window->con->w, window->con->h,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glocTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glocTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glocBindTexture(GL_TEXTURE_2D, 0);

/* bind it to a new FBO */
	glocGenFramebuffers(1, &window->fbo_id);
    glocBindFramebuffer(GL_FRAMEBUFFER, window->fbo_id);
    glocFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D, window->tex_id, 0);
    glocGenRenderbuffers(1, &window->rbuf_id);
    glocBindRenderbuffer(GL_RENDERBUFFER, window->rbuf_id);
    glocRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
        window->con->w, window->con->h);
    glocFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
        GL_RENDERBUFFER, window->rbuf_id);

    status = glocCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        SDL_SetError("Incomplete Framebuffer Attachment (Arcan)");
}

/* shmifext links to the related EGL/GL library already */
int
Arcan_EGL_LoadLibrary(_THIS, const char *path)
{
    Arcan_WindowData *wnd = _this->driverdata;
    void* display;

    if (!arcan_shmifext_headless_egl(arcan_shmif_primary(SHMIF_INPUT),
        &display, (void*) arcan_shmifext_headless_lookup, NULL))
        return -1;

    _this->gl_config.driver_loaded = 1;

    if (SDL_GL_LoadLibrary(NULL) < 0){
        return -1;
    }

#define LOOKUP(X) arcan_shmifext_headless_lookup(wnd->con, X);
    glocGenFramebuffers = LOOKUP("glGenFramebuffers");
    glocBindFramebuffer = LOOKUP("glBindFramebuffer");
    glocFramebufferTexture2D = LOOKUP("glFramebufferTexture2D");
	glocCheckFramebufferStatus = LOOKUP("glCheckFramebufferStatus");
    glocBindRenderbuffer = LOOKUP("glBindRenderbuffer");
    glocRenderbufferStorage = LOOKUP("glRenderbufferStorage");
    glocFramebufferRenderbuffer = LOOKUP("glFramebufferRenderbuffer");
    glocGenRenderbuffers = LOOKUP("glGenRenderbuffers");
    glocGetIntegeri_v = LOOKUP("glGetIntegeri_v");
    glocGetTexImage = LOOKUP("glGetTexImage");
    glocBindTexture = LOOKUP("glBindTexture");
    glocGenTextures = LOOKUP("glGenTextures");
    glocTexImage2D = LOOKUP("glTexImage2D");
    glocTexParameteri = LOOKUP("glTexParameteri");
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
    if (0 == fbo){
        fbo = arcan_gl_fbo;
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

    ret = arcan_shmifext_headless_lookup(wnd->con, proc);
    if (!ret){
        TRACE("arcan lookup fail on (%s)", proc);
    }

    return ret;
}

SDL_GLContext
Arcan_EGL_CreateContext(_THIS, SDL_Window* window)
{
    Arcan_WindowData* wnd = window->driverdata;
    uintptr_t context;
    TRACE("Create Context");

    if (!arcan_shmifext_egl_meta(wnd->con, NULL, NULL, &context)){
        TRACE("no context for window, create one");
        return NULL;
    }

    return (SDL_GLContext) context;
}

void
Arcan_EGL_DeleteContext(_THIS, SDL_GLContext context)
{
/*
 * here we probably need to check driver-data and see if the context
 * match, if it does, don't drop it as it's our primary connection
 */
    TRACE("Delete Context\n");
}

int
Arcan_EGL_MakeCurrent(_THIS, SDL_Window* window, SDL_GLContext context)
{
    Arcan_WindowData *wnd = _this->driverdata;
    uintptr_t display, surface;
    TRACE("Make Current\n");

    if (window)
        wnd = window->driverdata;

/* this window has no working GL setup */
    if (!arcan_shmifext_egl_meta(wnd->con, &display, &surface, NULL))
        return -1;

    glocBindFramebuffer(GL_FRAMEBUFFER, wnd->fbo_id);
    arcan_gl_fbo = wnd->fbo_id;

/*
    eglMakeCurrent((EGLDisplay) display, (EGLSurface) surface,
                   (EGLSurface) surface, context);
 */

    return 0;
}
#endif /* SDL_VIDEO_DRIVER_Arcan */

/* vi: set ts=4 sw=4 expandtab: */
