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
#include "../../events/SDL_events_c.h"
#include "../../events/SDL_keyboard_c.h"
#include "../../events/SDL_touch_c.h"
#include "../../events/SDL_events_c.h"

#include "SDL_arcanwindow.h"
#include "SDL_video.h"
#include "SDL_timer.h"
#include "SDL_arcanevent.h"

#ifdef __LINUX__
#include "../../events/scancodes_linux.h"
#endif

static inline void process_mouse(struct arcan_shmif_cont *prim,
                                 struct arcan_shmif_cont *cur,
                                 SDL_Window *wnd,
                                 Arcan_SDL_Meta *meta,
                                 arcan_ioevent ev)
{
/* many packing options here, as we have the full set of:
 * x/y split into two events, relative dominant, relative desired
 * x/y split into two events, non-relative dominant, relative desired
 * x/y merged, non-relative dominant, non-relative desired
 * x/y merged, relative dominant, relative desired
 * x/y merged, non-relative dominant, relative desired
 * x/y merged, non-relative dominant, non-relative desired
 */
    if (ev.datatype == EVENT_IDATATYPE_ANALOG){
/* x/y merged, both absolute and tracked relative */
        if (!meta->dirty_mouse){
            meta->mx = 0;
            meta->my = 0;
        }
        meta->dirty_mouse = true;
        meta->mrel = ev.input.analog.gotrel;

        if (ev.subid == 2){
            if (meta->mrel){
                meta->mx += ev.input.analog.axisval[0];
                meta->my += ev.input.analog.axisval[2];
            }
            else {
                meta->mx = ev.input.analog.axisval[0];
                meta->my = ev.input.analog.axisval[2];
            }
        }
/* inefficient fallbacks */
        else{
            if (ev.input.analog.gotrel){
                if (ev.subid == 0){
                    meta->mx += ev.input.analog.axisval[0];
                }
                else if (ev.subid == 1){
                    meta->my += ev.input.analog.axisval[0];
                }
            }
            else {
                if (ev.subid == 0){
                    meta->mx = ev.input.analog.axisval[0];
                }
                else if (ev.subid == 1){
                    meta->my = ev.input.analog.axisval[0];
                }
            }
        }
    }
    else if (ev.datatype == EVENT_IDATATYPE_DIGITAL){
        if (meta->dirty_mouse){
            printf("sendMouse: %d, %d\n", meta->mx, meta->my);
            SDL_SendMouseMotion(wnd, 0, meta->mrel, meta->mx, meta->my);
            meta->dirty_mouse = false;
        }

        switch(ev.subid){
        case MBTN_LEFT_IND:
            SDL_SendMouseButton(wnd, ev.devid,
                                ev.input.digital.active, SDL_BUTTON_LEFT);
        break;
        case MBTN_RIGHT_IND:
            SDL_SendMouseButton(wnd, ev.devid,
                                ev.input.digital.active, SDL_BUTTON_RIGHT);
        break;
        case MBTN_MIDDLE_IND:
            SDL_SendMouseButton(wnd, ev.devid,
                                ev.input.digital.active, SDL_BUTTON_MIDDLE);
        break;
        case MBTN_WHEEL_UP_IND:
            if (ev.input.digital.active)
                SDL_SendMouseWheel(wnd, ev.devid, 0,-1, SDL_MOUSEWHEEL_NORMAL);
        break;
        case MBTN_WHEEL_DOWN_IND:
            if (ev.input.digital.active)
                SDL_SendMouseWheel(wnd, ev.devid, 0, 1, SDL_MOUSEWHEEL_NORMAL);
        break;
        default:
        break;
        }
    }
}

/*
 * We have a few possible tables because portable life is great.
 * SDL1.2 ? doesn't map cleanly to SDL2 (but of course).
 */
