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

// From target-arm/helper.c.
extern int get_phys_addr(CPUState *env, uint32_t address,
                         int access_type, int is_user,
                         uint32_t *phys_ptr, int *prot,
                         target_ulong *page_size);

// List of calls to used by the kernel module (page 0).
static gles2_Call const gles2_kcalls[];
// List of calls used by clients (page 1->15).
static gles2_Call const gles2_calls[];

// Translate a target virtual address to physical address.
static target_ulong gles2_pa(gles2_State *s, target_ulong va,
    int access_type)
{
    target_ulong pa, ps;
    int prot;

    if (get_phys_addr(s->env, va, access_type, 1, &pa, &prot, &ps)) {
        GLES2_PRINT("ERROR: Page fault on guest!\n");
        return 0;
    }

    return pa;
}

int gles2_transfer_compile(gles2_CompiledTransfer* tfr, gles2_State *s,
    target_ulong va, target_ulong len)
{
    tfr->nsections = 0;
    tfr->sections = 0;

#if (GLES2_DEBUG == 1)
    target_ulong first_page = TARGET_PAGE(va);
#endif // GLES2_DEBUG == 1

    target_ulong last_page = TARGET_PAGE(va + len - 1);
    target_ulong start_addr = va;

    GLES2_PRINT("DEBUG: Compiling transfer of %d bytes at 0x%x (0x%x->0x%x).\n",
        len, va, first_page, last_page);

    // Loop through the pages.
    while (len) {
        target_ulong start_page = TARGET_PAGE(start_addr);
        target_ulong start_pa = gles2_pa(s, start_page, 0);
        target_ulong end_pa = start_pa;

        // Solve length of continuous section.
        target_ulong end_page = start_page;
        while(end_page < last_page) {
            target_ulong next_page = end_page + TARGET_PAGE_SIZE;
            target_ulong next_pa = gles2_pa(s, next_page, 0);

            // If the target pages are not linearly spaced, stop..
            if((next_pa < start_pa) ||
                (next_pa - start_pa > next_page - start_page)) {
                break;
            }

            end_page = next_page;
            end_pa = next_pa;
        }

        unsigned id = tfr->nsections++;

        GLES2_PRINT("\tContinuous from 0x%x to 0x%x (0x%x to 0x%x) #%d.\n",
	        start_page, end_page, start_pa, end_pa, id);
        tfr->sections = realloc(tfr->sections,
            tfr->nsections*sizeof(*(tfr->sections)));

        target_phys_addr_t pages_len = end_page + TARGET_PAGE_SIZE - start_page;
        void* target_pages = cpu_physical_memory_map(start_pa, &pages_len, 0);

        if (!target_pages || !pages_len) {
            fprintf(stderr, "ERROR: Failed to map memory to host!\n");
            return 0;
        }

        target_ulong section_len = end_page + TARGET_PAGE_SIZE - start_addr;

        if (section_len > len) {
            section_len = len;
        }

        target_ulong offset = TARGET_OFFSET(start_addr);
        void* target_data = target_pages + offset;

        GLES2_PRINT("\tSlice of %d bytes at %p (offset = %x).\n",
            section_len, target_data, offset);

        tfr->sections[id].base = target_data;
        tfr->sections[id].len = section_len;

        cpu_physical_memory_unmap(target_pages, pages_len, 0, pages_len);
        len -= section_len;
        start_addr += section_len;
        GLES2_PRINT("\t%d bytes remain...\n", len);
    }
    return 1;
}

void gles2_transfer_exec(gles2_CompiledTransfer* tfr, gles2_State *s,
    void* data, int access_type)
{
    unsigned i;

    for (i = 0; i < tfr->nsections; ++i) {
        void* target_data = tfr->sections[i].base;
        target_ulong len = tfr->sections[i].len;
        if (access_type == 0) {
            memcpy(data, target_data, len);
        } else {
            memcpy(target_data, data, len);
        }
        data += len;
    }
}

