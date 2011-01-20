/* Copyright (c) 2009-2010 Nokia Corporation
 * Copyright (c) 2010 Samsung Electronics.
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
#include "GLES/gl.h"
#include "glapi.h"

static int context_index = 0;

static HGL hgl;
#undef GLES2_CB
#define GLES2_CB(func) \
    CB(func,es11)

/*********************************************
*
*  OpenGL ES 1.1 helper functions and macros
*
**********************************************/

#define gles1_count_glClipPlanef  4
#define gles1_count_glClipPlanex  4
#define gles1_count_glLoadMatrixf 16
#define gles1_count_glLoadMatrixx 16
#define gles1_count_glMultMatrixf 16
#define gles1_count_glMultMatrixx 16


// Numbers from c->array's end
#define gles1_num_glColorPointer     1
#define gles1_num_glNormalPointer    2
#define gles1_num_glTexCoordPointer  3
#define gles1_num_glVertexPointer    4
#define gles1_num_glPointSizePointer 5

#define gles1_size_glNormalPointer 3
#define gles1_size_glPointSizePointer 1
static void gles1_apply_glVertexPointer(gles2_Array *va);
static void gles1_apply_glTexCoordPointer(gles2_Array *va);
static void gles1_apply_glNormalPointer(gles2_Array *va);
static void gles1_apply_glColorPointer(gles2_Array *va);
static void gles1_apply_glPointSizePointer(gles2_Array *va);