static inline void process_keyb(struct arcan_shmif_cont *prim,
                                struct arcan_shmif_cont *cur,
                                SDL_Window *wnd,
                                Arcan_SDL_Meta *meta,
                                arcan_ioevent ev)
{
    if (ev.input.translated.active && 0 != ev.input.translated.utf8[0]){
        SDL_SendKeyboardText((const char*)ev.input.translated.utf8);
    }

/*
 * Sadly enough, the keysym field was modeled after SDL1.2 from legacy,
 * as was the symtable.lua support script that is used everywhere. But we
 * have no 1.2->2.0 table here(!), so the "forward" compatible option for
 * the sdl- platform (osx) is to put that value in scancode, and think out
 * something else for BSD.
 */
#ifdef __LINUX__
    SDL_SendKeyboardKey(ev.input.translated.active,
        linux_scancode_table[ev.input.translated.scancode]);
#else
    SDL_SendKeyboardKey(ev.input.translated.active, ev.input.translated.scancode);
#endif
}

static inline void process_gamedev(struct arcan_shmif_cont *prim,
                                   struct arcan_shmif_cont *cure,
                                   SDL_Window *wnd,
                                   Arcan_SDL_Meta *meta,
                                   arcan_ioevent ev)
{
/* game device */
}

static inline void process_touch(struct arcan_shmif_cont *prim,
                                 struct arcan_shmif_cont *cure,
                                 SDL_Window *wnd,
                                 Arcan_SDL_Meta *meta,
                                 arcan_ioevent ev)
{
//    SDL_SendTouch(ev.devid, ev.subid, ev.touch.active, ...);
}

/*
 * On Window Management:
 * The Arcan API refuses quite a lot of the fine-grained control mechanisms for
 * windows on the "a normal application is not priv.  to do this, so we'd have
 * to implement a basic virtual window manager here and forward hinting events
 * on position upwards relative among the SDL windows. Before doing so, we'd
 * need good example software that actually uses these features.
 */
static void process_input(struct arcan_shmif_cont* prim,
                          struct arcan_shmif_cont* cur,
                          SDL_Window *wnd,
                          Arcan_SDL_Meta *meta,
                          arcan_ioevent ev)
{
    if (ev.devkind == EVENT_IDEVKIND_MOUSE){
        process_mouse(prim, cur, wnd, meta, ev);
    }
    else if (ev.devkind == EVENT_IDEVKIND_KEYBOARD){
        process_keyb(prim, cur, wnd, meta, ev);
    }
    else if (ev.devkind == EVENT_IDEVKIND_TOUCHDISP){
        process_touch(prim, cur, wnd, meta, ev);
    }
    else {
        process_gamedev(prim, cur, wnd, meta, ev);
    }
}