void gles2_transfer_free(gles2_CompiledTransfer* tfr)
{
    free(tfr->sections);
    tfr->sections = 0;
    tfr->nsections = 0;
}

int gles2_transfer(gles2_State *s, target_ulong va, target_ulong len,
    void* data, int access_type)
{
#if (GLES2_DEBUG == 1)
    target_ulong first_page = TARGET_PAGE(va);
#endif // GLES2_DEBUG == 1

    target_ulong last_page = TARGET_PAGE(va + len - 1);
    target_ulong start_addr = va;

    GLES2_PRINT("DEBUG: Request transfer of %d bytes at 0x%x (0x%x->0x%x) (access=%d).\n",
        len, va, first_page, last_page, access_type);

    // Loop through the pages.
    while (len) {
        target_ulong start_page = TARGET_PAGE(start_addr);
        target_ulong start_pa = gles2_pa(s, start_page, access_type);
        target_ulong end_pa = start_pa;

        // Solve length of continuous section.
        target_ulong end_page = start_page;
        while(end_page < last_page) {
            target_ulong next_page = end_page + TARGET_PAGE_SIZE;
            target_ulong next_pa = gles2_pa(s, next_page, access_type);

            // If the target pages are not linearly spaced, stop..
            if ((next_pa < start_pa) ||
                (next_pa - start_pa > next_page - start_page)) {
                break;
            }

            end_page = next_page;
            end_pa = next_pa;
        }

        GLES2_PRINT("\tContinuous from 0x%x to 0x%x (0x%x to 0x%x).\n",
            start_page, end_page, start_pa, end_pa);

        target_phys_addr_t pages_len = end_page + TARGET_PAGE_SIZE - start_page;
        void* target_pages = cpu_physical_memory_map(start_pa, &pages_len, access_type);
        if (!target_pages || !pages_len) {
            GLES2_PRINT("ERROR: Failed to map memory to host!\n");
            return 0;
        }

        target_ulong section_len = end_page + TARGET_PAGE_SIZE - start_addr;
        target_ulong offset = TARGET_OFFSET(start_addr);
        void* target_data = target_pages + offset;

        if (section_len > len) {
            section_len = len;
        }

        GLES2_PRINT("\tTransfering %d bytes at %p (offset = %x).\n",
            section_len, target_data, offset);

        if (access_type == 0) {
            memcpy(data, target_data, section_len);
        } else {
            memcpy(target_data, data, section_len);
        }

        cpu_physical_memory_unmap(target_pages, pages_len, access_type, pages_len);
        len -= section_len;
        start_addr += section_len;
        data += section_len;

        GLES2_PRINT("\t%d bytes remain...\n", len);
    }
    return 1;
}

void gles2_put_byte(gles2_State *s, target_ulong va, uint8_t byte)
{
    int prot;
    target_ulong pa, ps;

    if (get_phys_addr(s->env, va, 1, 1, &pa, &prot, &ps)) {
        GLES2_PRINT("ERROR: Memory mapping failed for 0x%x!\n", va);
        return;
    }

    GLES2_PRINT("DEBUG: Written 0x%x to 0x%x(0x%x)\n", byte, va, pa);
    stb_phys(pa, byte);
}

uint8_t gles2_get_byte(gles2_State *s, target_ulong va)
{
    uint8_t byte;
    int prot;
    target_ulong pa, ps;

    if (get_phys_addr(s->env, va, 0, 1, &pa, &prot, &ps)) {
        GLES2_PRINT("ERROR: Memory mapping failed for 0x%x!\n", va);
        return 0xDE;
    }

    byte = ldub_phys(pa);

    GLES2_PRINT("DEBUG: Read 0x%x from 0x%x(0x%x)\n", byte, va, pa);

    return byte;
}

void gles2_put_word(gles2_State *s, target_ulong va, uint16_t word)
{
    int prot;
    target_ulong pa, ps;

    if (get_phys_addr(s->env, va, 1, 1, &pa, &prot, &ps)) {
        GLES2_PRINT("ERROR: Memory mapping failed for 0x%x!\n", va);
        return;
    }

    GLES2_PRINT("DEBUG: Written 0x%x to 0x%x(0x%x)\n", word, va, pa);
    stw_phys(pa, word);
}

