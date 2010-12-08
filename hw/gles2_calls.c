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

// Defines shorthands for handling types.
#define GLES2_TYPE_DEF(TYPE, SIZE) \
    inline void gles2_ret_##TYPE(gles2_State *s, TYPE value) \
    { gles2_ret_##SIZE(s, value); } \
    inline void gles2_put_##TYPE(gles2_State *s, target_ulong va, TYPE value) \
    { gles2_put_##SIZE(s, va, value); } \
    inline TYPE gles2_get_##TYPE(gles2_State *s, target_ulong va) \
    { return (TYPE)gles2_get_##SIZE(s, va); } \
    inline TYPE gles2_arg_##TYPE(gles2_State *s, gles2_decode_t *d) \
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


 void gles2_TransferArrays(gles2_State *s, gles2_Context *c,
    TGLint first, TGLsizei count)
{
    int i;

    for(i = 0; i < c->narrays; ++i) {
        gles2_Array* va = c->arrays + i;
        if(!va->enabled) {
            continue;
        }
        unsigned esize = 1;
        switch (va->type) {
            case GL_BYTE:
            case GL_UNSIGNED_BYTE:  esize = 1; break;
            case GL_SHORT:
            case GL_UNSIGNED_SHORT: esize = 2; break;
            case GL_FIXED:
            case GL_FLOAT:          esize = 4; break;
        }
        if (!va->stride) {
            va->stride = va->size*esize;
        }

        if (va->ptr) {
            free(va->ptr);
        }
        unsigned nbytes = esize*count*va->size;
        va->ptr = malloc(nbytes);

        unsigned int j;
        for (j = 0; j < count; ++j) {
            signed k;
            for (k = 0; k < va->size; ++k) {
                switch (esize) {
                    case 1:
                        ((TGLubyte*)va->ptr)[j*va->size + k] =
                            gles2_get_byte(s, va->tptr + va->stride*(first + j)
                            + k*sizeof(TGLubyte));
                        break;
                    case 2:
                        ((TGLushort*)va->ptr)[j*va->size + k] =
                            gles2_get_word(s, va->tptr + va->stride*(first + j)
                            + k*sizeof(TGLushort));
                        break;
                    case 4:
                        if(va->type == GL_FLOAT) {
                            ((TGLfloat*)va->ptr)[j*va->size + k] =
                                gles2_get_float(s, va->tptr
                                + va->stride*(first + j)
                                + k*sizeof(TGLfloat));
                        } else {
                            ((TGLuint*)va->ptr)[j*va->size + k] =
                                gles2_get_dword(s, va->tptr
                                + va->stride*(first + j)
                                + k*sizeof(TGLuint));
                        }
                        break;
                }
            }
        }

       va->apply(va);
    }
}


/**
 * used to know the index in %gles2_Client context array
 * where contexts of type context_client_type are stored
 * @param context_client_type is the client API.
 */
int gles2_getContextArrayIndex(EGLenum context_client_type)
{
    switch (context_client_type) {
        case EGL_OPENGL_ES_API:
        case EGL_OPENGL_API:
            return 0;
        break;
        case EGL_OPENVG_API:
            return 1;
        break;
        default:
            return -1;
        break;
    }

}
