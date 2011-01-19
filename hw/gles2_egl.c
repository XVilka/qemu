/* Copyright (c) 2009-2010 Nokia Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 or
 * (at your option) any later version of the License.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "gles2.h"
#include <pthread.h>

#define CALL_ENTRY(func) \
    ENTRY(func, kernel)

#define CALL_DUMMY(func) \
    DUMMY(func, kernel)

#include "gles2_kernel.def"

#undef CALL_ENTRY
#undef CALL_DUMMY


/* egl function declaration */
#define CALL_ENTRY(func) \
    ENTRY( func, egl)

#define CALL_DUMMY(func) \
    DUMMY( func , egl)

#include "gles2_egl.def"

#undef CALL_ENTRY
#undef CALL_DUMMY

#define CALL_ENTRY(func) { #func, FNAME(func, egl,)},
#define CALL_DUMMY(func) { #func, FNAME(func, egl,)},

gles2_Call const gles2_egl_calls[] =
{
#include "gles2_egl.def"
};

#undef CALL_ENTRY
#undef CALL_DUMMY

#define CALL_ENTRY(func) { #func, FNAME(func, kernel,) },
#define CALL_DUMMY(func) { #func, FNAME(func, kernel,) },

gles2_Call const gles2_kernel_calls[] =
{
#include "gles2_kernel.def"
};

#undef CALL_ENTRY
#undef CALL_DUMMY


void gles2_egl_write(void *opaque, target_phys_addr_t addr, uint32_t value);
void gles2_egl_write(void *opaque, target_phys_addr_t addr, uint32_t value)
{
    gles2_State *s = (gles2_State*)opaque;

    target_ulong page = addr/(4*TARGET_PAGE_SIZE);
    target_ulong callnr = TARGET_OFFSET(addr)/0x04;

    GLES2_PRINT("page %d write (call nr. %d) in EGL block.\n", page, callnr);

    if (page) {
    gles2_Call const *call;
    call = gles2_egl_calls + callnr;

        if (page > 1) {
            gles2_Client* client;
        // Invalid clients (out of range) and
        // calls from pages for which there is no allocated client/worker thread
        //should be ignored.
            if ((page - 2 > GLES2_NCLIENTS) ||
                !(client = s->clients[page - 2])) {
                GLES2_PRINT("ignoring invalid page(%d) write...\n",page);
                return;
            }


            // Make sure nothing is running.
            GLES2_PRINT("Syncing with worker...\n");
            pthread_mutex_lock(&client->mutex_wait);
            while (client->state != gles2_ClientState_ready) {
                pthread_cond_wait(&client->cond_state, &client->mutex_wait);
            }
            pthread_mutex_lock(&client->mutex_run);
            pthread_mutex_lock(&client->mutex_xcode);
            client->call = call;
            client->state = gles2_ClientState_pending;
            client->phase_xcode = 0;
            GLES2_PRINT("Requesting call %s...\n", call->name);
            pthread_cond_signal(&client->cond_start);
            pthread_mutex_unlock(&client->mutex_wait);

            GLES2_PRINT("Releasing worker for decoding...\n");
            pthread_mutex_unlock(&client->mutex_run);
            do {
                pthread_cond_wait(&client->cond_xcode, &client->mutex_xcode);
            } while (client->phase_xcode < 1);

            pthread_mutex_unlock(&client->mutex_xcode);

            GLES2_PRINT("Decoding finished.\n");

        } else {
                gles2_decode_t d = 0;
                GLES2_PRINT("Calling contextless EGL function %s...\n", call->name);
                call->callback(s, &d, 0);
        }

    } else {
        gles2_Call const *call = gles2_kernel_calls + callnr;
        gles2_decode_t d = 0;
        GLES2_PRINT("Calling kernel function %s...\n", call->name);
        call->callback(s, &d, 0);
    }
}

CPUWriteMemoryFunc *gles2_egl_writefn[] = {
    gles2_egl_write,
    gles2_egl_write,
    gles2_egl_write,
};