void gles1_loadHGL(void);
void gles1_loadHGL(void)
{
    const char *libname =
#ifdef __APPLE__
    "libGLES_CM.dylib";
#elif defined(_WIN32)
    "GLES_CM.dll";
#else
    "libGLES_CM.so";
#endif

    void* handle;
    if (!(handle = dlopen(libname, RTLD_LOCAL | RTLD_LAZY))) {
        fprintf(stderr, "ERROR: Couldn't load GLES_CM library!\n");
        exit(0);
    }

    #define GLES2_HGL_FUNC(ret,name,attr) \
        if((hgl.name = dlsym(handle, #name))==NULL) \
        { \
            fprintf(stderr, "ES1.1 Function " #name " not found!\n"); \
        }
    GLES2_HGL_FUNCS
    #undef DGLES_HGL_FUNC
}

static void gles1_apply_glColorPointer(gles2_Array *va)
{
    hgl.glColorPointer(va->size, va->type,
        0, va->ptr);
    GLenum error;

    if ((error = hgl.glGetError()) != GL_NO_ERROR) {
        GLES2_PRINT("glColorPointer(%d, 0x%x, %d, %p\n)"
            " failed with 0x%x!\n", va->size, va->type,
            va->stride, va->ptr, error);
    }
}

static void gles1_apply_glNormalPointer(gles2_Array *va)
{
    hgl.glNormalPointer(va->type, 0, va->ptr);
    GLenum error;

    if ((error = hgl.glGetError()) != GL_NO_ERROR) {
        GLES2_PRINT("glNormalPointer(0x%x, %d, %p\n)"
            " failed with 0x%x!\n", va->type,
            va->stride, va->ptr, error);
    }
}

static void gles1_apply_glTexCoordPointer(gles2_Array *va)
{
    hgl.glTexCoordPointer(va->size, va->type,
       0, va->ptr);
    GLenum error;

    if ((error = hgl.glGetError()) != GL_NO_ERROR) {
        GLES2_PRINT("glTexCoordPointer(%d, 0x%x, %d, %p\n)"
            " failed with 0x%x!\n", va->size, va->type,
            va->stride, va->ptr, error);
    }
}

static void gles1_apply_glVertexPointer(gles2_Array *va)
{
    hgl.glVertexPointer(va->size, va->type,
        0, va->ptr);
    GLenum error;

    if ((error = hgl.glGetError()) != GL_NO_ERROR) {
        GLES2_PRINT("glVertexPointer(%d, 0x%x, %d, %p\n)"
            " failed with 0x%x!\n", va->size, va->type,
            va->stride, va->ptr, error);
    }
}

static void gles1_apply_glPointSizePointer(gles2_Array *va)
{
    hgl.glPointSizePointerOES(va->type, 0, va->ptr);
    GLenum error;
    
    if ((error = hgl.glGetError()) != GL_NO_ERROR) {
        GLES2_PRINT("glPointSizePointerOES(0x%x, %d, %p) failed with 0x%x!\n",
                    va->type, va->stride, va->ptr, error);
    }
}

static unsigned gles2_GetCount(TGLenum pname)
{
    unsigned count;
    switch(pname) {
        case GL_ACTIVE_TEXTURE: count=1; break;
        case GL_ALIASED_LINE_WIDTH_RANGE: count=2; break;
        case GL_ALIASED_POINT_SIZE_RANGE: count=2; break;
        case GL_ALPHA_BITS: count=1; break;
        case GL_ALPHA_SCALE: count=1; break;
        case GL_ALPHA_TEST: count=1; break;
        case GL_ALPHA_TEST_FUNC: count=1; break;
        case GL_ALPHA_TEST_REF: count=1; break;
        case GL_AMBIENT: count=4; break;
        case GL_AMBIENT_AND_DIFFUSE: count=4; break;
        case GL_ARRAY_BUFFER_BINDING: count=1; break;
        case GL_BLEND: count=1; break;
        case GL_BLEND_DST: count=1; break;
        case GL_BLEND_SRC: count=1; break;
        case GL_BLUE_BITS: count=1; break;
        case GL_CLIENT_ACTIVE_TEXTURE: count=1; break;
        case GL_CLIP_PLANE0: count=1; break;
        case GL_CLIP_PLANE1: count=1; break;
        case GL_CLIP_PLANE2: count=1; break;
        case GL_CLIP_PLANE3: count=1; break;
        case GL_CLIP_PLANE4: count=1; break;
        case GL_CLIP_PLANE5: count=1; break;
        case GL_COLOR_ARRAY: count=1; break;
        case GL_COLOR_ARRAY_BUFFER_BINDING: count=1; break;
        case GL_COLOR_ARRAY_SIZE: count=1; break;
        case GL_COLOR_ARRAY_STRIDE: count=1; break;
        case GL_COLOR_ARRAY_TYPE: count=1; break;
        case GL_COLOR_CLEAR_VALUE: count=4; break;
        case GL_COLOR_LOGIC_OP: count=1; break;
        case GL_COLOR_MATERIAL: count=1; break;
        case GL_COLOR_WRITEMASK: count=4; break;
        case GL_COMBINE_ALPHA: count=1; break;
        case GL_COMBINE_RGB: count=1; break;
        case GL_CONSTANT_ATTENUATION: count=1; break;
        case GL_CULL_FACE: count=1; break;
        case GL_CULL_FACE_MODE: count=1; break;
        case GL_CURRENT_COLOR: count=4; break;
        case GL_CURRENT_NORMAL: count=3; break;
        case GL_CURRENT_TEXTURE_COORDS: count=4; break;
        case GL_DEPTH_BITS: count=1; break;
        case GL_DEPTH_CLEAR_VALUE: count=1; break;
        case GL_DEPTH_FUNC: count=1; break;
        case GL_DEPTH_RANGE: count=2; break;
        case GL_DEPTH_TEST: count=1; break;
        case GL_DEPTH_WRITEMASK: count=1; break;
        case GL_DIFFUSE: count=4; break;
        case GL_DITHER: count=1; break;
        case GL_ELEMENT_ARRAY_BUFFER_BINDING: count=1; break;
        case GL_EMISSION: count=4; break;
        case GL_FOG: count=1; break;
        case GL_FOG_COLOR: count=4; break;
        case GL_FOG_DENSITY: count=1; break;
        case GL_FOG_END: count=1; break;
        case GL_FOG_HINT: count=1; break;
        case GL_FOG_MODE: count=1; break;
        case GL_FOG_START: count=1; break;
        case GL_FRONT_FACE: count=1; break;
        case GL_GREEN_BITS: count=1; break;
        case GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES: count=1; break;
        case GL_IMPLEMENTATION_COLOR_READ_TYPE_OES: count=1; break;
        case GL_LIGHT0: count=1; break;
        case GL_LIGHT1: count=1; break;
        case GL_LIGHT2: count=1; break;
        case GL_LIGHT3: count=1; break;
        case GL_LIGHT4: count=1; break;
        case GL_LIGHT5: count=1; break;
        case GL_LIGHT6: count=1; break;
        case GL_LIGHT7: count=1; break;
        case GL_LIGHTING: count=1; break;
        case GL_LIGHT_MODEL_AMBIENT: count=4; break;
        case GL_LIGHT_MODEL_TWO_SIDE: count=1; break;
        case GL_LINEAR_ATTENUATION: count=1; break;
        case GL_LINE_SMOOTH: count=1; break;
        case GL_LINE_SMOOTH_HINT: count=1; break;
        case GL_LINE_WIDTH: count=1; break;
        case GL_LOGIC_OP_MODE: count=1; break;
        case GL_MATRIX_MODE: count=1; break;
        case GL_MAX_CLIP_PLANES: count=1; break;
        case GL_MAX_LIGHTS: count=1; break;
        case GL_MAX_MODELVIEW_STACK_DEPTH: count=1; break;
        case GL_MAX_PROJECTION_STACK_DEPTH: count=1; break;
        case GL_MAX_TEXTURE_SIZE: count=1; break;
        case GL_MAX_TEXTURE_STACK_DEPTH: count=1; break;
        case GL_MAX_TEXTURE_UNITS: count=1; break;
        case GL_MAX_VIEWPORT_DIMS: count=2; break;
        case GL_MODELVIEW_MATRIX: count=16; break;
        case GL_MODELVIEW_STACK_DEPTH: count=1; break;
        case GL_MULTISAMPLE: count=1; break;
        case GL_NORMALIZE: count=1; break;
        case GL_NORMAL_ARRAY: count=1; break;
        case GL_NORMAL_ARRAY_BUFFER_BINDING: count=1; break;
        case GL_NORMAL_ARRAY_STRIDE: count=1; break;
        case GL_NORMAL_ARRAY_TYPE: count=1; break;
        case GL_OPERAND0_ALPHA: count=1; break;
        case GL_OPERAND0_RGB: count=1; break;
        case GL_OPERAND1_ALPHA: count=1; break;
        case GL_OPERAND1_RGB: count=1; break;
        case GL_OPERAND2_ALPHA: count=1; break;
        case GL_OPERAND2_RGB: count=1; break;
        case GL_PACK_ALIGNMENT: count=1; break;
        case GL_PERSPECTIVE_CORRECTION_HINT: count=1; break;
        case GL_POINT_DISTANCE_ATTENUATION: count=3; break;
        case GL_POINT_FADE_THRESHOLD_SIZE: count=1; break;
        case GL_POINT_SIZE: count=1; break;
        case GL_POINT_SIZE_ARRAY_BUFFER_BINDING_OES: count = 1; break;
        case GL_POINT_SIZE_ARRAY_OES: count = 1; break;
        case GL_POINT_SIZE_ARRAY_STRIDE_OES: count = 1; break;
        case GL_POINT_SIZE_ARRAY_TYPE_OES: count = 1; break;
        case GL_POINT_SIZE_MAX: count=1; break;
        case GL_POINT_SIZE_MIN: count=1; break;
        case GL_POINT_SMOOTH: count=1; break;
        case GL_POINT_SMOOTH_HINT: count=1; break;
        case GL_POINT_SPRITE_OES: count=1; break;
        case GL_POLYGON_OFFSET_FACTOR: count=1; break;
        case GL_POLYGON_OFFSET_FILL: count=1; break;
        case GL_POLYGON_OFFSET_UNITS: count=1; break;
        case GL_POSITION: count=4; break;
        case GL_PROJECTION_MATRIX: count=16; break;
        case GL_PROJECTION_STACK_DEPTH: count=1; break;
        case GL_QUADRATIC_ATTENUATION: count=1; break;
        case GL_RED_BITS: count=1; break;
        case GL_RESCALE_NORMAL: count=1; break;
        case GL_RGB_SCALE: count=1; break;
        case GL_SAMPLES: count=1; break;
        case GL_SAMPLE_ALPHA_TO_COVERAGE: count=1; break;
        case GL_SAMPLE_ALPHA_TO_ONE: count=1; break;
        case GL_SAMPLE_BUFFERS: count=1; break;
        case GL_SAMPLE_COVERAGE: count=1; break;
        case GL_SAMPLE_COVERAGE_INVERT: count=1; break;
        case GL_SAMPLE_COVERAGE_VALUE: count=1; break;
        case GL_SCISSOR_BOX: count=4; break;
        case GL_SCISSOR_TEST: count=1; break;
        case GL_SHADE_MODEL: count=1; break;
        case GL_SHININESS: count=1; break;
        case GL_SMOOTH_LINE_WIDTH_RANGE: count=2; break;
        case GL_SMOOTH_POINT_SIZE_RANGE: count=2; break;
        case GL_SPECULAR: count=4; break;
        case GL_SPOT_CUTOFF: count=1; break;
        case GL_SPOT_DIRECTION: count=3; break;
        case GL_SPOT_EXPONENT: count=1; break;
        case GL_SRC0_ALPHA: count=1; break;
        case GL_SRC0_RGB: count=1; break;
        case GL_SRC1_ALPHA: count=1; break;
        case GL_SRC1_RGB: count=1; break;
        case GL_SRC2_ALPHA: count=1; break;
        case GL_SRC2_RGB: count=1; break;
        case GL_STENCIL_BITS: count=1; break;
        case GL_STENCIL_CLEAR_VALUE: count=1; break;
        case GL_STENCIL_FAIL: count=1; break;
        case GL_STENCIL_FUNC: count=1; break;
        case GL_STENCIL_PASS_DEPTH_FAIL: count=1; break;
        case GL_STENCIL_PASS_DEPTH_PASS: count=1; break;
        case GL_STENCIL_REF: count=1; break;
        case GL_STENCIL_TEST: count=1; break;
        case GL_STENCIL_VALUE_MASK: count=1; break;
        case GL_STENCIL_WRITEMASK: count=1; break;
        case GL_SUBPIXEL_BITS: count=1; break;
        case GL_TEXTURE_2D: count=1; break;
        case GL_TEXTURE_BINDING_2D: count=1; break;
        case GL_TEXTURE_COORD_ARRAY: count=1; break;
        case GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING: count=1; break;
        case GL_TEXTURE_COORD_ARRAY_SIZE: count=1; break;
        case GL_TEXTURE_COORD_ARRAY_STRIDE: count=1; break;
        case GL_TEXTURE_COORD_ARRAY_TYPE: count=1; break;
        case GL_TEXTURE_ENV_COLOR: count=4; break;
        case GL_TEXTURE_ENV_MODE: count=1; break;
        case GL_TEXTURE_MATRIX: count=16; break;
        case GL_TEXTURE_STACK_DEPTH: count=1; break;
        case GL_UNPACK_ALIGNMENT: count=1; break;
        case GL_VERTEX_ARRAY: count=1; break;
        case GL_VERTEX_ARRAY_BUFFER_BINDING: count=1; break;
        case GL_VERTEX_ARRAY_SIZE: count=1; break;
        case GL_VERTEX_ARRAY_STRIDE: count=1; break;
        case GL_VERTEX_ARRAY_TYPE: count=1; break;
        case GL_VIEWPORT: count=4; break;
        default:
            GLES2_PRINT("ERROR: Unknown pname 0x%x in GLES1 GetCount!\n", pname);
            count = 1;
            break;
    }

    GLES2_PRINT("GLES1 GetCount(0x%x) -> %u!\n", pname, count);

    return count;
}

#include "gles2_escommon_calls.c"

GLES2_CB(glGetPointerv)
{
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    GLES2_BARRIER_ARG_NORET;
    Tptr res = 0;
    gles2_Context * ctx = c->context[context_index];
    switch (pname)
    {
    case GL_COLOR_ARRAY_POINTER:
        res = ctx->arrays[gles1_num_glColorPointer].tptr;
        break;
    case GL_NORMAL_ARRAY_POINTER:
        res = ctx->arrays[gles1_num_glNormalPointer].tptr;
        break;
    case GL_TEXTURE_COORD_ARRAY_POINTER:
        res = ctx->arrays[gles1_num_glTexCoordPointer].tptr;
        break;
    case GL_VERTEX_ARRAY_POINTER:
        res = ctx->arrays[gles1_num_glVertexPointer].tptr;
        break;
    case GL_POINT_SIZE_ARRAY_POINTER_OES:
        res = ctx->arrays[gles1_num_glPointSizePointer].tptr;
        break;
    default:
        GLES2_PRINT("ERROR: Unknown pname 0x%x in glGetPointerv!\n", pname);
        break;
    }

    gles2_put_Tptr(s, paramsp, res);
}

GLES2_CB(glEnableClientState)
{
    GLES2_ARG(TGLenum, array);
    GLES2_BARRIER_ARG_NORET;

    gles2_Context * ctx = c->context[context_index];
    switch (array)
    {
    case GL_COLOR_ARRAY:
        ctx->arrays[gles1_num_glColorPointer].enabled = 1;
        break;
    case GL_NORMAL_ARRAY:
        ctx->arrays[gles1_num_glNormalPointer].enabled = 1;
        break;
    case GL_TEXTURE_COORD_ARRAY:
        ctx->arrays[gles1_num_glTexCoordPointer].enabled = 1;
        break;
    case GL_VERTEX_ARRAY:
        ctx->arrays[gles1_num_glVertexPointer].enabled = 1;
        break;
    case GL_POINT_SIZE_ARRAY_OES:
        ctx->arrays[gles1_num_glPointSizePointer].enabled = 1;
        break;
    default:
        GLES2_PRINT("ERROR: Unknown array 0x%x in glEnableClientState!\n", array);
        break;
    }
    hgl.glEnableClientState(array);
}

GLES2_CB(glDisableClientState)
{
    GLES2_ARG(TGLenum, array);
    GLES2_BARRIER_ARG_NORET;

    gles2_Context * ctx = c->context[context_index];
    switch (array)
    {
    case GL_COLOR_ARRAY:
        ctx->arrays[gles1_num_glColorPointer].enabled = 0;
        break;
    case GL_NORMAL_ARRAY:
        ctx->arrays[gles1_num_glNormalPointer].enabled = 0;
        break;
    case GL_TEXTURE_COORD_ARRAY:
        ctx->arrays[gles1_num_glTexCoordPointer].enabled = 0;
        break;
    case GL_VERTEX_ARRAY:
        ctx->arrays[gles1_num_glVertexPointer].enabled = 0;
        break;
    case GL_POINT_SIZE_ARRAY_OES:
        ctx->arrays[gles1_num_glPointSizePointer].enabled = 0;
        break;
    default:
        GLES2_PRINT("ERROR: Unknown array 0x%x in glDisableClientState!\n", array);
        break;
    }

    hgl.glDisableClientState(array);
}


//***************** Has been generated! *******************

GLES2_CB(glAlphaFunc)
{
    GLES2_ARG(TGLenum, func);
    GLES2_ARG(TGLclampf, ref);
    GLES2_BARRIER_ARG_NORET;

    hgl.glAlphaFunc(func, ref);
}

GLES2_CB(glClipPlanef)
{
    GLES2_ARG(TGLenum, plane);
    GLES2_ARG(Tptr, equationp);
    unsigned count = gles1_count_glClipPlanef;
    GLfloat equation [16];
    unsigned i = 0;
    for (i = 0; i < count; ++i) {
        equation[i] = gles2_get_TGLfloat(s, equationp + i*sizeof(TGLfloat));
    }
    GLES2_BARRIER_ARG_NORET;

    hgl.glClipPlanef(plane, equation);
}

GLES2_CB(glColor4f)
{
    GLES2_ARG(TGLfloat, red);
    GLES2_ARG(TGLfloat, green);
    GLES2_ARG(TGLfloat, blue);
    GLES2_ARG(TGLfloat, alpha);
    GLES2_BARRIER_ARG_NORET;

    hgl.glColor4f(red, green, blue, alpha);
}

GLES2_CB(glFogf)
{
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(TGLfloat, param);
    GLES2_BARRIER_ARG_NORET;

    hgl.glFogf(pname, param);
}

GLES2_CB(glFogfv)
{
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    unsigned count = gles2_GetCount(pname);
    GLfloat params [16];
    unsigned i = 0;
    for (i = 0; i < count; ++i) {
        params[i] = gles2_get_TGLfloat(s, paramsp + i*sizeof(TGLfloat));
    }
    GLES2_BARRIER_ARG_NORET;

    hgl.glFogfv(pname, params);
}

GLES2_CB(glFrustumf)
{
    GLES2_ARG(TGLfloat, left);
    GLES2_ARG(TGLfloat, right);
    GLES2_ARG(TGLfloat, bottom);
    GLES2_ARG(TGLfloat, top);
    GLES2_ARG(TGLfloat, zNear);
    GLES2_ARG(TGLfloat, zFar);
    GLES2_BARRIER_ARG_NORET;

    hgl.glFrustumf(left, right, bottom, top, zNear, zFar);
}

GLES2_CB(glGetClipPlanef)
{
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, eqnp);
    GLES2_BARRIER_ARG_NORET;
    unsigned count = gles2_GetCount(pname);
    GLfloat eqn [16];

    hgl.glGetClipPlanef(pname, eqn);
    unsigned i = 0;
    for (i = 0; i < count; ++i) {
        gles2_put_TGLfloat(s, eqnp + i*sizeof(TGLfloat), eqn[i]);
    }
}

GLES2_CB(glGetLightfv)
{
    GLES2_ARG(TGLenum, light);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    GLES2_BARRIER_ARG_NORET;
    unsigned count = gles2_GetCount(pname);
    GLfloat params [16];

    hgl.glGetLightfv(light, pname, params);
    unsigned i = 0;
    for (i = 0; i < count; ++i) {
        gles2_put_TGLfloat(s, paramsp + i*sizeof(TGLfloat), params[i]);
    }
}

GLES2_CB(glGetMaterialfv)
{
    GLES2_ARG(TGLenum, face);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    GLES2_BARRIER_ARG_NORET;
    unsigned count = gles2_GetCount(pname);
    GLfloat params [16];

    hgl.glGetMaterialfv(face, pname, params);
    unsigned i = 0;
    for (i = 0; i < count; ++i) {
        gles2_put_TGLfloat(s, paramsp + i*sizeof(TGLfloat), params[i]);
    }
}

GLES2_CB(glGetTexEnvfv)
{
    GLES2_ARG(TGLenum, env);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    GLES2_BARRIER_ARG_NORET;
    unsigned count = gles2_GetCount(pname);
    GLfloat params [16];

    hgl.glGetTexEnvfv(env, pname, params);
    unsigned i = 0;
    for (i = 0; i < count; ++i) {
        gles2_put_TGLfloat(s, paramsp + i*sizeof(TGLfloat), params[i]);
    }
}

GLES2_CB(glLightModelf)
{
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(TGLfloat, param);
    GLES2_BARRIER_ARG_NORET;

    hgl.glLightModelf(pname, param);
}

GLES2_CB(glLightModelfv)
{
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    GLES2_BARRIER_ARG_NORET;
    unsigned count = gles2_GetCount(pname);
    GLfloat params [16];
    unsigned i = 0;
    for (i = 0; i < count; ++i) {
        params[i] = gles2_get_TGLfloat(s, paramsp + i*sizeof(TGLfloat));
    }

    hgl.glLightModelfv(pname, params);
}

GLES2_CB(glLightf)
{
    GLES2_ARG(TGLenum, light);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(TGLfloat, param);
    GLES2_BARRIER_ARG_NORET;

    hgl.glLightf(light, pname, param);
}

GLES2_CB(glLightfv)
{
    GLES2_ARG(TGLenum, light);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    unsigned count = gles2_GetCount(pname);
    GLfloat params [16];
    unsigned i = 0;
    for (i = 0; i < count; ++i) {
        params[i] = gles2_get_TGLfloat(s, paramsp + i*sizeof(TGLfloat));
    }
    GLES2_BARRIER_ARG_NORET;

    hgl.glLightfv(light, pname, params);
}

GLES2_CB(glLoadMatrixf)
{
    GLES2_ARG(Tptr, mp);
    unsigned count = gles1_count_glLoadMatrixf;
    GLfloat m [16];
    unsigned i = 0;
    for (i = 0; i < count; ++i) {
        m[i] = gles2_get_TGLfloat(s, mp + i*sizeof(TGLfloat));
    }
    GLES2_BARRIER_ARG_NORET;

    hgl.glLoadMatrixf(m);
}

GLES2_CB(glMaterialf)
{
    GLES2_ARG(TGLenum, face);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(TGLfloat, param);
    GLES2_BARRIER_ARG_NORET;

    hgl.glMaterialf(face, pname, param);
}

GLES2_CB(glMaterialfv)
{
    GLES2_ARG(TGLenum, face);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    unsigned count = gles2_GetCount(pname);
    GLfloat params [16];
    unsigned i = 0;
    for (i = 0; i < count; ++i) {
        params[i] = gles2_get_TGLfloat(s, paramsp + i*sizeof(TGLfloat));
    }
    GLES2_BARRIER_ARG_NORET;

    hgl.glMaterialfv(face, pname, params);
}

GLES2_CB(glMultMatrixf)
{
    GLES2_ARG(Tptr, mp);
    unsigned count = gles1_count_glMultMatrixf;
    GLfloat m [16];
    unsigned i = 0;
    for (i = 0; i < count; ++i) {
        m[i] = gles2_get_TGLfloat(s, mp + i*sizeof(TGLfloat));
    }
    GLES2_BARRIER_ARG_NORET;

    hgl.glMultMatrixf(m);
}

GLES2_CB(glMultiTexCoord4f)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLfloat, ps);
    GLES2_ARG(TGLfloat, t);
    GLES2_ARG(TGLfloat, rr);
    GLES2_ARG(TGLfloat, q);
    GLES2_BARRIER_ARG_NORET;

    hgl.glMultiTexCoord4f(target, ps, t, rr, q);
}

GLES2_CB(glNormal3f)
{
    GLES2_ARG(TGLfloat, nx);
    GLES2_ARG(TGLfloat, ny);
    GLES2_ARG(TGLfloat, nz);
    GLES2_BARRIER_ARG_NORET;

    hgl.glNormal3f(nx, ny, nz);
}

GLES2_CB(glOrthof)
{
    GLES2_ARG(TGLfloat, left);
    GLES2_ARG(TGLfloat, right);
    GLES2_ARG(TGLfloat, bottom);
    GLES2_ARG(TGLfloat, top);
    GLES2_ARG(TGLfloat, zNear);
    GLES2_ARG(TGLfloat, zFar);
    GLES2_BARRIER_ARG_NORET;

    hgl.glOrthof(left, right, bottom, top, zNear, zFar);
}

GLES2_CB(glPointParameterf)
{
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(TGLfloat, param);
    GLES2_BARRIER_ARG_NORET;

    hgl.glPointParameterf(pname, param);
}

GLES2_CB(glPointParameterfv)
{
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    unsigned count = gles2_GetCount(pname);
    GLfloat params [16];
    unsigned i = 0;
    for (i = 0; i < count; ++i) {
        params[i] = gles2_get_TGLfloat(s, paramsp + i*sizeof(TGLfloat));
    }
    GLES2_BARRIER_ARG_NORET;

    hgl.glPointParameterfv(pname, params);
}

GLES2_CB(glPointSize)
{
    GLES2_ARG(TGLfloat, size);
    GLES2_BARRIER_ARG_NORET;

    hgl.glPointSize(size);
}

GLES2_CB(glRotatef)
{
    GLES2_ARG(TGLfloat, angle);
    GLES2_ARG(TGLfloat, x);
    GLES2_ARG(TGLfloat, y);
    GLES2_ARG(TGLfloat, z);
    GLES2_BARRIER_ARG_NORET;

    hgl.glRotatef(angle, x, y, z);
}

GLES2_CB(glScalef)
{
    GLES2_ARG(TGLfloat, x);
    GLES2_ARG(TGLfloat, y);
    GLES2_ARG(TGLfloat, z);
    GLES2_BARRIER_ARG_NORET;

    hgl.glScalef(x, y, z);
}

GLES2_CB(glTexEnvf)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(TGLfloat, param);
    GLES2_BARRIER_ARG_NORET;

    hgl.glTexEnvf(target, pname, param);
}

