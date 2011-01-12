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
#define gles1_num_glColorPointer    1
#define gles1_num_glNormalPointer   2
#define gles1_num_glTexCoordPointer 3
#define gles1_num_glVertexPointer   4

#define gles1_size_glNormalPointer 3
static void gles1_apply_glVertexPointer(gles2_Array *va);
static void gles1_apply_glTexCoordPointer(gles2_Array *va);
static void gles1_apply_glNormalPointer(gles2_Array *va);
static void gles1_apply_glColorPointer(gles2_Array *va);

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



//for debug only
void checkGLESError(void)
{
    GLenum error;
    if ((error = hgl.glGetError()) != 0) {
        GLES2_PRINT("Error after call 0x%x!\n", error);
    }
    else {
        GLES2_PRINT("Ok after call!\n");
    }
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


unsigned __attribute__((const)) gles1_glGetCount(TGLenum pname)
{
    unsigned count;
    switch(pname) {
        case GL_ACTIVE_TEXTURE:
        case GL_ALPHA_BITS:
        case GL_ALPHA_SCALE:
        case GL_ALPHA_TEST:
        case GL_ALPHA_TEST_FUNC:
        case GL_ALPHA_TEST_REF:
        case GL_BLEND:
        case GL_BLEND_DST:
        case GL_BLEND_SRC:
        case GL_BLUE_BITS:
        case GL_CLIENT_ACTIVE_TEXTURE:
        case GL_CLIP_PLANE0:
        case GL_CLIP_PLANE1:
        case GL_CLIP_PLANE2:
        case GL_CLIP_PLANE3:
        case GL_CLIP_PLANE4:
        case GL_CLIP_PLANE5:
        case GL_COLOR_MATERIAL:
        case GL_CULL_FACE:
        case GL_CULL_FACE_MODE:
        case GL_DEPTH_BITS:
        case GL_DEPTH_CLEAR_VALUE:
        case GL_DEPTH_FUNC:
        case GL_DEPTH_TEST:
        case GL_DEPTH_WRITEMASK:
        case GL_DITHER:
        case GL_FOG:
        case GL_FOG_DENSITY:
        case GL_FOG_END:
        case GL_FOG_HINT:
        case GL_FOG_MODE:
        case GL_FOG_START:
        case GL_FRONT_FACE:
        case GL_GREEN_BITS:
        case GL_LIGHT0:
        case GL_LIGHT1:
        case GL_LIGHT2:
        case GL_LIGHT3:
        case GL_LIGHT4:
        case GL_LIGHT5:
        case GL_LIGHT6:
        case GL_LIGHT7:
        case GL_LIGHTING:
        case GL_LIGHT_MODEL_TWO_SIDE:
        case GL_LINE_SMOOTH:
        case GL_LINE_SMOOTH_HINT:
        case GL_LINE_WIDTH:
        case GL_COLOR_LOGIC_OP:
        case GL_LOGIC_OP_MODE:
        case GL_MATRIX_MODE:
        case GL_MAX_CLIP_PLANES:
        case GL_MAX_LIGHTS:
        case GL_MAX_MODELVIEW_STACK_DEPTH:
        case GL_MAX_PROJECTION_STACK_DEPTH:
        case GL_MAX_TEXTURE_SIZE:
        case GL_MAX_TEXTURE_STACK_DEPTH:
        case GL_MAX_TEXTURE_UNITS:
        case GL_MODELVIEW_STACK_DEPTH:
        case GL_MULTISAMPLE:
        case GL_NORMALIZE:
        case GL_PACK_ALIGNMENT:
        case GL_PERSPECTIVE_CORRECTION_HINT:
        case GL_POINT_SIZE:
        case GL_POINT_SMOOTH:
        case GL_POINT_SMOOTH_HINT:
        case GL_POLYGON_OFFSET_FACTOR:
        case GL_POLYGON_OFFSET_UNITS:
        case GL_POLYGON_OFFSET_FILL:
        case GL_PROJECTION_STACK_DEPTH:
        case GL_RED_BITS:
        case GL_RESCALE_NORMAL:
        case GL_SCISSOR_TEST:
        case GL_SHADE_MODEL:
        case GL_STENCIL_BITS:
        case GL_STENCIL_CLEAR_VALUE:
        case GL_STENCIL_FAIL:
        case GL_STENCIL_FUNC:
        case GL_STENCIL_PASS_DEPTH_FAIL:
        case GL_STENCIL_PASS_DEPTH_PASS:
        case GL_STENCIL_REF:
        case GL_STENCIL_TEST:
        case GL_STENCIL_VALUE_MASK:
        case GL_STENCIL_WRITEMASK:
        case GL_SUBPIXEL_BITS:
        case GL_TEXTURE_2D:
        case GL_TEXTURE_BINDING_2D:
        case GL_TEXTURE_STACK_DEPTH:
        case GL_UNPACK_ALIGNMENT:
        case GL_VERTEX_ARRAY:
        case GL_VERTEX_ARRAY_SIZE:
        case GL_VERTEX_ARRAY_TYPE:
        case GL_VERTEX_ARRAY_STRIDE:
        case GL_NORMAL_ARRAY:
        case GL_NORMAL_ARRAY_TYPE:
        case GL_NORMAL_ARRAY_STRIDE:
        case GL_COLOR_ARRAY:
        case GL_COLOR_ARRAY_SIZE:
        case GL_COLOR_ARRAY_TYPE:
        case GL_COLOR_ARRAY_STRIDE:
        case GL_TEXTURE_COORD_ARRAY:
        case GL_TEXTURE_COORD_ARRAY_SIZE:
        case GL_TEXTURE_COORD_ARRAY_TYPE:
        case GL_TEXTURE_COORD_ARRAY_STRIDE:
        case GL_IMPLEMENTATION_COLOR_READ_TYPE_OES:
        case GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES:
        case GL_SPOT_EXPONENT:
        case GL_SPOT_CUTOFF:
        case GL_CONSTANT_ATTENUATION:
        case GL_LINEAR_ATTENUATION:
        case GL_QUADRATIC_ATTENUATION:
        case GL_SHININESS:
        case GL_TEXTURE_ENV_MODE:
        case GL_COMBINE_RGB:
        case GL_COMBINE_ALPHA:
        case GL_SRC0_RGB:
        case GL_SRC1_RGB:
        case GL_SRC2_RGB:
        case GL_SRC0_ALPHA:
        case GL_SRC1_ALPHA:
        case GL_SRC2_ALPHA:
        case GL_OPERAND0_RGB:
        case GL_OPERAND1_RGB:
        case GL_OPERAND2_RGB:
        case GL_OPERAND0_ALPHA:
        case GL_OPERAND1_ALPHA:
        case GL_OPERAND2_ALPHA:
        case GL_RGB_SCALE:
        case GL_POINT_SIZE_MIN:
        case GL_POINT_SIZE_MAX:
        case GL_POINT_FADE_THRESHOLD_SIZE:
        case GL_SAMPLE_ALPHA_TO_COVERAGE:
        case GL_SAMPLE_ALPHA_TO_ONE:
        case GL_SAMPLE_COVERAGE:
        case GL_SAMPLE_COVERAGE_VALUE:
        case GL_SAMPLE_COVERAGE_INVERT:
        case GL_SAMPLE_BUFFERS:
        case GL_SAMPLES:
        case GL_POINT_SPRITE_OES:
        case GL_ARRAY_BUFFER_BINDING:
        case GL_VERTEX_ARRAY_BUFFER_BINDING:
        case GL_NORMAL_ARRAY_BUFFER_BINDING:
        case GL_COLOR_ARRAY_BUFFER_BINDING:
        case GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING:
        case GL_ELEMENT_ARRAY_BUFFER_BINDING:

          count = 1;
          break;

        case GL_DEPTH_RANGE:
        case GL_ALIASED_LINE_WIDTH_RANGE:
        case GL_MAX_VIEWPORT_DIMS:
        case GL_ALIASED_POINT_SIZE_RANGE:
        case GL_SMOOTH_LINE_WIDTH_RANGE:
        case GL_SMOOTH_POINT_SIZE_RANGE:
          count = 2;
          break;

        case GL_CURRENT_NORMAL:
        case GL_SPOT_DIRECTION:
        case GL_POINT_DISTANCE_ATTENUATION:
          count = 3;
          break;

        case GL_COLOR_CLEAR_VALUE:
        case GL_COLOR_WRITEMASK:
        case GL_CURRENT_COLOR:
        case GL_CURRENT_TEXTURE_COORDS:
        case GL_FOG_COLOR:
        case GL_LIGHT_MODEL_AMBIENT:
        case GL_SCISSOR_BOX:
        case GL_VIEWPORT:
        case GL_AMBIENT:
        case GL_DIFFUSE:
        case GL_SPECULAR:
        case GL_EMISSION:
        case GL_TEXTURE_ENV_COLOR:
        case GL_POSITION:
        case GL_AMBIENT_AND_DIFFUSE:
          count = 4;
          break;


        case GL_MODELVIEW_MATRIX:
        case GL_PROJECTION_MATRIX:
        case GL_TEXTURE_MATRIX:
          count = 16;
          break;

        default:
            //GLES2_PRINT("ERROR: Unknown pname 0x%x in glGet!\n", pname);
            //exit(0);
            count = 1;
            break;
    }

    //GLES2_PRINT("glGet(0x%x) -> %u!\n", pname, count);

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
    unsigned count = gles1_glGetCount(pname);
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
    unsigned count = gles1_glGetCount(pname);
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
    unsigned count = gles1_glGetCount(pname);
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
    unsigned count = gles1_glGetCount(pname);
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
    unsigned count = gles1_glGetCount(pname);
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
    unsigned count = gles1_glGetCount(pname);
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
    unsigned count = gles1_glGetCount(pname);
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
    unsigned count = gles1_glGetCount(pname);
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
    unsigned count = gles1_glGetCount(pname);
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
    unsigned count = gles1_glGetCount(pname);
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
    unsigned count = gles1_glGetCount(pname);
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
    unsigned count = gles1_glGetCount(pname);
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
    unsigned count = gles1_glGetCount(pname);
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
    unsigned count = gles1_glGetCount(pname);
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
    unsigned count = gles1_glGetCount(pname);
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
    unsigned count = gles1_glGetCount(pname);
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
    unsigned count = gles1_glGetCount(pname);
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
    unsigned count = gles1_glGetCount(pname);
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
    unsigned count = gles1_glGetCount(pname);
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
    unsigned count = gles1_glGetCount(pname);
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
    unsigned count = gles1_glGetCount(pname);
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
    unsigned count = gles1_glGetCount(pname);
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
    unsigned count = gles1_glGetCount(pname);
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
    unsigned count = gles1_glGetCount(pname);
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
    unsigned count = gles1_glGetCount(pname);
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
