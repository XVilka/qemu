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
    static inline void gles2_ret_##TYPE(gles2_State *s, TYPE value); \
    static inline void gles2_put_##TYPE(gles2_State *s, target_ulong va, TYPE value); \
    static inline TYPE gles2_get_##TYPE(gles2_State *s, target_ulong va); \
    static inline TYPE gles2_arg_##TYPE(gles2_State *s, gles2_decode_t *d); \

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

// Defines shorthands for handling types.
#define GLES2_TYPE_DEF(TYPE, SIZE) \
    static inline void gles2_ret_##TYPE(gles2_State *s, TYPE value) \
    { gles2_ret_##SIZE(s, value); } \
    static inline void gles2_put_##TYPE(gles2_State *s, target_ulong va, TYPE value) \
    { gles2_put_##SIZE(s, va, value); } \
    static inline TYPE gles2_get_##TYPE(gles2_State *s, target_ulong va) \
    { return (TYPE)gles2_get_##SIZE(s, va); } \
    static inline TYPE gles2_arg_##TYPE(gles2_State *s, gles2_decode_t *d) \
    { return (TYPE)gles2_arg_##SIZE(s, d); }

// Bunch of expansions of previous macro to ease things up.
GLES2_TYPE_DEF(Tptr, dword)
GLES2_TYPE_DEF(TEGLBoolean, dword)
GLES2_TYPE_DEF(TEGLenum, dword)
GLES2_TYPE_DEF(TEGLint, dword)
GLES2_TYPE_DEF(TEGLDisplay, handle)
GLES2_TYPE_DEF(TEGLConfig, handle)
GLES2_TYPE_DEF(TEGLContext, handle)
GLES2_TYPE_DEF(TEGLSurface, handle)

GLES2_TYPE_DEF(TGLclampf, float)
GLES2_TYPE_DEF(TGLbitfield, dword)
GLES2_TYPE_DEF(TGLboolean, byte)
GLES2_TYPE_DEF(TGLint, dword)
GLES2_TYPE_DEF(TGLuint, dword)
GLES2_TYPE_DEF(TGLushort, word)
GLES2_TYPE_DEF(TGLubyte, byte)
GLES2_TYPE_DEF(TGLenum, dword)
GLES2_TYPE_DEF(TGLsizei, dword)
GLES2_TYPE_DEF(TGLfloat, float)
GLES2_TYPE_DEF(TGLfixed, dword)
GLES2_TYPE_DEF(TGLclampx, dword)

#define GL_BYTE                           0x1400
#define GL_UNSIGNED_BYTE                  0x1401
#define GL_SHORT                          0x1402
#define GL_UNSIGNED_SHORT                 0x1403
#define GL_FLOAT                          0x1406
#define GL_FIXED                          0x140C

#endif // GLES2_CALLS_H__