GLES2_CB(glTexEnvfv)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    unsigned count = gles2_GetCount(pname);
    GLfloat params [16];
    unsigned i = 0;
    for (i = 0; i < count; ++i) {
        params[i] = gles2_get_TGLfloat(s, paramsp + i*sizeof(TGLfloat));
    }
    GLES2_BARRIER_ARG_NORET;

    hgl.glTexEnvfv(target, pname, params);
}

GLES2_CB(glTranslatef)
{
    GLES2_ARG(TGLfloat, x);
    GLES2_ARG(TGLfloat, y);
    GLES2_ARG(TGLfloat, z);
    GLES2_BARRIER_ARG_NORET;

    hgl.glTranslatef(x, y, z);
}

GLES2_CB(glAlphaFuncx)
{
    GLES2_ARG(TGLenum, func);
    GLES2_ARG(TGLclampx, ref);
    GLES2_BARRIER_ARG_NORET;

    hgl.glAlphaFuncx(func, ref);
}

GLES2_CB(glClearColorx)
{
    GLES2_ARG(TGLclampx, red);
    GLES2_ARG(TGLclampx, green);
    GLES2_ARG(TGLclampx, blue);
    GLES2_ARG(TGLclampx, alpha);
    GLES2_BARRIER_ARG_NORET;

    hgl.glClearColorx(red, green, blue, alpha);
}

