#include "../../SDL_internal.h"

#if SDL_AUDIO_DRIVER_ARCAN
/* Allow access to a raw mixing buffer */

#include <stdio.h>              /* For perror() */
#include <string.h>             /* For strerror() */
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <arcan_shmif.h>

#include "SDL_timer.h"
#include "SDL_audio.h"
#include "SDL_arcanaudio.h"
#include "SDL_video.h"
#include "SDL_stdinc.h"
#include "../../video/arcan/SDL_arcanvideo.h"
#include "../SDL_audiomem.h"
#include "../SDL_audio_c.h"
#include "../SDL_audiodev_c.h"

static void
Arcan_DetectDevices(void)
{
}

static void
Arcan_CloseDevice(_THIS)
{
    struct arcan_shmif_cont *acont = arcan_shmif_primary(SHMIF_INPUT);
    Arcan_SDL_Meta *cont = (Arcan_SDL_Meta*) acont->user;
    int refc;

    /* either audio or video or "other" can be last */
    SDL_LockMutex(cont->av_sync);
    cont->refc--;
    refc = cont->refc;
    SDL_UnlockMutex(cont->av_sync);

    if (0 == refc){
        SDL_DestroyMutex(cont->av_sync);
        arcan_shmif_drop(acont);
        arcan_shmif_setprimary(SHMIF_INPUT, NULL);
        SDL_free(cont);
    }

    SDL_free(this->hidden->mixbuf);
    this->hidden = NULL;
}

static int
Arcan_OpenDevice(_THIS, void *handle, const char *devname, int iscapture)
{
    Arcan_SDL_Meta *cont;
    struct shmif_resize_ext ext;
    struct arcan_shmif_cont *shmcont;

    if (!getenv("ARCAN_CONNPATH")){
        return SDL_SetError("No arcan connection (ARCAN_CONNPATH env.)");
    }

    /* any capture device will come as a hotplug event and we maintain a
     * separate queue for those, remain a fixme until we have a good test case */
    if (iscapture){
        return SDL_SetError("No capture device available");
    }

    /* this depends on initialization order, if video comes first, we already
     * have a primary segment ready to use (and we do no audio on the secondary
     * ones) */
    if (!arcan_shmif_primary(SHMIF_INPUT)){
        if (0 != arcan_av_setup_primary()){
            return SDL_SetError("Couldn't setup av- segment");
        }
    }

    shmcont = arcan_shmif_primary(SHMIF_INPUT);
    cont = (Arcan_SDL_Meta*) shmcont->user;
    SDL_LockMutex(cont->av_sync);
    cont->refc++;
    SDL_UnlockMutex(cont->av_sync);

    /* we only support one spec */
    this->hidden = (struct SDL_PrivateAudioData *)
        SDL_malloc((sizeof *this->hidden));
    if (this->hidden == NULL) {
        return SDL_OutOfMemory();
    }
    SDL_memset(this->hidden, 0, (sizeof *this->hidden));
    this->spec.channels= ARCAN_SHMIF_ACHANNELS;
    this->spec.format = AUDIO_S16LSB;
    this->spec.freq = ARCAN_SHMIF_SAMPLERATE;
    SDL_CalculateAudioSpec(&this->spec);

    /* Unfortunately we need an intermediate buffer here as we never know
     * when / if the video- window decides to resize - invalidating the audp */
    this->hidden->mixlen = this->spec.size;
    this->hidden->mixbuf = (Uint8 *) SDL_AllocAudioMem(this->hidden->mixlen);
    SDL_memset(this->hidden->mixbuf, this->spec.silence, this->spec.size);

    SDL_LockMutex(cont->av_sync);

    /* Still no guarantee that we'll get this size, so it is not safe to
     * assume that we do. Buffering parameters can be adjusted at resize
     * and is controlled server-side */
    ext.abuf_sz = this->hidden->mixlen;
    ext.abuf_cnt = 65536 / ext.abuf_sz;
    ext.vbuf_cnt = -1;
    arcan_shmif_resize_ext(shmcont, shmcont->w, shmcont->h, ext);
    SDL_UnlockMutex(cont->av_sync);
    return 0;
}

static void
Arcan_PlayDevice(_THIS)
{
    Arcan_SDL_Meta *cont = (Arcan_SDL_Meta*)
                           (arcan_shmif_primary(SHMIF_INPUT)->user);

    struct SDL_PrivateAudioData *adata = (struct SDL_PrivateAudioData *)
                                         this->hidden;

    size_t left_in = adata->mixlen;
    uint8_t *cur = adata->mixbuf;

    if (!cont){
        return;
    }

    /* the video driver gets to be the 'main thread', so it is only during
     * signalling we need some protection against a resize- being called */

    SDL_LockMutex(cont->av_sync);
    while (left_in){
        size_t ntc = (cont->mcont.abufsize - cont->mcont.abufused) < left_in ?
                     (cont->mcont.abufsize - cont->mcont.abufused) : left_in;

        printf("audio ntc: %zu, size: %zu\n", ntc, cont->mcont.abufsize);
        memcpy(&((uint8_t*)cont->mcont.audp)[cont->mcont.abufused], cur, ntc);
        cont->mcont.abufused += ntc;
        left_in -= ntc;
        cur += ntc;
        if (cont->mcont.abufused == cont->mcont.abufsize){
            printf("signal transfer\n");
            arcan_shmif_signal(&cont->mcont, SHMIF_SIGAUD);
            printf("done signalling\n");
        }
    }
    SDL_UnlockMutex(cont->av_sync);
}

static Uint8 *
Arcan_GetDeviceBuf(_THIS)
{
    return (this->hidden->mixbuf);
}

static int
Arcan_Init(SDL_AudioDriverImpl *impl)
{
    impl->DetectDevices = Arcan_DetectDevices;
    impl->OpenDevice = Arcan_OpenDevice;
    impl->PlayDevice = Arcan_PlayDevice;
    impl->GetDeviceBuf = Arcan_GetDeviceBuf;
    impl->CloseDevice = Arcan_CloseDevice;
    impl->OnlyHasDefaultOutputDevice = 1;
    impl->AllowsArbitraryDeviceNames = 1;

    return 1;
}

/*
 * This is very similar to DSP audio since we can't directly mix into
 * the normal arcan buffer due to the risk of aliasing and video resizes
 * affecting buffer pointers.
 */
AudioBootStrap ARCANAUDIO_bootstrap = {
    "arcan", "Arcan audio driver", Arcan_Init, 0
};

#endif /* SDL_AUDIO_DRIVER_ARCAN  */

/* vi: set ts=4 sw=4 expandtab: */
