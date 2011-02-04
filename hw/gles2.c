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
//FIXME: Move this to a better place
#ifdef __APPLE__
#ifndef __unix__
#define __unix__
#endif
#endif

extern void gles1_loadHGL(void);
extern void gles2_loadHGL(void);

#ifdef _WIN32
#include <windows.h>
void* dlopen(char const*name, unsigned flags)
{
    return LoadLibrary(name);
}

void* dlsym(void* handle, char const* proc)
{
    return GetProcAddress(handle,proc);
}
int dlclose(void* handle)
{
    return !FreeLibrary(handle);
}
#endif
// From target-arm/helper.c.
extern int get_phys_addr(CPUState *env, uint32_t address,
                         int access_type, int is_user,
                         uint32_t *phys_ptr, int *prot,
                         target_ulong *page_size);

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
    //  flt = ldfl_p(pa);

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
    //  stfl_p(pa, flt);
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

// Virtual register area read operation handler.
uint32_t gles2_read(void *opaque, target_phys_addr_t addr)
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


CPUReadMemoryFunc *gles2_readfn[] = {
    gles2_read,
    gles2_read,
    gles2_read,
};

//from gles2_xx.c
extern CPUWriteMemoryFunc *gles2_egl_writefn[];
extern CPUWriteMemoryFunc *gles2_es11_writefn[];
extern CPUWriteMemoryFunc *gles2_es20_writefn[];

// Initializes a new gles device.
void *gles2_init(CPUState *env)
{
    setenv("DGLES2_FRONTEND", "offscreen", 1);
    setenv("DGLES2_NO_ALPHA", "1", 1);

    gles2_State *s = qemu_mallocz(sizeof(*s));
    unsigned i;

    s->env = env;
    s->abi = gles2_abi_arm_hardfp;
    s->quality = gles2_quality;

    GLES2_PRINT("GLES2 quality: %d\n", s->quality);

    //register EGL
    GLES2_PRINT("Mapping EGL Block to : %x\n", GLES2_EGL_HWBASE);
    cpu_register_physical_memory(GLES2_EGL_HWBASE, GLES2_BLOCKSIZE,
                                 cpu_register_io_memory(gles2_readfn,
                                                        gles2_egl_writefn, s,
                                                        DEVICE_NATIVE_ENDIAN));

    //register ES11
    GLES2_PRINT("Mapping ES11 Block to : %x\n", GLES2_ES11_HWBASE);
    cpu_register_physical_memory(GLES2_ES11_HWBASE, GLES2_BLOCKSIZE,
                                 cpu_register_io_memory(gles2_readfn,
                                                        gles2_es11_writefn, s,
                                                        DEVICE_NATIVE_ENDIAN));

    //register ES20
    GLES2_PRINT("Mapping ES20 Block to : %x\n", GLES2_ES20_HWBASE);
    cpu_register_physical_memory(GLES2_ES20_HWBASE, GLES2_BLOCKSIZE,
                                 cpu_register_io_memory(gles2_readfn,
                                                        gles2_es20_writefn, s,
                                                        DEVICE_NATIVE_ENDIAN));

    for (i = 0; i < GLES2_NCLIENTS; ++i) {
        s->clients[i] = NULL;
    }

    for (i = 0; i < GLES2_NHANDLES; ++i) {
        s->handles[i] = NULL;
    }

    GLES2_PRINT("Registered IO memory!\n");

    gles1_loadHGL();
    gles2_loadHGL();

    return s;
}