GLES2_CB(glClearDepthx)
{
    GLES2_ARG(TGLclampx, depth);
    GLES2_BARRIER_ARG_NORET;

    hgl.glClearDepthx(depth);
}

GLES2_CB(glClientActiveTexture)
{
    GLES2_ARG(TGLenum, texture);
    GLES2_BARRIER_ARG_NORET;

    hgl.glClientActiveTexture(texture);
}

GLES2_CB(glClipPlanex)
{
    GLES2_ARG(TGLenum, plane);
    GLES2_ARG(Tptr, equationp);
    unsigned count = gles1_count_glClipPlanex;
    GLfixed equation [16];
    unsigned i = 0;
    for (i = 0; i < count; ++i) {
        equation[i] = gles2_get_TGLfixed(s, equationp + i*sizeof(TGLfixed));
    }
    GLES2_BARRIER_ARG_NORET;

    hgl.glClipPlanex(plane, equation);
}

GLES2_CB(glColor4ub)
{
    GLES2_ARG(TGLubyte, red);
    GLES2_ARG(TGLubyte, green);
    GLES2_ARG(TGLubyte, blue);
    GLES2_ARG(TGLubyte, alpha);
    GLES2_BARRIER_ARG_NORET;

    hgl.glColor4ub(red, green, blue, alpha);
}