static void process_target(struct arcan_shmif_cont* prim,
                           struct arcan_shmif_cont* cur,
                           SDL_Window *wnd,
                           Arcan_SDL_Meta *meta,
                           arcan_tgtevent ev)
{
    switch (ev.kind){
    case TARGET_COMMAND_EXIT:
        SDL_SendWindowEvent(wnd, SDL_WINDOWEVENT_CLOSE, 0, 0);
    break;
    case TARGET_COMMAND_FONTHINT:
/* ignore, we don't have a way to communicate font changes within SDL */
    break;
    case TARGET_COMMAND_ATTENUATE:
/* FIXME: if we don't run with ourself as an audio driver try and at least
 * change the mixing output gain */
    break;
    case TARGET_COMMAND_DEVICE_NODE:
/* FIXME: need to indicate context loss and possily run into a suspend/
 * wait loop until we get it back (iv == 1), run migrate and treat as
 * case 3 reset command */
    break;
    case TARGET_COMMAND_RESET:
/* indicate that we have lost context? */
        switch(ev.ioevs[0].iv){
        case 0:
/* FIXME: check last known modifier state and send releases */
        case 1:
        break;
        case 2:
        case 3:
/* FIXME: drop all subwindows that are used (popup, ...) and re-request,
 * we may also need to indicate that we have lost GL context, same for
 * DEVICE_NODE */
        break;
        }
    break;
    case TARGET_COMMAND_BCHUNK_IN:
/* FIXME: send this as a DROPFILE if the state is enabled */
    break;
    case TARGET_COMMAND_STEPFRAME:
/* FIXME: we don't really have control to implement this here, an option
 * would be to add the blocking to the window- update and have a frame-
 * counter in the related structure */
    break;
    case TARGET_COMMAND_DISPLAYHINT:
        if ((ev.ioevs[0].iv && ev.ioevs[1].iv) &&
            (ev.ioevs[0].iv != cur->w || ev.ioevs[1].iv != cur->h)){
            if (wnd->flags & SDL_WINDOW_RESIZABLE){
/* FIXME: we need to lock audio here if we're on the primary segment */
                if (arcan_shmif_resize(cur, ev.ioevs[0].iv, ev.ioevs[1].iv)){
                    arcan_shmifext_make_current(cur);
                    SDL_SendWindowEvent(wnd, SDL_WINDOWEVENT_RESIZED,
                                        ev.ioevs[0].iv, ev.ioevs[1].iv);
                }
            }
        }
 /* This only affects a client setting fullscreen, not accepting resize */
    case TARGET_COMMAND_OUTPUTHINT:
        if (ev.ioevs[0].iv && ev.ioevs[1].iv){
            meta->disp_w = ev.ioevs[0].iv;
            meta->disp_h = ev.ioevs[1].iv;
        }
    break;
/* FIXME: we also have SDL_WINDOWEVENT_HIDDEN, SDL_WINDOWEVENT_SHOWN */
    break;
    case TARGET_COMMAND_NEWSEGMENT:
/* FIXME: if output segment, register new capture / audio device -
 * otherwise the only segment that should arrive here clipboard paste */
    break;
    default:
    break;
    }

/* for DISPLAYHINT:
 * RESIZED vs. Window flags RESIZABLE,
 * FOCUS_GAINED, FOCUS_LOST, CLOSED
 * if (!ioev[2].iv & 128):
 *  ioev[2].iv & 2 inactive
 *  ioev[2].iv & 4 > not focus */
}

static void eventDispatch(struct arcan_shmif_cont* prim,
                          struct arcan_shmif_cont* cur,
                          SDL_Window *wnd,
                          Arcan_SDL_Meta *meta,
                          arcan_event *ev)
{
    if (ev->category == EVENT_IO){
        process_input(prim, cur, meta->main, meta, ev->io);
    }
    else if (ev->category == EVENT_TARGET){
        process_target(prim, cur, meta->main, meta, ev->tgt);
    }
}

void Arcan_PumpEvents(_THIS)
{
    arcan_event ev;
    struct arcan_shmif_cont *prim = arcan_shmif_primary(SHMIF_INPUT);
    struct arcan_shmif_cont *con = prim;
    Arcan_SDL_Meta *meta = con->user;

/* don't process events until we are fully initialized with a working wnd */
    if (!con || !con->user || !meta->main)
        return;

/* events that might have accumulated while waiting for a subseg req. */
    if (meta->pqueue){
        for (int i = 0; i < meta->pqueue_sz && meta->pqueue_sz > 0; i++){
            eventDispatch(prim, con, meta->main, meta, &meta->pqueue[i]);
            if (arcan_shmif_descrevent(&meta->pqueue[i]) &&
                meta->pqueue[i].tgt.ioevs[0].iv != -1){
                    close(meta->pqueue[i].tgt.ioevs[0].iv);
            }
        }
        SDL_free(meta->pqueue);
        meta->pqueue = NULL;
        meta->pqueue_sz = 0;
   }

/* we maintain multiple con to handle events for all possible subsegs */
    SDL_LockMutex(meta->av_sync);
    while (con && meta->main){
        while (arcan_shmif_poll(con, &ev) > 0){
            eventDispatch(prim, con, meta->main, meta, &ev);
        }

/* defer / aggregate mouse events to reduce the cost of accumulated events */
        if (meta->dirty_mouse && meta->main){
            SDL_SendMouseMotion(meta->main, 0, meta->mrel, meta->mx, meta->my);
            meta->dirty_mouse = false;
        }

/* FIXME: should switch to next window here */
        con = NULL;
    }

    SDL_UnlockMutex(meta->av_sync);
}

/* vi: set ts=4 sw=4 expandtab: */