uint16_t gles2_get_word(gles2_State *s, target_ulong va)
{
    uint16_t word;

    int prot;
    target_ulong pa, ps;
    if (get_phys_addr(s->env, va, 0, 1, &pa, &prot, &ps)) {
        GLES2_PRINT("ERROR: Memory mapping failed for 0x%x!\n", va);
        return 0xDEAD;
    }

    word = lduw_phys(pa);

    GLES2_PRINT("DEBUG: Read 0x%x from 0x%x(0x%x)\n", word, va, pa);

    return word;
}

uint32_t gles2_get_dword(gles2_State *s, target_ulong va)
{
    uint32_t dword;
    int prot;
    target_ulong pa, ps;

    if (get_phys_addr(s->env, va, 0, 1, &pa, &prot, &ps)) {
        GLES2_PRINT("ERROR: Memory mapping failed for 0x%x!\n", va);
        return 0xDEAD000;
    }

    dword = ldl_phys(pa);

    GLES2_PRINT("DEBUG: Read 0x%x from 0x%x(0x%x)\n", dword, va, pa);

    return dword;
}

void gles2_put_dword(gles2_State *s, target_ulong va, uint32_t dword)
{
    int prot;
    target_ulong pa, ps;

    if (get_phys_addr(s->env, va, 1, 1, &pa, &prot, &ps)) {
        GLES2_PRINT("ERROR: Memory mapping failed for 0x%x!\n", va);
        return;
    }

    GLES2_PRINT("DEBUG: Written 0x%x to 0x%x(0x%x)\n", dword, va, pa);
    stl_phys(pa, dword);
}

float gles2_get_float(gles2_State *s, target_ulong va)
{
    float flt;
    int prot;
    target_ulong pa, ps;

    if (get_phys_addr(s->env, va, 0, 1, &pa, &prot, &ps)) {
        GLES2_PRINT("ERROR: Memory mapping failed for 0x%x!\n", va);
        return -123456789.f;
    }

    cpu_physical_memory_read(pa, (unsigned char*)&flt, 4);
    //	flt = ldfl_p(pa);

    GLES2_PRINT("DEBUG: Read %f from 0x%x(0x%x)\n", flt, va, pa);

    return flt;
}

void gles2_put_float(gles2_State *s, target_ulong va, float flt)
{
    int prot;
    target_ulong pa, ps;

    if (get_phys_addr(s->env, va, 1, 1, &pa, &prot, &ps)) {
        GLES2_PRINT("ERROR: Memory mapping failed for 0x%x!\n", va);
        return;
    }

    GLES2_PRINT("DEBUG: Written %f to 0x%x(0x%x)\n", flt, va, pa);
    cpu_physical_memory_write(pa, (unsigned char*)&flt, 4);
    //	stfl_p(pa, flt);
}

uint32_t gles2_handle_create(gles2_State *s, void* data)
{
    uint32_t i = 0;

    if (data) {
        for (i = 0; i < GLES2_NHANDLES; ++i) {
            if (!s->handles[i]) {
                break;
            }
        }

        if (i == GLES2_NHANDLES) {
            fprintf(stderr, "ERROR: No free handles!\n");
            return 0x0;
        }
        s->handles[i] = data;
        i |= GLES2_HANDLE_BASE;
    }

    GLES2_PRINT("Handle %x created for %p!\n", i, data);
    return i;
}

uint32_t gles2_handle_find(gles2_State *s, void* data)
{
    uint32_t i = 0;

    if (data) {
        for(i = 0; i < GLES2_NHANDLES; ++i) {
            if(s->handles[i] == data) {
                break;
            }
        }

        if (i == GLES2_NHANDLES) {
//            fprintf(stderr, "ERROR: Handle not found!\n");
            return 0x0;
        }
        i |= GLES2_HANDLE_BASE;
    }

    GLES2_PRINT("Handle %x found for %p!\n", i, data);
    return i;
}