GLES2_CB(glColor4x)
{
    GLES2_ARG(TGLfixed, red);
    GLES2_ARG(TGLfixed, green);
    GLES2_ARG(TGLfixed, blue);
    GLES2_ARG(TGLfixed, alpha);
    GLES2_BARRIER_ARG_NORET;

    hgl.glColor4x(red, green, blue, alpha);
}

GLES2_CB(glColorPointer)
{
    GLES2_ARG(TGLint, size);
    GLES2_ARG(TGLenum, type);
    GLES2_ARG(TGLsizei, stride);
    GLES2_ARG(Tptr, pointerp);
    GLES2_BARRIER_ARG_NORET;

    GLES2_PRINT("Array glColorPointer at 0x%x\n", pointerp);

    gles2_Context * ctx = c->context[context_index];

    gles2_Array *va = ctx->arrays + (gles1_num_glColorPointer);
    va->size = size;
    va->type = type;
    va->stride = stride;
    va->tptr = pointerp;
    va->apply = gles1_apply_glColorPointer;
    va->enabled = 1;
}

GLES2_CB(glDepthRangex)
{
    GLES2_ARG(TGLclampx, zNear);
    GLES2_ARG(TGLclampx, zFar);
    GLES2_BARRIER_ARG_NORET;

    hgl.glDepthRangex(zNear, zFar);
}

