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

#ifndef GLES2_H__
#define GLES2_H__

#include "qemu-common.h"
#include "cpu.h"
#include <pthread.h>

#define GLES2_HWBASE 0x4f000000
#define GLES2_HWSIZE 0x00100000
#define GLES2_NCLIENTS (GLES2_HWSIZE/TARGET_PAGE_SIZE - 2)
#define GLES2_NHANDLES GLES2_NCLIENTS * 16
// Address base for host to guest pointer handles.
#define GLES2_HANDLE_BASE 0xCAFE0000
#define GLES2_HANDLE_MASK 0x0000FFFF // Handle to index bitmask.

#define GLES2_BARRIER_ARG \
    if (c) { \
        pthread_mutex_lock(&c->mutex_xcode); \
        c->phase_xcode = 1; \
        pthread_cond_signal(&c->cond_xcode); \
        pthread_mutex_unlock(&c->mutex_xcode); \
        GLES2_PRINT("-- ARG BARRIER --\n"); \
    }
#define GLES2_BARRIER_ARG_NORET \
    if (c) { \
        pthread_mutex_lock(&c->mutex_xcode); \
        c->phase_xcode = 3; \
        GLES2_PRINT("-- ARG & RETURN BARRIER --\n"); \
    }
#define GLES2_BARRIER_RET \
    if (c) { \
        pthread_mutex_lock(&c->mutex_xcode); \
        c->phase_xcode = 2; \
        do { \
            pthread_cond_wait(&c->cond_return, &c->mutex_xcode); \
        } while (c->phase_xcode == 2); \
        GLES2_PRINT("-- RETURN BARRIER --\n"); \
    }

// Round address to lower page boundary.
#define TARGET_PAGE(addr) ((addr) & ~(TARGET_PAGE_SIZE - 1))
// Return the page offset part of address.
#define TARGET_OFFSET(addr) ((addr) & (TARGET_PAGE_SIZE - 1))