void* gles2_handle_get(gles2_State *s, uint32_t i)
{
#if(GLES2_DEBUG == 1)
	if(i && (i & ~GLES2_HANDLE_MASK) != GLES2_HANDLE_BASE)
	{
		GLES2_PRINT("ERROR: Invalid handle %x!\n", i);
		exit(1);
	}
#endif // GLES2_DEBUG == 1

    void* data = i ? s->handles[i & GLES2_HANDLE_MASK] : NULL;

    GLES2_PRINT("Reading handle %x => %p\n", i, data);

    return data;
}

void* gles2_handle_free(gles2_State *s, uint32_t i)
{
    void* data = NULL;
    if (i) {
        data = s->handles[i & GLES2_HANDLE_MASK];
        s->handles[i & GLES2_HANDLE_MASK] = NULL;
    }

    GLES2_PRINT("Freed handle %i => %p\n", i, data);
    return data;
}

// Virtual register area write operation handler.
static void gles2_write(void *opaque, target_phys_addr_t addr, uint32_t value)
{
    gles2_State *s = (gles2_State*)opaque;

    target_ulong page = addr/(4*TARGET_PAGE_SIZE);
    target_ulong callnr = TARGET_OFFSET(addr)/0x04;

    GLES2_PRINT("Page %d write (call nr. %d).\n", page, callnr);

    if (page) {
        gles2_Call const *call = gles2_calls + callnr;

        if (page > 1) {
            gles2_Client* client;

            // Client API calls without active context should be ignored.
            if ((page - 2 > GLES2_NCLIENTS) ||
                !(client = s->clients[page - 2])) {
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
            GLES2_PRINT("Calling clientless function %s...\n", call->name);
            call->callback(s, &d, 0);
        }
    } else {
        gles2_Call const *call = gles2_kcalls + callnr;
        gles2_decode_t d = 0;

        GLES2_PRINT("Calling kernel function %s...\n", call->name);

        call->callback(s, &d, 0);
    }
}

// Virtual register area read operation handler.
static uint32_t gles2_read(void *opaque, target_phys_addr_t addr)
{
    gles2_State *s = (gles2_State*)opaque;

    target_ulong page = addr/(4*TARGET_PAGE_SIZE);

    if (page) {
        gles2_Client* client;

        // Client API calls without active context should be ignored.
        if ((page - 2 > GLES2_NCLIENTS) ||
            !(client = s->clients[page - 2])) {
            return 0;
        }

        if(pthread_mutex_trylock(&client->mutex_xcode)) {
            return 1;
        }

        if(client->phase_xcode == 2) {
            while (client->phase_xcode < 4) {
                client->phase_xcode = 3;
                pthread_cond_signal(&client->cond_return);
                pthread_cond_wait(&client->cond_xcode, &client->mutex_xcode);
            }
            pthread_mutex_unlock(&client->mutex_xcode);
            return 0;
        }
        int ret = (client->phase_xcode == 4 ? 0 : 1);
        pthread_mutex_unlock(&client->mutex_xcode);
        return ret;
    }
    return 0;
}

static CPUReadMemoryFunc *gles2_readfn[] = {
    gles2_read,
    gles2_read,
    gles2_read,
};

static CPUWriteMemoryFunc *gles2_writefn[] = {
    gles2_write,
    gles2_write,
    gles2_write,
};

// Initializes a new gles2 device.
void *gles2_init(CPUState *env)
{
    gles2_State *s = qemu_mallocz(sizeof(*s));
    unsigned i;

    s->env = env;
    s->abi = gles2_abi_unknown;
    s->quality = gles2_quality;

    GLES2_PRINT("GLES2 quality: %d\n", s->quality);

    cpu_register_physical_memory(GLES2_HWBASE,
        GLES2_HWSIZE,
        cpu_register_io_memory(gles2_readfn,
            gles2_writefn, s));

    for (i = 0; i < GLES2_NCLIENTS; ++i) {
        s->clients[i] = NULL;
    }

    for (i = 0; i < GLES2_NHANDLES; ++i) {
        s->handles[i] = NULL;
    }

    GLES2_PRINT("Registered IO memory!\n");
    return s;
}

/******************************************************************************
 *
 * Kernel interface functions.
 *
 *****************************************************************************/

#include "GLES2/gl2.h"
#ifdef __APPLE__
#ifndef __unix__
#define __unix__
#endif
#endif
#include "EGL/egl.h"

static void* gles2_client_worker(void *opaque)
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
            GLES2_PRINT("WORKER(%d): Calling function %s (%p)...\n",
                        client->nr, client->call->name, client->call);
            client->call->callback(client->s, &d, client);
#if(GLES2_DEBUG == 1)
            if (client->arrays) {
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
            }
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

// Called by kernel module when a new client connects.
static void gles2_init_cb(gles2_State *s, gles2_decode_t *d, gles2_Client *c)
{
    unsigned i;
    gles2_Client *client;
    pthread_attr_t attr;
    /* TODO: ABI is unknown at this point, ensure gles2_arg_dword works
     * with gles2_abi_unknown for the first parameter *always*! */
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

    GLES2_PRINT("Initialization!\n");

    if (!abi || abi >= gles2_abi_last) {
        fprintf(stderr, "ERROR: unknown ABI %d!\n", abi);
        gles2_ret_dword(s, 0);
        return;
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
    client->arrays = 0;
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
static void gles2_exit_cb(gles2_State *s, gles2_decode_t *d, gles2_Client *c)
{
    uint32_t nr = gles2_arg_dword(s, d);
    gles2_Client *client;

    GLES2_PRINT("Exit called for client %d!\n", nr);

    if ((nr > GLES2_NCLIENTS + 1) ||
	(nr == 0)) {
        GLES2_PRINT("Client number out of range!\n");
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

/******************************************************************************
 *
 * Call tables
 *
 *****************************************************************************/

/* Make a weak stub for every dummy function. */
#define CALL_DUMMY(func) \
    void gles2_##func##_cb(gles2_State *s, gles2_decode_t *d, struct gles2_Client *c); \
    void gles2_##func##_cb(gles2_State *s, gles2_decode_t *d, struct gles2_Client *c) \
    { \
        GLES2_BARRIER_ARG_NORET; \
        fprintf(stderr, "GLES2: DUMMY " #func "\n"); \
    }

#define CALL_ENTRY_NC(func) \
    void gles2_##func##_cb(gles2_State *s, gles2_decode_t *d, struct gles2_Client *c); \

#define CALL_ENTRY(func) \
	CALL_ENTRY_NC(func) \
	void gles2_##func##_cb_wrap(gles2_State *s, gles2_decode_t *d, struct gles2_Client *c); \
    void gles2_##func##_cb_wrap(gles2_State *s, gles2_decode_t *d, struct gles2_Client *c) \
{\
	if (c==NULL) \
	{ \
		GLES2_PRINT("No context. " #func " call ignored.\n"); \
		return; \
	} \
	else \
		gles2_##func##_cb(s,d,c); \
}




#include "gles2_calls.h"

#undef CALL_ENTRY
#undef CALL_ENTRY_NC
#undef CALL_DUMMY

#define CALL_ENTRY(func) { #func, gles2_##func##_cb_wrap },
#define CALL_ENTRY_NC(func) { #func, gles2_##func##_cb },
#define CALL_DUMMY(func) { #func, gles2_##func##_cb },

static gles2_Call const gles2_kcalls[] =
{
    CALL_ENTRY_NC(init)
    CALL_ENTRY_NC(exit)
};

static gles2_Call const gles2_calls[] =
{
    { "<<none>>", 0 },
#include "gles2_calls.h"
};

#undef CALL_ENTRY
#undef CALL_ENTRY_NC