GLES2_CB(glFogx)
{
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(TGLfixed, param);
    GLES2_BARRIER_ARG_NORET;

    hgl.glFogx(pname, param);
}

GLES2_CB(glFogxv)
{
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    unsigned count = gles2_GetCount(pname);
    GLfixed params [16];
    unsigned i = 0;
    for (i = 0; i < count; ++i) {
        params[i] = gles2_get_TGLfixed(s, paramsp + i*sizeof(TGLfixed));
    }
    GLES2_BARRIER_ARG_NORET;

    hgl.glFogxv(pname, params);
}

GLES2_CB(glFrustumx)
{
    GLES2_ARG(TGLfixed, left);
    GLES2_ARG(TGLfixed, right);
    GLES2_ARG(TGLfixed, bottom);
    GLES2_ARG(TGLfixed, top);
    GLES2_ARG(TGLfixed, zNear);
    GLES2_ARG(TGLfixed, zFar);
    GLES2_BARRIER_ARG_NORET;

    hgl.glFrustumx(left, right, bottom, top, zNear, zFar);
}

GLES2_CB(glGetClipPlanex)
{
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, eqnp);
    GLES2_BARRIER_ARG_NORET;
    unsigned count = gles2_GetCount(pname);
    GLfixed eqn [16];

    hgl.glGetClipPlanex(pname, eqn);
    unsigned i = 0;
    for (i = 0; i < count; ++i) {
        gles2_put_TGLfixed(s, eqnp + i*sizeof(TGLfixed), eqn[i]);
    }
}

GLES2_CB(glGetFixedv)
{
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    GLES2_BARRIER_ARG_NORET;
    unsigned count = gles2_GetCount(pname);
    GLfixed params [16];

    hgl.glGetFixedv(pname, params);
    unsigned i = 0;
    for (i = 0; i < count; ++i) {
        gles2_put_TGLfixed(s, paramsp + i*sizeof(TGLfixed), params[i]);
    }
}

GLES2_CB(glGetLightxv)
{
    GLES2_ARG(TGLenum, light);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    GLES2_BARRIER_ARG_NORET;
    unsigned count = gles2_GetCount(pname);
    GLfixed params [16];

    hgl.glGetLightxv(light, pname, params);
    unsigned i = 0;
    for (i = 0; i < count; ++i) {
        gles2_put_TGLfixed(s, paramsp + i*sizeof(TGLfixed), params[i]);
    }
}

GLES2_CB(glGetMaterialxv)
{
    GLES2_ARG(TGLenum, face);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    GLES2_BARRIER_ARG_NORET;
    unsigned count = gles2_GetCount(pname);
    GLfixed params [16];

    hgl.glGetMaterialxv(face, pname, params);
    unsigned i = 0;
    for (i = 0; i < count; ++i) {
        gles2_put_TGLfixed(s, paramsp + i*sizeof(TGLfixed), params[i]);
    }
}

GLES2_CB(glGetTexEnviv)
{
    GLES2_ARG(TGLenum, env);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    GLES2_BARRIER_ARG_NORET;
    unsigned count = gles2_GetCount(pname);
    GLint params [16];

    hgl.glGetTexEnviv(env, pname, params);
    unsigned i = 0;
    for (i = 0; i < count; ++i) {
        gles2_put_TGLint(s, paramsp + i*sizeof(TGLint), params[i]);
    }
}

GLES2_CB(glGetTexEnvxv)
{
    GLES2_ARG(TGLenum, env);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    GLES2_BARRIER_ARG_NORET;
    unsigned count = gles2_GetCount(pname);
    GLfixed params [16];

    hgl.glGetTexEnvxv(env, pname, params);
    unsigned i = 0;
    for (i = 0; i < count; ++i) {
        gles2_put_TGLfixed(s, paramsp + i*sizeof(TGLfixed), params[i]);
    }
}