#define GLES2_DEBUG 0
#if(GLES2_DEBUG == 1)
#   define GLES2_DEBUG_ARGS 1
#   define GLES2_PRINT(format, args...) \
        fprintf(stderr, "GLES2: " format, ##args)
#else
#   define GLES2_DEBUG_ARGS 0
#   define GLES2_PRINT(format, args...) (void)0
#endif // GLES_DEBUG != 1

struct gles2_Array;
typedef struct gles2_Array gles2_Array;
struct gles2_Call;
typedef struct gles2_Call gles2_Call;
struct gles2_State;
typedef struct gles2_State gles2_State;

typedef enum gles2_ClientState
{
    gles2_ClientState_init,
    gles2_ClientState_ready,
    gles2_ClientState_pending,
    gles2_ClientState_running,
    gles2_ClientState_done,
    gles2_ClientState_exit
} gles2_ClientState;

// A connected GLES2 client.
typedef struct gles2_Client
{
    gles2_State *s;     // Link to the device state the client was connected to.
    target_ulong nr;    // The client ID from kernel.

    gles2_Call const *call;     // Next/current call to perform.
    gles2_Call const *prev_call;// Previous call executed.
    pthread_t thread;           // The worker thread.
    pthread_cond_t cond_start;  // To wake worker for a call.
    pthread_cond_t cond_state;  // Worker signals when state changes.
    volatile gles2_ClientState state;// Status of the client.
    pthread_mutex_t mutex_run;  // Locked when thread is running, or is
                                // prevented from doing so.
    pthread_mutex_t mutex_wait; // For synchronization of worker and caller.

    pthread_mutex_t mutex_xcode; // For decode/encode synchronization.
    volatile int phase_xcode;    // Phase of call 0: pending 1: decode done
                                 // 2: exec done 3: encode done
    pthread_cond_t cond_xcode;   // --''--
    pthread_cond_t cond_return;  // --''--

    gles2_Array *arrays;        // Host side vertex pointer arrays.
    int narrays;                // Number of arrays (the maximum too).
} gles2_Client;

typedef enum gles2_abi_t {
    gles2_abi_unknown = 0,
    gles2_abi_arm_softfp,
    gles2_abi_arm_hardfp,

    gles2_abi_last
} gles2_abi_t;

// The GLES2 device state holder.
struct gles2_State
{
    CPUState *env;                         // The CPU the device is attached to.
    gles2_abi_t abi;                       // ABI used to pass values.
    gles2_Client *clients[GLES2_NCLIENTS]; // Array of clients.
    void *handles[GLES2_NHANDLES];         // Handles passed from host to guest.
    int quality;                           // Rendering quality.
};

typedef unsigned int gles2_decode_t; // Function call decoding state.
typedef uint32_t gles2_target_arg_t; // Target unit argument type.
// Callback for register area access.
typedef void gles2_Callback(gles2_State *s, gles2_decode_t *d,
    gles2_Client *c);

// Information holder for guest->host call.
struct gles2_Call
{
#ifndef NDEBUG
    char const* name;
#endif //!NDEBUG
    gles2_Callback* callback;
};

// Create and initialize a GLES2 device and attach to CPU.
extern void *gles2_init(CPUState *env);
// Rendering quality option.
extern int gles2_quality;

/******************************************************************************
 *
 * Guest memory continuous access functions
 *
 *****************************************************************************/

// Holder for compiled transfer holder.
typedef struct gles2_CompiledTransfer
{
    unsigned nsections;   // Number of physical memory sections in the transfer.
    struct
    {
        char* base;       // Base address of the section.
        target_ulong len; // Length of the section.
    } *sections;          // Sections of the transfer.
} gles2_CompiledTransfer;

// Pre-compile a transfer to or from virtual guest address.
// NOTE: An assumption is made that the mapping is equal for read and write, for
//       complicated transfers, use gles2_transfer or Fix It!
extern int  gles2_transfer_compile(gles2_CompiledTransfer *tfr, gles2_State *s,
    target_ulong va, target_ulong len);
// Execute a pre-compiled transfer.
extern void gles2_transfer_exec(gles2_CompiledTransfer *tfr, gles2_State *s,
    void* data, int access_type);
// Free a pre-compiled transfer.
extern void gles2_transfer_free(gles2_CompiledTransfer *tfr);
// Perform a non-compiled transfer between guest and host.
// access_type, read = 0, write = 1, execute = 2
extern int  gles2_transfer(gles2_State *s, target_ulong va, target_ulong len,
    void* data, int access_type);

/******************************************************************************
 *
 * Guest memory random access functions
 *
 *****************************************************************************/

// Read an 8-bit byte from target system memory.
extern uint8_t  gles2_get_byte(gles2_State *s, target_ulong va);
// Write an 8-bit byte to target system memory.
extern void     gles2_put_byte(gles2_State *s, target_ulong va, uint8_t byte);
// Read a 16-bit word from target system memory.
extern uint16_t gles2_get_word(gles2_State *s, target_ulong va);
// Write a 16-bit word to target system memory.
extern void     gles2_put_word(gles2_State *s, target_ulong va, uint16_t word);
// Read a 32-bit double word from target system memory.
extern uint32_t gles2_get_dword(gles2_State *s, target_ulong va);
// Write a 32-bit double word to target system memory.
extern void     gles2_put_dword(gles2_State *s, target_ulong va,
    uint32_t dword);
// Read a 32-bit float from target system memory.
extern float    gles2_get_float(gles2_State *s, target_ulong va);
// Write a 32-bit float to target system memory.
extern void     gles2_put_float(gles2_State *s, target_ulong va, float flt);

#define gles2_put_handle(s, va, handle) gles2_put_dword(s, va, handle)
#define gles2_get_handle(s, va) gles2_get_dword(s, va)

/******************************************************************************
 *
 * Handle management functions
 *
 *****************************************************************************/

// Create a handle from host side pointer to be passed to guest.
extern uint32_t gles2_handle_create(gles2_State *s, void* data);
// Find if there is previously created handle for pointer.
extern uint32_t gles2_handle_find(gles2_State *s, void* data);
// Get the host pointer by guest handle.
extern void* gles2_handle_get(gles2_State *s, uint32_t i);
// Release a handle for reuse.
extern void* gles2_handle_free(gles2_State *s, uint32_t i);

// Get the smallest function argument according to target CPU ABI.
static inline gles2_target_arg_t gles2_arg_raw(gles2_State *s, unsigned i);
static inline gles2_target_arg_t gles2_arg_raw(gles2_State *s, unsigned i)
{
    unsigned j = i >> 8;
    i = i & 0xff;
    if (s->abi == gles2_abi_arm_hardfp) {
        if (i < 4) {
            return s->env->regs[i];
        }
        j = (j < 16) ? 0 : (j - 16);
    } else {
        if (i + j < 4) {
            return s->env->regs[i + j];
        }
    }
    return gles2_get_dword(s, s->env->regs[13] + 2*0x04 + ((j + i - 4)*0x04));
}

/******************************************************************************
 *
 * ABI function argument decoding functions
 *
 *****************************************************************************/

static inline uint8_t gles2_arg_byte(gles2_State *s, gles2_decode_t *d);
static inline uint8_t gles2_arg_byte(gles2_State *s, gles2_decode_t *d)
{
    uint8_t byte = gles2_arg_raw(s, (*d)++) & 0xFF;
#if (GLES2_DEBUG_ARGS == 1)
    GLES2_PRINT("byte arg(%d) = %x\n", *d - 1, byte);
#endif
    return byte;
}

static inline uint16_t gles2_arg_word(gles2_State *s, gles2_decode_t *d);
static inline uint16_t gles2_arg_word(gles2_State *s, gles2_decode_t *d)
{
    uint16_t word = gles2_arg_raw(s, (*d)++) & 0xFFFF;
#if (GLES2_DEBUG_ARGS == 1)
    GLES2_PRINT("word arg(%d) = %x\n", *d - 1, word);
#endif
    return word;
}

static inline uint32_t gles2_arg_dword(gles2_State *s, gles2_decode_t *d);
static inline uint32_t gles2_arg_dword(gles2_State *s, gles2_decode_t *d)
{
    uint32_t dword = gles2_arg_raw(s, (*d)++);
#if (GLES2_DEBUG_ARGS == 1)
    GLES2_PRINT("dword arg(%d) = %x\n", *d - 1, dword);
#endif
    return dword;
}

static inline uint64_t gles2_arg_qword(gles2_State *s, gles2_decode_t *d);
static inline uint64_t gles2_arg_qword(gles2_State *s, gles2_decode_t *d)
{
    uint64_t qword = gles2_arg_raw(s, (*d)++)
        | ((uint64_t)gles2_arg_raw(s, (*d)++) << 32);
#if (GLES2_DEBUG_ARGS == 1)
    GLES2_PRINT("qword arg(%d) = %"PRIu64"\n", *d - 2, qword);
#endif
    return qword;
}

static inline uint32_t gles2_arg_handle(gles2_State *s, gles2_decode_t *d);
static inline uint32_t gles2_arg_handle(gles2_State *s, gles2_decode_t *d)
{
    uint32_t handle = gles2_arg_raw(s, (*d)++);

#if (GLES2_DEBUG_ARGS == 1)
    GLES2_PRINT("handle arg(%d) = %x\n", *d - 1, handle);
#endif
    return handle;
}

// This needs to be its own special function, because we must preserve the byteorder.
static inline float gles2_arg_float(gles2_State *s, gles2_decode_t *d);
static inline float gles2_arg_float(gles2_State *s, gles2_decode_t *d)
{
    unsigned i = *d, j;
    j = i >> 8;
    i = i & 0xff;
    *d += 1 << 8;

    if (s->abi == gles2_abi_arm_hardfp) {
        if (j < 16) {
            return ((float*)(((CPUARMState *)s->env)->vfp.regs))[j];
        }
        j += ((i < 4) ? 0 : (i - 4)) - 16;
    } else if (s->abi == gles2_abi_arm_softfp) {
        if (i + j < 4) {
            return *((float*)&s->env->regs[i + j]);
        }
        j = i + j - 4;
    }
    return gles2_get_float(s, s->env->regs[13] + 2*0x04 + j*0x04);
}

/******************************************************************************
 *
 * ABI return value encoding functions
 *
 *****************************************************************************/

static inline void gles2_ret_byte(gles2_State *s, uint8_t byte);
static inline void gles2_ret_byte(gles2_State *s, uint8_t byte)
{
    s->env->regs[0] = byte;
#if (GLES2_DEBUG_ARGS == 1)
    GLES2_PRINT("byte ret = %d\n", byte);
#endif
}

static inline void gles2_ret_word(gles2_State *s, uint16_t word);
static inline void gles2_ret_word(gles2_State *s, uint16_t word)
{
    s->env->regs[0] = word;
#if (GLES2_DEBUG_ARGS == 1)
    GLES2_PRINT("word ret = %d\n", word);
#endif
}

static inline void gles2_ret_dword(gles2_State *s, uint32_t dword);
static inline void gles2_ret_dword(gles2_State *s, uint32_t dword)
{
    s->env->regs[0] = dword;
#if (GLES2_DEBUG_ARGS == 1)
    GLES2_PRINT("dword ret = %d\n", dword);
#endif
}

static inline void gles2_ret_qword(gles2_State *s, uint64_t qword);
static inline void gles2_ret_qword(gles2_State *s, uint64_t qword)
{
    s->env->regs[0] = qword & 0xFFFFFFFF;
    s->env->regs[1] = qword >> 32;
#if (GLES2_DEBUG_ARGS == 1)
    GLES2_PRINT("qword ret = %"PRIu64"\n", qword);
#endif
}

static inline void gles2_ret_handle(gles2_State *s, uint32_t handle);
static inline void gles2_ret_handle(gles2_State *s, uint32_t handle)
{
    s->env->regs[0] = handle;

#if (GLES2_DEBUG_ARGS == 1)
    GLES2_PRINT("handle ret = %x\n", handle);
#endif
}

static inline void gles2_ret_float(gles2_State *s, float flt);
static inline void gles2_ret_float(gles2_State *s, float flt)
{
    if (s->abi == gles2_abi_arm_hardfp) {
        ((CPUARMState *)s->env)->vfp.regs[0] = *(uint32_t*)&flt;
    } else {
        s->env->regs[0] = *(uint32_t*)&flt;
    }
#if (GLES2_DEBUG_ARGS == 1)
    GLES2_PRINT("float ret = %f\n", flt);
#endif
}

#endif // GLES2_H__
