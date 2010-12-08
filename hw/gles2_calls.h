/* Copyright (c) 2009-2010 Nokia Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * (at your option) any later version of the License.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */
#ifndef GLES2_CALLS_H__
#define GLES2_CALLS_H__

// Automatically create the prototype and function definition.

#include "gles2.h"
#define CB(FUNC, API) \
        PROTO(FUNC, API); \
        PROTO(FUNC, API)

// Sizes of primitive types in the ABI.
#define GLES2_HTYPE_byte uint8_t
#define GLES2_HTYPE_word uint16_t
#define GLES2_HTYPE_dword uint32_t
#define GLES2_HTYPE_float float
#define GLES2_HTYPE_handle uint32_t

// Defines shorthands for handling types.
#define GLES2_TYPE(TYPE, SIZE) \
    typedef GLES2_HTYPE_##SIZE TYPE; \
    inline void gles2_ret_##TYPE(gles2_State *s, TYPE value); \
    inline void gles2_put_##TYPE(gles2_State *s, target_ulong va, TYPE value); \
    inline TYPE gles2_get_##TYPE(gles2_State *s, target_ulong va); \
    inline TYPE gles2_arg_##TYPE(gles2_State *s, gles2_decode_t *d); \

// Bunch of expansions of previous macro to ease things up.
GLES2_TYPE(Tptr, dword)
GLES2_TYPE(TEGLBoolean, dword)
GLES2_TYPE(TEGLenum, dword)
GLES2_TYPE(TEGLint, dword)
GLES2_TYPE(TEGLDisplay, handle)
GLES2_TYPE(TEGLConfig, handle)
GLES2_TYPE(TEGLContext, handle)
GLES2_TYPE(TEGLSurface, handle)

GLES2_TYPE(TGLclampf, float)
GLES2_TYPE(TGLbitfield, dword)
GLES2_TYPE(TGLboolean, byte)
GLES2_TYPE(TGLint, dword)
GLES2_TYPE(TGLuint, dword)
GLES2_TYPE(TGLushort, word)
GLES2_TYPE(TGLubyte, byte)
GLES2_TYPE(TGLenum, dword)
GLES2_TYPE(TGLsizei, dword)
GLES2_TYPE(TGLfloat, float)
GLES2_TYPE(TGLfixed, dword)
GLES2_TYPE(TGLclampx, dword)


// Just one more macro for even less typing.
#define GLES2_ARG(TYPE, NAME) \
    TYPE NAME = gles2_arg_##TYPE(s, d)

unsigned gles1_glGetCount(TGLenum pname);
void checkGLESError(void);


// See if guest offscreen drawable was changed and if so, update host copy.
int gles2_surface_update(gles2_State *s, gles2_Surface *surf);

// TODO: Support swapping of offscreen surfaces.
void gles2_eglSwapCallback(void* userdata);

void gles2_TransferArrays(gles2_State *s, gles2_Context *c,
    TGLint first, TGLsizei count);

unsigned gles2_glGetCount(TGLenum pname);

unsigned gles2_glTexParameterCount(TGLenum pname);

int gles2_getContextArrayIndex(TEGLenum context_client_type);

// Host to guest vertex array copy.
struct gles2_Array
{
    TGLuint indx;          // Parameter of the call.
    TGLint size;           // --''--
    TGLenum type;          // --''--
    TGLboolean normalized; // --''--
    TGLsizei stride;       // --''--
    TGLsizei real_stride;       // --''--
    Tptr tptr;            // Pointer in the guest memory.
    void* ptr;            // Pointer in the host memory.

    void (*apply) (struct gles2_Array *va);
    TGLboolean enabled;    // State.
};
#endif // GLES2_CALLS_H__