GLES2_CB(glGetTexParameterxv)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    GLES2_BARRIER_ARG_NORET;
    unsigned count = gles2_GetCount(pname);
    GLfixed params [16];

    hgl.glGetTexParameterxv(target, pname, params);
    unsigned i = 0;
    for (i = 0; i < count; ++i) {
        gles2_put_TGLfixed(s, paramsp + i*sizeof(TGLfixed), params[i]);
    }
}

GLES2_CB(glLightModelx)
{
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(TGLfixed, param);
    GLES2_BARRIER_ARG_NORET;

    hgl.glLightModelx(pname, param);
}

GLES2_CB(glLightModelxv)
{
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    unsigned count = gles2_GetCount(pname);
    GLfixed params [16];
    unsigned i = 0;
    for (i = 0; i < count; ++i) {
        params[i] = gles2_get_TGLfixed(s, paramsp + i*sizeof(TGLfixed));
    }
    GLES2_BARRIER_ARG_NORET;

    hgl.glLightModelxv(pname, params);
}

GLES2_CB(glLightx)
{
    GLES2_ARG(TGLenum, light);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(TGLfixed, param);
    GLES2_BARRIER_ARG_NORET;

    hgl.glLightx(light, pname, param);
}

GLES2_CB(glLightxv)
{
    GLES2_ARG(TGLenum, light);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    unsigned count = gles2_GetCount(pname);
    GLfixed params [16];
    unsigned i = 0;
    for (i = 0; i < count; ++i) {
        params[i] = gles2_get_TGLfixed(s, paramsp + i*sizeof(TGLfixed));
    }
    GLES2_BARRIER_ARG_NORET;

    hgl.glLightxv(light, pname, params);
}

GLES2_CB(glLineWidthx)
{
    GLES2_ARG(TGLfixed, width);
    GLES2_BARRIER_ARG_NORET;

    hgl.glLineWidthx(width);
}

GLES2_CB(glLoadIdentity)
{
    GLES2_BARRIER_ARG_NORET;

    hgl.glLoadIdentity();
}

GLES2_CB(glLoadMatrixx)
{
    GLES2_ARG(Tptr, mp);
    unsigned count = gles1_count_glLoadMatrixx;
    GLfixed m [16];
    unsigned i = 0;
    for (i = 0; i < count; ++i) {
        m[i] = gles2_get_TGLfixed(s, mp + i*sizeof(TGLfixed));
    }
    GLES2_BARRIER_ARG_NORET;

    hgl.glLoadMatrixx(m);
}

GLES2_CB(glLogicOp)
{
    GLES2_ARG(TGLenum, opcode);
    GLES2_BARRIER_ARG_NORET;

    hgl.glLogicOp(opcode);
}

GLES2_CB(glMaterialx)
{
    GLES2_ARG(TGLenum, face);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(TGLfixed, param);
    GLES2_BARRIER_ARG_NORET;

    hgl.glMaterialx(face, pname, param);
}

GLES2_CB(glMaterialxv)
{
    GLES2_ARG(TGLenum, face);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    unsigned count = gles2_GetCount(pname);
    GLfixed params [16];
    unsigned i = 0;
    for (i = 0; i < count; ++i) {
        params[i] = gles2_get_TGLfixed(s, paramsp + i*sizeof(TGLfixed));
    }
    GLES2_BARRIER_ARG_NORET;

    hgl.glMaterialxv(face, pname, params);
}

GLES2_CB(glMatrixMode)
{
    GLES2_ARG(TGLenum, mode);
    GLES2_BARRIER_ARG_NORET;

    hgl.glMatrixMode(mode);
}

GLES2_CB(glMultMatrixx)
{
    GLES2_ARG(Tptr, mp);
    unsigned count = gles1_count_glMultMatrixx;
    GLfixed m [16];
    unsigned i = 0;
    for (i = 0; i < count; ++i) {
        m[i] = gles2_get_TGLfixed(s, mp + i*sizeof(TGLfixed));
    }
    GLES2_BARRIER_ARG_NORET;

    hgl.glMultMatrixx(m);
}

GLES2_CB(glMultiTexCoord4x)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLfixed, ps);
    GLES2_ARG(TGLfixed, t);
    GLES2_ARG(TGLfixed, rr);
    GLES2_ARG(TGLfixed, q);
    GLES2_BARRIER_ARG_NORET;

    hgl.glMultiTexCoord4x(target, ps, t, rr, q);
}

GLES2_CB(glNormal3x)
{
    GLES2_ARG(TGLfixed, nx);
    GLES2_ARG(TGLfixed, ny);
    GLES2_ARG(TGLfixed, nz);
    GLES2_BARRIER_ARG_NORET;

    hgl.glNormal3x(nx, ny, nz);
}

GLES2_CB(glNormalPointer)
{
    GLES2_ARG(TGLenum, type);
    GLES2_ARG(TGLsizei, stride);
    GLES2_ARG(Tptr, pointerp);
    GLES2_BARRIER_ARG_NORET;

    gles2_Context * ctx = c->context[context_index];
    GLES2_PRINT("Array glNormalPointer at 0x%x\n", pointerp);

    gles2_Array *va = ctx->arrays + gles1_num_glNormalPointer;
    va->type = type;
    va->stride = stride;
    va->tptr = pointerp;
    va->size = gles1_size_glNormalPointer;
    va->apply = gles1_apply_glNormalPointer;
    va->enabled = 1;
}

GLES2_CB(glOrthox)
{
    GLES2_ARG(TGLfixed, left);
    GLES2_ARG(TGLfixed, right);
    GLES2_ARG(TGLfixed, bottom);
    GLES2_ARG(TGLfixed, top);
    GLES2_ARG(TGLfixed, zNear);
    GLES2_ARG(TGLfixed, zFar);
    GLES2_BARRIER_ARG_NORET;

    hgl.glOrthox(left, right, bottom, top, zNear, zFar);
}

GLES2_CB(glPointParameterx)
{
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(TGLfixed, param);
    GLES2_BARRIER_ARG_NORET;

    hgl.glPointParameterx(pname, param);
}

GLES2_CB(glPointParameterxv)
{
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    unsigned count = gles2_GetCount(pname);
    GLfixed params [16];
    unsigned i = 0;
    for (i = 0; i < count; ++i) {
        params[i] = gles2_get_TGLfixed(s, paramsp + i*sizeof(TGLfixed));
    }
    GLES2_BARRIER_ARG_NORET;

    hgl.glPointParameterxv(pname, params);
}

