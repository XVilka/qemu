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

#include "gles2_calls.h"

void* gles2_client_worker(void *opaque);

#undef GLES2_CB
#define GLES2_CB(func) \
    CB(func, kernel)

// Called by kernel module when a new client connects.
GLES2_CB(init)
{
    unsigned i;
    gles2_Client *client;
    pthread_attr_t attr;
    uint32_t abi = gles2_arg_dword(s, d);

    for (i = 0; i < GLES2_NCLIENTS; ++i) {
        if (!s->clients[i]) {
            break;
        }
    }

    if (i == GLES2_NCLIENTS) {
        GLES2_PRINT("ERROR: No free slots!\n");
        gles2_ret_dword(s, 0);
        return;
    }

    GLES2_PRINT("Client Initialization!\n");

    if (!abi || abi >= gles2_abi_last) {
        GLES2_PRINT("ERROR: unknown ABI %d!\n", abi);
        /* support legacy clients that do not provide ABI id */
        abi = gles2_abi_arm_softfp;
    }
    if (s->abi == gles2_abi_unknown) {
        s->abi = abi;
        GLES2_PRINT("Selected ABI %d\n", s->abi);
    } else if (s->abi != abi) {
        fprintf(stderr, "ERROR: trying to change ABI (%d to %d)!\n", s->abi, abi);
        gles2_ret_dword(s, 0);
        return;
    }

    client = malloc(sizeof(*client));
    client->s = s;
    client->nr = i + 1;
    client->rendering_api = EGL_OPENGL_ES_API;
    pthread_mutex_init(&client->mutex_wait, NULL);
    pthread_mutex_init(&client->mutex_run, NULL);
    pthread_mutex_init(&client->mutex_xcode, NULL);
    pthread_cond_init(&client->cond_start, NULL);
    pthread_cond_init(&client->cond_state, NULL);
    pthread_cond_init(&client->cond_xcode, NULL);
    pthread_cond_init(&client->cond_return, NULL);
    client->state = gles2_ClientState_init;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    pthread_mutex_lock(&client->mutex_wait);

    GLES2_PRINT("Creating worker...\n");
    pthread_create(&client->thread, &attr, gles2_client_worker, client);

    do {
        pthread_cond_wait(&client->cond_state, &client->mutex_wait);
    } while(client->state != gles2_ClientState_ready);
    pthread_mutex_unlock(&client->mutex_wait);

    GLES2_PRINT("Worker initialized\n");

    s->clients[i] = client;
    gles2_ret_dword(s, client->nr);
}

// Called by kernel module when an existing client disconnects.
 GLES2_CB(exit)
{
    uint32_t nr = gles2_arg_dword(s, d);
    gles2_Client *client;

    GLES2_PRINT("Exit called for client %d!\n", nr);

    if ((nr > GLES2_NCLIENTS + 1) ||
    (nr < 1)) {
        GLES2_PRINT("Client number (%d) out of range!\n",nr);
    return;
    }  else {
        client = s->clients[nr - 1];
    }

    if (!client) {
        GLES2_PRINT("Can't exit NULL client!\n");
        return;
    }

    GLES2_PRINT("\tRequesting worker to exit.\n");

    // Make sure nothing is running.
    GLES2_PRINT("Syncing with worker...\n");
    pthread_mutex_lock(&client->mutex_wait);
    while (client->state != gles2_ClientState_ready) {
        pthread_cond_wait(&client->cond_state, &client->mutex_wait);
    }
    pthread_mutex_lock(&client->mutex_run);
    client->call = NULL;
    client->state = gles2_ClientState_pending;
    GLES2_PRINT("Requesting exit...\n");
    pthread_cond_signal(&client->cond_start);
    pthread_mutex_unlock(&client->mutex_wait);

    GLES2_PRINT("Waiting worker to exit...\n");
    do {
        pthread_cond_wait(&client->cond_state, &client->mutex_run);
    } while (client->state != gles2_ClientState_exit);
    pthread_mutex_unlock(&client->mutex_run);

    GLES2_PRINT("\tJoining...\n");
    pthread_join(client->thread, NULL);
    pthread_mutex_destroy(&client->mutex_wait);
    pthread_mutex_destroy(&client->mutex_run);
    pthread_cond_destroy(&client->cond_start);
    pthread_cond_destroy(&client->cond_state);

    free(client);
    s->clients[nr - 1] = NULL;

    GLES2_PRINT("\tDone!\n");
}

/**
* Worker thread function for clients .
* Each worker thread is linked to a %gles2_Client struct and to
* one guest thread in the guest system.
*/
void* gles2_client_worker(void *opaque)
{
    gles2_Client *client = opaque;
    int run = 1;

    GLES2_PRINT("WORKER(%d): Starting!\n", client->nr);

    pthread_mutex_lock(&client->mutex_xcode);
    do
    {
        gles2_decode_t d = 0;
        GLES2_PRINT("WORKER(%d): Waiting for call...\n", client->nr);
        pthread_mutex_lock(&client->mutex_wait);

        client->state = gles2_ClientState_ready;
        pthread_cond_signal(&client->cond_state);
        client->phase_xcode = 4;
        pthread_cond_signal(&client->cond_xcode);
        pthread_mutex_unlock(&client->mutex_xcode);
        while (client->state != gles2_ClientState_pending) {
            pthread_cond_wait(&client->cond_start, &client->mutex_wait);
        }

        GLES2_PRINT("WORKER(%d): Got call, waiting permission to run...\n", client->nr);

        pthread_mutex_lock(&client->mutex_run);
        GLES2_PRINT("WORKER(%d): Running!\n", client->nr);
        client->state = gles2_ClientState_running;
        pthread_mutex_unlock(&client->mutex_wait);

        if (client->call) {
            GLES2_TRACE("WORKER(%d): Calling function %s (%p)...\n",
                        client->nr, client->call->name, client->call);
            client->call->callback(client->s, &d, client);
#if(GLES2_DEBUG == 1)
            /*if (client->arrays) {
                if((client->call - gles2_calls) < 35) {
                    EGLint error;
                    if((error = eglGetError()) != EGL_SUCCESS) {
                        fprintf(stderr, "WARNING: EGL error 0x%x at function %s!\n", error, client->call->name);
                    }
                } else {
                    GLenum error;
                    if((error = glGetError()) != GL_NO_ERROR) {
                        fprintf(stderr, "WARNING: GL error 0x%x at function %s!\n", error, client->call->name);
                    }
                }
            }*/
#endif // GLES2_DEBUG == 1
            GLES2_PRINT("\tWORKER(%d): Done.\n", client->nr);
            client->prev_call = client->call;
            client->state = gles2_ClientState_done;
        } else {
            GLES2_PRINT("WORKER(%d): Exit requested!\n", client->nr);
            run = 0;
            client->state = gles2_ClientState_exit;
            pthread_cond_signal(&client->cond_state);
        }
        pthread_mutex_unlock(&client->mutex_run);
    } while (run);

    GLES2_PRINT("WORKER(%d): Exiting!\n", client->nr);
    return opaque;
}