GLES2_CB(glPointSizex)
{
    GLES2_ARG(TGLfixed, size);
    GLES2_BARRIER_ARG_NORET;

    hgl.glPointSizex(size);
}

GLES2_CB(glPolygonOffsetx)
{
    GLES2_ARG(TGLfixed, factor);
    GLES2_ARG(TGLfixed, units);
    GLES2_BARRIER_ARG_NORET;

    hgl.glPolygonOffsetx(factor, units);
}

GLES2_CB(glPopMatrix)
{
    GLES2_BARRIER_ARG_NORET;

    hgl.glPopMatrix();
}

GLES2_CB(glPushMatrix)
{
    GLES2_BARRIER_ARG_NORET;

    hgl.glPushMatrix();
}

GLES2_CB(glRotatex)
{
    GLES2_ARG(TGLfixed, angle);
    GLES2_ARG(TGLfixed, x);
    GLES2_ARG(TGLfixed, y);
    GLES2_ARG(TGLfixed, z);
    GLES2_BARRIER_ARG_NORET;

    hgl.glRotatex(angle, x, y, z);
}

GLES2_CB(glSampleCoveragex)
{
    GLES2_ARG(TGLclampx, value);
    GLES2_ARG(TGLboolean, invert);
    GLES2_BARRIER_ARG_NORET;

    hgl.glSampleCoveragex(value, invert);
}

GLES2_CB(glScalex)
{
    GLES2_ARG(TGLfixed, x);
    GLES2_ARG(TGLfixed, y);
    GLES2_ARG(TGLfixed, z);
    GLES2_BARRIER_ARG_NORET;

    hgl.glScalex(x, y, z);
}

GLES2_CB(glShadeModel)
{
    GLES2_ARG(TGLenum, mode);
    GLES2_BARRIER_ARG_NORET;

    hgl.glShadeModel(mode);
}

GLES2_CB(glTexCoordPointer)
{
    GLES2_ARG(TGLint, size);
    GLES2_ARG(TGLenum, type);
    GLES2_ARG(TGLsizei, stride);
    GLES2_ARG(Tptr, pointerp);
    GLES2_BARRIER_ARG_NORET;

    GLES2_PRINT("Array glTexCoordPointer at 0x%x\n", pointerp);

    gles2_Context * ctx = c->context[context_index];
    gles2_Array *va = ctx->arrays + gles1_num_glTexCoordPointer;
    va->size = size;
    va->type = type;
    va->stride = stride;
    va->tptr = pointerp;
    va->apply = gles1_apply_glTexCoordPointer;
    va->enabled = 1;
}

GLES2_CB(glTexEnvi)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(TGLint, param);
    GLES2_BARRIER_ARG_NORET;

    hgl.glTexEnvi(target, pname, param);
}

GLES2_CB(glTexEnvx)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(TGLfixed, param);
    GLES2_BARRIER_ARG_NORET;

    hgl.glTexEnvx(target, pname, param);
}

GLES2_CB(glTexEnviv)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    unsigned count = gles2_GetCount(pname);
    GLint params [16];
    unsigned i = 0;
    for (i = 0; i < count; ++i) {
        params[i] = gles2_get_TGLint(s, paramsp + i*sizeof(TGLint));
    }
    GLES2_BARRIER_ARG_NORET;

    hgl.glTexEnviv(target, pname, params);
}

GLES2_CB(glTexEnvxv)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    unsigned count = gles2_GetCount(pname);
    GLfixed params [16];
    unsigned i = 0;
    for (i = 0; i < count; ++i) {
        params[i] = gles2_get_TGLfixed(s, paramsp + i*sizeof(TGLfixed));
    }
    GLES2_BARRIER_ARG_NORET;

    hgl.glTexEnvxv(target, pname, params);
}

GLES2_CB(glTexParameterx)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(TGLfixed, param);
    GLES2_BARRIER_ARG_NORET;

    hgl.glTexParameterx(target, pname, param);
}

GLES2_CB(glTexParameterxv)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    unsigned count = gles2_GetCount(pname);
    GLfixed params [16];
    unsigned i = 0;
    for (i = 0; i < count; ++i) {
        params[i] = gles2_get_TGLfixed(s, paramsp + i*sizeof(TGLfixed));
    }
    GLES2_BARRIER_ARG_NORET;

    hgl.glTexParameterxv(target, pname, params);
}

GLES2_CB(glTranslatex)
{
    GLES2_ARG(TGLfixed, x);
    GLES2_ARG(TGLfixed, y);
    GLES2_ARG(TGLfixed, z);
    GLES2_BARRIER_ARG_NORET;

    hgl.glTranslatex(x, y, z);
}

GLES2_CB(glVertexPointer)
{
    GLES2_ARG(TGLint, size);
    GLES2_ARG(TGLenum, type);
    GLES2_ARG(TGLsizei, stride);
    GLES2_ARG(Tptr, pointerp);
    GLES2_BARRIER_ARG_NORET;

    gles2_Context * ctx = c->context[context_index];
    GLES2_PRINT("Array glVertexPointer at 0x%x\n", pointerp);

    gles2_Array *va = ctx->arrays + gles1_num_glVertexPointer;
    va->size = size;
    va->type = type;
    va->stride = stride;
    va->tptr = pointerp;
    va->apply = gles1_apply_glVertexPointer;
    va->enabled = 1;
}

GLES2_CB(glPointSizePointerOES)
{
    GLES2_ARG(TGLenum, type);
    GLES2_ARG(TGLsizei, stride);
    GLES2_ARG(Tptr, pointerp);
    GLES2_BARRIER_ARG_NORET;
    
    gles2_Context * ctx = c->context[context_index];
    GLES2_PRINT("Array glPointSizePointerOES at 0x%x\n", pointerp);
    
    gles2_Array *va = ctx->arrays + gles1_num_glPointSizePointer;
    va->size = gles1_size_glPointSizePointer;
    va->type = type;
    va->stride = stride;
    va->tptr = pointerp;
    va->apply = gles1_apply_glPointSizePointer;
    va->enabled = 1;
}
