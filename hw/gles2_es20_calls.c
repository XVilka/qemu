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
#include "GLES2/gl2.h"
#include "gl2api.h"



/**
* used by each api to know it's context index
* each gles2_xx_calls.c assigns a value to their own (static) copy of the variable
* This is used by the functions that access the contexts' arrays
*/

static int context_index = 0;
static HGL hgl;

#undef GLES2_CB
#define GLES2_CB(func) \
    CB(func,es20)


/*********************************************
*
*  OpenGL ES 2.0 helper functions and macros
*
**********************************************/
void gles2_loadHGL(void);
void gles2_loadHGL(void)
{
    const char *libname =
#ifdef __APPLE__
    "libGLESv2.dylib";
#elif defined(_WIN32)
    "GLESv2.dll";
#else
    "libGLESv2.so";
#endif

    void* handle;
    if (!(handle = dlopen(libname, RTLD_LOCAL | RTLD_LAZY))) {
        fprintf(stderr, "ERROR: Couldn't load GLESv2 library!\n");
        exit(0);
    }

    #define GLES2_HGL_FUNC(ret,name,attr) \
        if((hgl.name = dlsym(handle,#name))==NULL) \
        { \
            fprintf(stderr, "ES2.0 Function " #name " not found!\n"); \
        }
    GLES2_HGL_FUNCS
    #undef GLES2_HGL_FUNC
}

static void gles2_glApplyVertexAttrib(gles2_Array *va)
{
        hgl.glVertexAttribPointer(va->indx, va->size, va->type,
                                  va->normalized, 0, va->ptr);
            GLenum error = hgl.glGetError();
                if (error != GL_NO_ERROR) {
                            GLES2_PRINT("glVertexAttribPointer(%d, %d, 0x%x, 0, %d, %p\n)"
                                                " failed with 0x%x!\n", va->indx, va->size, va->type,
                                                                    va->normalized, va->ptr, error);
                                }
}


// See if guest offscreen drawable was changed and if so, update host copy.
 int gles2_surface_update(gles2_State *s, gles2_Surface *surf)
{
    int ret = 0;

    uint32_t width   = gles2_get_dword(s, surf->ddrawp + 0*sizeof(uint32_t));
    uint32_t height  = gles2_get_dword(s, surf->ddrawp + 1*sizeof(uint32_t));
    uint32_t depth   = gles2_get_dword(s, surf->ddrawp + 2*sizeof(uint32_t));
    uint32_t bpp     = gles2_get_dword(s, surf->ddrawp + 3*sizeof(uint32_t));
    uint32_t pixelsp = gles2_get_dword(s, surf->ddrawp + 4*sizeof(uint32_t));

    if (width != surf->ddraw.width
         || height != surf->ddraw.height
         || depth != surf->ddraw.depth) {
        surf->ddraw.width = width;
        surf->ddraw.height = height;
        surf->ddraw.depth = depth;
        surf->ddraw.bpp = bpp;
        ret = 1;
    }

    surf->pixelsp = pixelsp;

    return ret;
}

// TODO: Support swapping of offscreen surfaces.
 void gles2_eglSwapCallback(void* userdata)
{
    (void)userdata;
    GLES2_PRINT("Swap called!\n");
}



 unsigned gles2_glGetCount(TGLenum pname)
{
    GLint count;
    switch(pname) {
        case GL_ACTIVE_TEXTURE: count = 1; break;
        case GL_ALIASED_LINE_WIDTH_RANGE: count = 2; break;
        case GL_ALIASED_POINT_SIZE_RANGE: count = 2; break;
        case GL_ALPHA_BITS: count = 1; break;
        case GL_ARRAY_BUFFER_BINDING: count = 1; break;
        case GL_BLEND: count = 1; break;
        case GL_BLEND_COLOR: count = 4; break;
        case GL_BLEND_DST_ALPHA: count = 1; break;
        case GL_BLEND_DST_RGB: count = 1; break;
        case GL_BLEND_EQUATION_ALPHA: count = 1; break;
        case GL_BLEND_EQUATION_RGB: count = 1; break;
        case GL_BLEND_SRC_ALPHA: count = 1; break;
        case GL_BLEND_SRC_RGB: count = 1; break;
        case GL_BLUE_BITS: count = 1; break;
        case GL_COLOR_CLEAR_VALUE: count = 4; break;
        case GL_COLOR_WRITEMASK: count = 4; break;
        case GL_COMPRESSED_TEXTURE_FORMATS: hgl.glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &count); break;
        case GL_CULL_FACE: count = 1; break;
        case GL_CULL_FACE_MODE: count = 1; break;
        case GL_CURRENT_PROGRAM: count = 1; break;
        case GL_DEPTH_BITS: count = 1; break;
        case GL_DEPTH_CLEAR_VALUE: count = 1; break;
        case GL_DEPTH_FUNC: count = 1; break;
        case GL_DEPTH_RANGE: count = 2; break;
        case GL_DEPTH_TEST: count = 1; break;
        case GL_DEPTH_WRITEMASK: count = 1; break;
        case GL_DITHER: count = 1; break;
        case GL_ELEMENT_ARRAY_BUFFER_BINDING: count = 1; break;
        case GL_FRAMEBUFFER_BINDING: count = 1; break;
        case GL_FRONT_FACE: count = 1; break;
        case GL_GENERATE_MIPMAP_HINT: count = 1; break;
        case GL_GREEN_BITS: count = 1; break;
        case GL_IMPLEMENTATION_COLOR_READ_FORMAT: count = 1; break;
        case GL_IMPLEMENTATION_COLOR_READ_TYPE: count = 1; break;
        case GL_LINE_WIDTH: count = 1; break;
        case GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS: count = 1; break;
        case GL_MAX_CUBE_MAP_TEXTURE_SIZE: count = 1; break;
        case GL_MAX_FRAGMENT_UNIFORM_VECTORS: count = 1; break;
        case GL_MAX_RENDERBUFFER_SIZE: count = 1; break;
        case GL_MAX_TEXTURE_IMAGE_UNITS: count = 1; break;
        case GL_MAX_TEXTURE_SIZE: count = 1; break;
        case GL_MAX_VARYING_VECTORS: count = 1; break;
        case GL_MAX_VERTEX_ATTRIBS: count = 1; break;
        case GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS: count = 1; break;
        case GL_MAX_VERTEX_UNIFORM_VECTORS: count = 1; break;
        case GL_MAX_VIEWPORT_DIMS: count = 2; break;
        case GL_NUM_COMPRESSED_TEXTURE_FORMATS: count = 1; break;
        case GL_NUM_SHADER_BINARY_FORMATS: count = 1; break;
        case GL_PACK_ALIGNMENT: count = 1; break;
        case GL_POLYGON_OFFSET_FACTOR: count = 1; break;
        case GL_POLYGON_OFFSET_FILL: count = 1; break;
        case GL_POLYGON_OFFSET_UNITS: count = 1; break;
        case GL_RED_BITS: count = 1; break;
        case GL_RENDERBUFFER_BINDING: count = 1; break;
        case GL_SAMPLE_BUFFERS: count = 1; break;
        case GL_SAMPLE_COVERAGE_INVERT: count = 1; break;
        case GL_SAMPLE_COVERAGE_VALUE: count = 1; break;
        case GL_SAMPLES: count = 1; break;
        case GL_SCISSOR_BOX: count = 4; break;
        case GL_SCISSOR_TEST: count = 1; break;
        case GL_SHADER_BINARY_FORMATS: hgl.glGetIntegerv(GL_NUM_SHADER_BINARY_FORMATS, &count); break;
        case GL_SHADER_COMPILER: count = 1; break;
        case GL_STENCIL_BACK_FAIL: count = 1; break;
        case GL_STENCIL_BACK_FUNC: count = 1; break;
        case GL_STENCIL_BACK_PASS_DEPTH_FAIL: count = 1; break;
        case GL_STENCIL_BACK_PASS_DEPTH_PASS: count = 1; break;
        case GL_STENCIL_BACK_REF: count = 1; break;
        case GL_STENCIL_BACK_VALUE_MASK: count = 1; break;
        case GL_STENCIL_BACK_WRITEMASK: count = 1; break;
        case GL_STENCIL_BITS: count = 1; break;
        case GL_STENCIL_CLEAR_VALUE: count = 1; break;
        case GL_STENCIL_FAIL: count = 1; break;
        case GL_STENCIL_FUNC: count = 1; break;
        case GL_STENCIL_PASS_DEPTH_FAIL: count = 1; break;
        case GL_STENCIL_PASS_DEPTH_PASS: count = 1; break;
        case GL_STENCIL_REF: count = 1; break;
        case GL_STENCIL_TEST: count = 1; break;
        case GL_STENCIL_VALUE_MASK: count = 1; break;
        case GL_STENCIL_WRITEMASK: count = 1; break;
        case GL_SUBPIXEL_BITS: count = 1; break;
        case GL_TEXTURE_BINDING_2D: count = 1; break;
        case GL_TEXTURE_BINDING_CUBE_MAP: count = 1; break;
        case GL_UNPACK_ALIGNMENT: count = 1; break;
        case GL_VIEWPORT: count = 4; break;
        default:
            GLES2_PRINT("ERROR: Unknown pname 0x%x in glGet!\n", pname);
            count = 1;
            break;
    }

    GLES2_PRINT("glGet(0x%x) -> %u!\n", pname, count);

    return (unsigned)count;
}

unsigned gles2_glTexParameterCount(TGLenum pname)
{
    unsigned count;

    switch(pname) {
        case GL_TEXTURE_MIN_FILTER: count = 1; break;
        case GL_TEXTURE_MAG_FILTER: count = 1; break;
        case GL_TEXTURE_WRAP_S: count = 1; break;
        case GL_TEXTURE_WRAP_T: count = 1; break;
        default:
            GLES2_PRINT("ERROR: Unknown texture parameter 0x%x!\n", pname);
            count = 1;
            break;
    }

    return count;
}
#include "gles2_escommon_calls.c"

GLES2_CB(glDisableVertexAttribArray)
{
    GLES2_ARG(TGLuint, index);
    GLES2_BARRIER_ARG_NORET;

    gles2_Context * ctx = c->context[context_index];
    ctx->arrays[index].enabled = 0;
    hgl.glDisableVertexAttribArray(index);
}

GLES2_CB(glEnableVertexAttribArray)
{
    GLES2_ARG(TGLuint, index);
    GLES2_BARRIER_ARG_NORET;

    gles2_Context * ctx = c->context[context_index];
    ctx->arrays[index].enabled = 1;
    ctx->arrays[index].apply = gles2_glApplyVertexAttrib;
    hgl.glEnableVertexAttribArray(index);
}

GLES2_CB(glGetVertexAttribfv)
{
    GLES2_ARG(TGLuint, index);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    GLES2_BARRIER_ARG;

    GLfloat params = 0;
    hgl.glGetVertexAttribfv(index, pname, &params);

    GLES2_BARRIER_RET;
    gles2_put_TGLfloat(s, paramsp, params);
}

GLES2_CB(glGetVertexAttribiv)
{
    GLES2_ARG(TGLuint, index);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    GLES2_BARRIER_ARG;

    GLint params = 0;
    hgl.glGetVertexAttribiv(index, pname, &params);

    GLES2_BARRIER_RET;
    gles2_put_TGLint(s, paramsp, params);
}

#if 0
GL_APICALL void GL_APIENTRY hgl.glGetVertexAttribPointerv(GLuint index,
    GLenum pname, void** pointer)
{
    DUMMY();
}
#endif //0

GLES2_CB(glVertexAttrib1f)
{
    GLES2_ARG(TGLuint, indx);
    GLES2_ARG(TGLfloat, x);
    GLES2_BARRIER_ARG_NORET;

    hgl.glVertexAttrib1f(indx, x);
}

GLES2_CB(glVertexAttrib1fv)
{
    GLES2_ARG(TGLuint, indx);
    GLES2_ARG(Tptr, valuesp);
    GLfloat x = gles2_get_float(s, valuesp);
    GLES2_BARRIER_ARG_NORET;

    hgl.glVertexAttrib1f(indx, x);
}

GLES2_CB(glVertexAttrib2f)
{
    GLES2_ARG(TGLuint, indx);
    GLES2_ARG(TGLfloat, x);
    GLES2_ARG(TGLfloat, y);
    GLES2_BARRIER_ARG_NORET;

    hgl.glVertexAttrib2f(indx, x, y);
}

GLES2_CB(glVertexAttrib2fv)
{
    GLES2_ARG(TGLuint, indx);
    GLES2_ARG(Tptr, valuesp);

    GLfloat x = gles2_get_float(s, valuesp);
    GLfloat y = gles2_get_float(s, valuesp + sizeof(TGLfloat));
    GLES2_BARRIER_ARG_NORET;

    hgl.glVertexAttrib2f(indx, x, y);
}

GLES2_CB(glVertexAttrib3f)
{
    GLES2_ARG(TGLuint, indx);
    GLES2_ARG(TGLfloat, x);
    GLES2_ARG(TGLfloat, y);
    GLES2_ARG(TGLfloat, z);
    GLES2_BARRIER_ARG_NORET;

    hgl.glVertexAttrib3f(indx, x, y, z);
}

GLES2_CB(glVertexAttrib3fv)
{
    GLES2_ARG(TGLuint, indx);
    GLES2_ARG(Tptr, valuesp);

    GLfloat x = gles2_get_float(s, valuesp + 0*sizeof(TGLfloat));
    GLfloat y = gles2_get_float(s, valuesp + 1*sizeof(TGLfloat));
    GLfloat z = gles2_get_float(s, valuesp + 2*sizeof(TGLfloat));
    GLES2_BARRIER_ARG_NORET;

    hgl.glVertexAttrib3f(indx, x, y, z);
}

GLES2_CB(glVertexAttrib4f)
{
    GLES2_ARG(TGLuint, indx);
    GLES2_ARG(TGLfloat, x);
    GLES2_ARG(TGLfloat, y);
    GLES2_ARG(TGLfloat, z);
    GLES2_ARG(TGLfloat, w);
    GLES2_BARRIER_ARG_NORET;

    hgl.glVertexAttrib4f(indx, x, y, z, w);
}

GLES2_CB(glVertexAttrib4fv)
{
    GLES2_ARG(TGLuint, indx);
    GLES2_ARG(Tptr, valuesp);

    GLfloat x = gles2_get_float(s, valuesp + 0*sizeof(TGLfloat));
    GLfloat y = gles2_get_float(s, valuesp + 1*sizeof(TGLfloat));
    GLfloat z = gles2_get_float(s, valuesp + 2*sizeof(TGLfloat));
    GLfloat w = gles2_get_float(s, valuesp + 3*sizeof(TGLfloat));
    GLES2_BARRIER_ARG_NORET;

    hgl.glVertexAttrib4f(indx, x, y, z, w);
}

GLES2_CB(glVertexAttribPointer)
{
    GLES2_ARG(TGLuint, indx);
    GLES2_ARG(TGLint, size);
    GLES2_ARG(TGLenum, type);
    GLES2_ARG(TGLboolean, normalized);
    GLES2_ARG(TGLsizei, stride);
    GLES2_ARG(Tptr, tptr);
    GLES2_BARRIER_ARG_NORET;

    GLES2_PRINT("Array %d at 0x%x (%d elements every %d bytes)\n",
        indx, tptr, size, stride);

    gles2_Context * ctx = c->context[context_index];

    gles2_Array *va = ctx->arrays + indx;
    va->type = type;
    va->indx = indx;
    va->size = size;
    va->normalized = normalized;
    va->stride = stride;
    va->tptr = tptr;
}























#if 0
GL_APICALL void GL_APIENTRY hgl.glCompressedTexImage2D(GLenum target,
    GLint level, GLenum internalformat, GLsizei width, GLsizei height,
    GLint border, GLsizei imageSize, const void* data)
{
    DUMMY();
}

GL_APICALL void GL_APIENTRY hgl.glCompressedTexSubImage2D(GLenum target,
    GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
    GLenum format, GLsizei imageSize, const void* data)
{
    DUMMY();
}

GL_APICALL void GL_APIENTRY hgl.glCopyTexImage2D(GLenum target, GLint level,
    GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height,
    GLint border)
{
    DUMMY();
}

GL_APICALL void GL_APIENTRY hgl.glCopyTexSubImage2D(GLenum target, GLint level,
    GLint xoffset, GLint yoffset, GLint x, GLint y,
    GLsizei width, GLsizei height)
{
    DUMMY();
}
#endif // 0


GLES2_CB(glGenerateMipmap)
{
    GLES2_ARG(TGLenum, target);
    GLES2_BARRIER_ARG_NORET;

    hgl.glGenerateMipmap(target);
}















GLES2_CB(glCompileShader)
{
    GLES2_ARG(TGLuint, shader);
    GLES2_BARRIER_ARG_NORET;

    hgl.glCompileShader(shader);
}

GLES2_CB(glCreateShader)
{
    GLES2_ARG(TGLenum, type);
    GLES2_BARRIER_ARG;
    GLuint ret =  hgl.glCreateShader(type);
    GLES2_BARRIER_RET;
    gles2_ret_TGLuint(s,ret );
}

GLES2_CB(glDeleteShader)
{
    GLES2_ARG(TGLuint, shader);
    GLES2_BARRIER_ARG_NORET;

    hgl.glDeleteShader(shader);
}

GLES2_CB(glIsShader)
{
    GLES2_ARG(TGLuint, shader);
    GLES2_BARRIER_ARG;

    GLES2_BARRIER_RET;
    gles2_ret_TGLboolean(s, hgl.glIsShader(shader));
}

GLES2_CB(glGetShaderiv)
{
    GLES2_ARG(TGLuint, shader);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    GLES2_BARRIER_ARG;

    GLint param;
    hgl.glGetShaderiv(shader, pname, &param);
    GLES2_BARRIER_RET;
    gles2_put_TGLint(s, paramsp, param);
}

GLES2_CB(glGetShaderInfoLog)
{
    GLES2_ARG(TGLuint, shader);
    GLES2_ARG(TGLsizei, bufsize);
    GLES2_ARG(Tptr, lengthp);
    GLES2_ARG(Tptr, infologp);

    GLsizei length = gles2_get_TGLsizei(s, lengthp);
    char* infolog = malloc(bufsize);
    hgl.glGetShaderInfoLog(shader, bufsize, &length, infolog);
    gles2_transfer(s, infologp, length, infolog, 1);
    gles2_put_TGLsizei(s, lengthp, length);
    GLES2_BARRIER_ARG_NORET;

    GLES2_PRINT("shader %d infolog:\n%.*s\n", shader, length, infolog);
    free(infolog);
}

#if 0
GL_APICALL void GL_APIENTRY hgl.glGetShaderPrecisionFormat(GLenum shadertype,
    GLenum precisiontype, GLint* range, GLint* precision)
{
    DUMMY();
}

GL_APICALL void GL_APIENTRY hgl.glGetShaderSource(GLuint shader, GLsizei bufsize,
    GLsizei* length, char* source)
{
    DUMMY();
}
#endif // 0

GLES2_CB(glReleaseShaderCompiler)
{
    GLES2_BARRIER_ARG_NORET;
    hgl.glReleaseShaderCompiler();
}

#if 0
GL_APICALL void GL_APIENTRY hgl.glShaderBinary(GLsizei n, const GLuint* shaders,
    GLenum binaryformat, const void* binary, GLsizei length)
{
    DUMMY();
}
#endif // 0

GLES2_CB(glShaderSource)
{
    GLES2_ARG(TGLuint, shader);
    GLES2_ARG(TGLsizei, count);
    GLES2_ARG(Tptr, stringp);
    GLES2_ARG(Tptr, lengthp);

    char** string_fgl = malloc(sizeof(char*)*count);
    GLint* length_fgl = malloc(sizeof(GLint)*count);

    unsigned i;
    for (i = 0; i < count; ++i) {
        length_fgl[i] = gles2_get_TGLint(s, lengthp + i*sizeof(TGLint));
        string_fgl[i] = malloc(length_fgl[i] + 1);
        gles2_transfer(s, gles2_get_dword(s, stringp + i*sizeof(Tptr)),
            length_fgl[i], string_fgl[i], 0);
        string_fgl[i][length_fgl[i]] = 0;
    }
    GLES2_BARRIER_ARG_NORET;

    GLES2_PRINT("shader %d source:\n", shader);
    #if(GLES2_DEBUG == 1)
    for(i = 0; i < count; ++i) {
        fprintf(stderr, "%.*s", length_fgl[i], string_fgl[i]);
    }
    #endif // GLES2_DEBUG == 1
    GLES2_PRINT("\n--END--");

    hgl.glShaderSource(shader, (GLsizei)count,
        (const char**)string_fgl, length_fgl);

    for (i = 0; i < count; ++i) {
        free(string_fgl[i]);
    }

    free(string_fgl);
    free(length_fgl);
}

GLES2_CB(glAttachShader)
{
    GLES2_ARG(TGLuint, program);
    GLES2_ARG(TGLuint, shader);
    GLES2_BARRIER_ARG_NORET;

    hgl.glAttachShader(program, shader);
}

GLES2_CB(glBindAttribLocation)
{
    GLES2_ARG(TGLuint, program);
    GLES2_ARG(TGLuint, index);
    GLES2_ARG(Tptr, namep);

    char name[120];
    Tptr i;

    for(i = 0; (name[i] = gles2_get_byte(s, namep + i)) ; ++i);
    GLES2_BARRIER_ARG_NORET;

    GLES2_PRINT("Binding attribute %s at %d...\n", name, index);
    hgl.glBindAttribLocation(program, index, name);
}




GLES2_CB(glCreateProgram)//(void)
{
    GLES2_BARRIER_ARG;

    GLES2_BARRIER_RET;
    gles2_ret_TGLuint(s, hgl.glCreateProgram());
}

GLES2_CB(glDeleteProgram)//(GLuint program)
{
    GLES2_ARG(TGLuint, program);
    GLES2_BARRIER_ARG_NORET;

    hgl.glDeleteProgram(program);
}

GLES2_CB(glDetachShader)
{
    GLES2_ARG(TGLuint, program);
    GLES2_ARG(TGLuint, shader);
    GLES2_BARRIER_ARG_NORET;

    hgl.glDetachShader(program, shader);
}

GLES2_CB(glGetActiveAttrib)
{
    GLES2_ARG(TGLuint, program);
    GLES2_ARG(TGLuint, index);
    GLES2_ARG(TGLsizei, bufsize);
    GLES2_ARG(Tptr, lengthp);
    GLES2_ARG(Tptr, sizep);
    GLES2_ARG(Tptr, typep);
    GLES2_ARG(Tptr, namep);
    GLES2_BARRIER_ARG;

    char *name = malloc(bufsize);
    GLsizei length = 0;
    GLint size = 0;
    GLenum type = 0;

    hgl.glGetActiveAttrib(program, index, bufsize, &length, &size, &type, name);

    GLES2_BARRIER_RET;
    if (lengthp) {
        gles2_put_TGLsizei(s, lengthp, length);
    }
    gles2_put_TGLint(s, sizep, size);
    gles2_put_TGLenum(s, typep, type);

    Tptr i;
    for (i = 0; i < length; i++) {
        gles2_put_byte(s, namep + i, name[i]);
    }
    if (i < bufsize) {
        gles2_put_byte(s, namep + i, 0);
    }
    free(name);
}

GLES2_CB(glGetActiveUniform)
{
    GLES2_ARG(TGLuint, program);
    GLES2_ARG(TGLuint, index);
    GLES2_ARG(TGLsizei, bufsize);
    GLES2_ARG(Tptr, lengthp); // GLsizei*
    GLES2_ARG(Tptr, sizep); // GLint*
    GLES2_ARG(Tptr, typep); // GLenum*
    GLES2_ARG(Tptr, namep); // char*
    GLES2_BARRIER_ARG;

    char* name = malloc(bufsize);
    GLsizei length = 0;
    GLint size = 0;
    GLenum type = 0;
    Tptr i;

    hgl.glGetActiveUniform(program, index, bufsize, &length, &size, &type, name);

    GLES2_BARRIER_RET;
    if(lengthp)
        gles2_put_TGLsizei(s, lengthp, length);
    gles2_put_TGLint(s, sizep, size);
    gles2_put_TGLenum(s, typep, type);
    (void) (namep + i);

    for (i = 0; i < length; ++i) {
        gles2_put_byte(s, namep + i, name[i]);
    }
    if(i < bufsize)
        gles2_put_byte(s, namep + i, 0);

    free(name);
}

GLES2_CB(glGetAttachedShaders)
{
    GLES2_ARG(TGLuint, program);
    GLES2_ARG(TGLsizei, maxcount);
    GLES2_ARG(Tptr, countp);
    GLES2_ARG(Tptr, shadersp);
    GLES2_BARRIER_ARG;

    GLsizei count = 0;
    GLuint *shaders = malloc(maxcount * sizeof(GLuint));

    hgl.glGetAttachedShaders(program, maxcount, &count, shaders);

    GLES2_BARRIER_RET;
    if (countp) {
        gles2_put_TGLsizei(s, countp, count);
    }

    GLsizei i;
    for (i = 0; i < count; i++, shadersp += sizeof(GLuint)) {
        gles2_put_TGLuint(s, shadersp, shaders[i]);
    }

    free(shaders);
}

GLES2_CB(glGetAttribLocation)
{
    GLES2_ARG(TGLuint, program);
    GLES2_ARG(Tptr, namep);

    char name[120];
    Tptr i;

    for (i = 0; (name[i] = gles2_get_byte(s, namep + i)) ; ++i);

    GLES2_PRINT("Getting attribute %s location...\n", name);

    gles2_ret_TGLint(s, hgl.glGetAttribLocation(program, name));
    GLES2_BARRIER_ARG_NORET;
}

GLES2_CB(glGetProgramiv)
{
    GLES2_ARG(TGLuint, program);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    GLES2_BARRIER_ARG;

    GLint param;

    hgl.glGetProgramiv(program, pname, &param);
    GLES2_BARRIER_RET;
    gles2_put_TGLint(s, paramsp, param);
}

GLES2_CB(glGetProgramInfoLog)
{
    GLES2_ARG(TGLuint, program);
    GLES2_ARG(TGLsizei, bufsize);
    GLES2_ARG(Tptr, lengthp);
    GLES2_ARG(Tptr, infologp);

    GLsizei length = gles2_get_TGLsizei(s, lengthp);
    char* infolog = malloc(bufsize);
    hgl.glGetProgramInfoLog(program, bufsize, &length, infolog);
    gles2_transfer(s, infologp, length, infolog, 1);
    gles2_put_TGLsizei(s, lengthp, length);
    GLES2_BARRIER_ARG_NORET;
    GLES2_PRINT("program %d infolog:\n%.*s\n", program, length, infolog);
    free(infolog);
}

static uint32_t gles2_typesize(GLenum type)
{
    switch (type) {
        case GL_FLOAT: return sizeof(GLfloat);
        case GL_FLOAT_VEC2: return sizeof(GLfloat) * 2;
        case GL_FLOAT_VEC3: return sizeof(GLfloat) * 3;
        case GL_FLOAT_VEC4: return sizeof(GLfloat) * 4;
        case GL_INT: return sizeof(GLint);
        case GL_INT_VEC2: return sizeof(GLint) * 2;
        case GL_INT_VEC3: return sizeof(GLint) * 3;
        case GL_INT_VEC4: return sizeof(GLint) * 4;
        case GL_BOOL: return sizeof(GLboolean);
        case GL_BOOL_VEC2: return sizeof(GLboolean) * 2;
        case GL_BOOL_VEC3: return sizeof(GLboolean) * 3;
        case GL_BOOL_VEC4: return sizeof(GLboolean) * 4;
        case GL_FLOAT_MAT2: return sizeof(GLfloat) * 2 * 2;
        case GL_FLOAT_MAT3: return sizeof(GLfloat) * 3 * 3;
        case GL_FLOAT_MAT4: return sizeof(GLfloat) * 4 * 4;
        //case GL_FLOAT_MAT2x3: return sizeof(GLfloat) * 2 * 3;
        //case GL_FLOAT_MAT2x4: return sizeof(GLfloat) * 2 * 4;
        //case GL_FLOAT_MAT3x2: return sizeof(GLfloat) * 3 * 2;
        //case GL_FLOAT_MAT3x4: return sizeof(GLfloat) * 3 * 4;
        //case GL_FLOAT_MAT4x2: return sizeof(GLfloat) * 4 * 2;
        //case GL_FLOAT_MAT4x3: return sizeof(GLfloat) * 4 * 3;
        //case GL_SAMPLER_1D: return sizeof(GLuint);
        case GL_SAMPLER_2D: return sizeof(GLuint);
        //case GL_SAMPLER_3D: return sizeof(GLuint);
        case GL_SAMPLER_CUBE: return sizeof(GLuint);
        //case GL_SAMPLER_1D_SHADOW: return sizeof(GLuint);
        //case GL_SAMPLER_2D_SHADOW: return sizeof(GLuint);
        default: break;
    }
    GLES2_PRINT("ERROR: unknown GL type 0x%x\n", type);
    return 0;
}

GLES2_CB(glGetUniformfv)
{
    GLES2_ARG(TGLuint, program);
    GLES2_ARG(TGLint, location);
    GLES2_ARG(Tptr, paramsp);
    GLES2_BARRIER_ARG;

    GLint size = 0;
    GLenum type = 0;
    hgl.glGetActiveUniform(program, location, 0, NULL, &size, &type, NULL);
    GLfloat *params = malloc(size * gles2_typesize(type));
    hgl.glGetUniformfv(program, location, params);

    GLES2_BARRIER_RET;
    GLint i;
    for (i = 0; i < size; i++, paramsp += sizeof(TGLfloat)) {
        gles2_put_TGLfloat(s, paramsp, params[i]);
    }
    free(params);
}

GLES2_CB(glGetUniformiv)
{
    GLES2_ARG(TGLuint, program);
    GLES2_ARG(TGLint, location);
    GLES2_ARG(Tptr, paramsp);
    GLES2_BARRIER_ARG;

    GLint size = 0;
    GLenum type = 0;
    hgl.glGetActiveUniform(program, location, 0, NULL, &size, &type, NULL);
    GLint *params = malloc(size * gles2_typesize(type));
    hgl.glGetUniformiv(program, location, params);

    GLES2_BARRIER_RET;
    GLint i;
    for (i = 0; i < size; i++, paramsp += sizeof(TGLint)) {
        gles2_put_TGLint(s, paramsp, params[i]);
    }
    free(params);
}

GLES2_CB(glGetUniformLocation)
{
    GLES2_ARG(TGLuint, program);
    GLES2_ARG(Tptr, namep);

    char name[120];
    Tptr i;

    for (i = 0; (name[i] = gles2_get_byte(s, namep + i)) ; ++i);

    GLES2_PRINT("Getting uniform %s location...\n", name);

    gles2_ret_TGLint(s, hgl.glGetUniformLocation(program, name));
    GLES2_BARRIER_ARG_NORET;
}

GLES2_CB(glIsProgram)
{
    GLES2_ARG(TGLuint, program);
    GLES2_BARRIER_ARG;

    GLES2_BARRIER_RET;
    gles2_ret_TGLboolean(s, hgl.glIsProgram(program));
}

GLES2_CB(glLinkProgram)
{
    GLES2_ARG(TGLuint, program);
    GLES2_BARRIER_ARG_NORET;

    hgl.glLinkProgram(program);
}

GLES2_CB(glUniform1f)
{
    GLES2_ARG(TGLint, location);
    GLES2_ARG(TGLfloat, x);
    GLES2_BARRIER_ARG_NORET;

    hgl.glUniform1f(location, x);
}

GLES2_CB(glUniform1fv)
{
    GLES2_ARG(TGLint, location);
    GLES2_ARG(TGLsizei, count);
    GLES2_ARG(Tptr, vp);

    unsigned nvs = count*1;
    GLfloat* v = malloc(nvs*sizeof(*v));
    unsigned i;

    for (i = 0; i < nvs; ++i) {
        v[i] = gles2_get_TGLfloat(s, vp + i*sizeof(TGLfloat));
    }
    GLES2_BARRIER_ARG_NORET;

    hgl.glUniform1fv(location, count, v);
    free(v);
}

GLES2_CB(glUniform1i)//(GLint location, GLint x)
{
    GLES2_ARG(TGLint, location);
    GLES2_ARG(TGLint, x);
    GLES2_BARRIER_ARG_NORET;

    hgl.glUniform1i(location, x);
}

GLES2_CB(glUniform1iv)
{
    GLES2_ARG(TGLint, location);
    GLES2_ARG(TGLsizei, count);
    GLES2_ARG(Tptr, vp);

    unsigned nvs = count*1;
    GLint* v = malloc(nvs*sizeof(*v));
    unsigned i;
    for (i = 0; i < nvs; ++i) {
        v[i] = gles2_get_TGLint(s, vp + i*sizeof(TGLint));
    }
    GLES2_BARRIER_ARG_NORET;

    hgl.glUniform1iv(location, count, v);
    free(v);
}

GLES2_CB(glUniform2f)
{
    GLES2_ARG(TGLint, location);
    GLES2_ARG(TGLfloat, x);
    GLES2_ARG(TGLfloat, y);
    GLES2_BARRIER_ARG_NORET;

    hgl.glUniform2f(location, x, y);
}

GLES2_CB(glUniform2fv)
{
    GLES2_ARG(TGLint, location);
    GLES2_ARG(TGLsizei, count);
    GLES2_ARG(Tptr, vp);

    unsigned nvs = count*2;
    GLfloat* v = malloc(nvs*sizeof(*v));
    unsigned i;
    for (i = 0; i < nvs; ++i) {
        v[i] = gles2_get_TGLfloat(s, vp + i*sizeof(TGLfloat));
    }
    GLES2_BARRIER_ARG_NORET;

    hgl.glUniform2fv(location, count, v);
    free(v);
}

GLES2_CB(glUniform2i)
{
    GLES2_ARG(TGLint, location);
    GLES2_ARG(TGLint, x);
    GLES2_ARG(TGLint, y);
    GLES2_BARRIER_ARG_NORET;

    hgl.glUniform2i(location, x, y);
}

GLES2_CB(glUniform2iv)
{
    GLES2_ARG(TGLint, location);
    GLES2_ARG(TGLsizei, count);
    GLES2_ARG(Tptr, vp);

    unsigned nvs = count*2;
    GLint* v = malloc(nvs*sizeof(*v));
    unsigned i;
    for (i = 0; i < nvs; ++i) {
        v[i] = gles2_get_TGLint(s, vp + i*sizeof(TGLint));
    }
    GLES2_BARRIER_ARG_NORET;

    hgl.glUniform2iv(location, count, v);
    free(v);
}

GLES2_CB(glUniform3f)
{
    GLES2_ARG(TGLint, location);
    GLES2_ARG(TGLfloat, x);
    GLES2_ARG(TGLfloat, y);
    GLES2_ARG(TGLfloat, z);
    GLES2_BARRIER_ARG_NORET;

    hgl.glUniform3f(location, x, y, z);
}

GLES2_CB(glUniform3fv)
{
    GLES2_ARG(TGLint, location);
    GLES2_ARG(TGLsizei, count);
    GLES2_ARG(Tptr, vp);

    unsigned nvs = count*3;
    GLfloat* v = malloc(nvs*sizeof(*v));
    unsigned i;
    for(i = 0; i < nvs; ++i) {
        v[i] = gles2_get_TGLfloat(s, vp + i*sizeof(TGLfloat));
    }
    GLES2_BARRIER_ARG_NORET;

    hgl.glUniform3fv(location, count, v);
    free(v);
}

GLES2_CB(glUniform3i)
{
    GLES2_ARG(TGLint, location);
    GLES2_ARG(TGLint, x);
    GLES2_ARG(TGLint, y);
    GLES2_ARG(TGLint, z);
    GLES2_BARRIER_ARG_NORET;

    hgl.glUniform3i(location, x, y, z);
}

GLES2_CB(glUniform3iv)
{
    GLES2_ARG(TGLint, location);
    GLES2_ARG(TGLsizei, count);
    GLES2_ARG(Tptr, vp);

    unsigned nvs = count*3;
    GLint* v = malloc(nvs*sizeof(*v));
    unsigned i;
    for(i = 0; i < nvs; ++i) {
        v[i] = gles2_get_TGLint(s, vp + i*sizeof(TGLint));
    }
    GLES2_BARRIER_ARG_NORET;

    hgl.glUniform3iv(location, count, v);
    free(v);
}

GLES2_CB(glUniform4f)
{
    GLES2_ARG(TGLint, location);
    GLES2_ARG(TGLfloat, x);
    GLES2_ARG(TGLfloat, y);
    GLES2_ARG(TGLfloat, z);
    GLES2_ARG(TGLfloat, w);
    GLES2_BARRIER_ARG_NORET;

    hgl.glUniform4f(location, x, y, z, w);
}

GLES2_CB(glUniform4fv)
{
    GLES2_ARG(TGLint, location);
    GLES2_ARG(TGLsizei, count);
    GLES2_ARG(Tptr, vp);

    unsigned nvs = count*4;
    GLfloat* v = malloc(nvs*sizeof(*v));
    unsigned i;
    for(i = 0; i < nvs; ++i) {
        v[i] = gles2_get_TGLfloat(s, vp + i*sizeof(TGLfloat));
    }

    GLES2_BARRIER_ARG_NORET;
    hgl.glUniform4fv(location, count, v);
    free(v);
}

GLES2_CB(glUniform4i)
{
    GLES2_ARG(TGLint, location);
    GLES2_ARG(TGLint, x);
    GLES2_ARG(TGLint, y);
    GLES2_ARG(TGLint, z);
    GLES2_ARG(TGLint, w);
    GLES2_BARRIER_ARG_NORET;

    hgl.glUniform4i(location, x, y, z, w);
}

GLES2_CB(glUniform4iv)
{
    GLES2_ARG(TGLint, location);
    GLES2_ARG(TGLsizei, count);
    GLES2_ARG(Tptr, vp);

    unsigned nvs = count*4;
    GLint* v = malloc(nvs*sizeof(*v));
    unsigned i;
    for(i = 0; i < nvs; ++i) {
        v[i] = gles2_get_TGLint(s, vp + i*sizeof(TGLint));
    }

    GLES2_BARRIER_ARG_NORET;
    hgl.glUniform4iv(location, count, v);
    free(v);
}

GLES2_CB(glUniformMatrix2fv)
{
    GLES2_ARG(TGLint, location);
    GLES2_ARG(TGLint, count);
    GLES2_ARG(TGLboolean, transpose);
    GLES2_ARG(Tptr, valuep);

    unsigned nfloats = 2*2*count;
    GLfloat* value = malloc(nfloats*sizeof(TGLfloat));
    unsigned i;

    for (i = 0; i < nfloats; ++i) {
        value[i] = gles2_get_TGLfloat(s, valuep + i*sizeof(TGLfloat));
    }

    GLES2_BARRIER_ARG_NORET;

    hgl.glUniformMatrix2fv(location, count, transpose, value);
    free(value);
}

GLES2_CB(glUniformMatrix3fv)
{
    GLES2_ARG(TGLint, location);
    GLES2_ARG(TGLint, count);
    GLES2_ARG(TGLboolean, transpose);
    GLES2_ARG(Tptr, valuep);

    unsigned nfloats = 3*3*count;
    GLfloat* value = malloc(nfloats*sizeof(TGLfloat));
    unsigned i;
    for(i = 0; i < nfloats; ++i) {
        value[i] = gles2_get_TGLfloat(s, valuep + i*sizeof(TGLfloat));
    }
    GLES2_BARRIER_ARG_NORET;

    hgl.glUniformMatrix3fv(location, count, transpose, value);
    free(value);
}

GLES2_CB(glUniformMatrix4fv)
{
    GLES2_ARG(TGLint, location);
    GLES2_ARG(TGLint, count);
    GLES2_ARG(TGLboolean, transpose);
    GLES2_ARG(Tptr, valuep);

    unsigned nfloats = 4*4*count;
    GLfloat* value = malloc(nfloats*sizeof(TGLfloat));
    unsigned i;
    for(i = 0; i < nfloats; ++i) {
        value[i] = gles2_get_TGLfloat(s, valuep + i*sizeof(TGLfloat));
    }
    GLES2_BARRIER_ARG_NORET;

    hgl.glUniformMatrix4fv(location, count, transpose, value);
    free(value);
}

GLES2_CB(glUseProgram)
{
    GLES2_ARG(TGLuint, program);
    GLES2_BARRIER_ARG_NORET;

    hgl.glUseProgram(program);
}

GLES2_CB(glBlendColor)
{
    GLES2_ARG(TGLclampf, red);
    GLES2_ARG(TGLclampf, green);
    GLES2_ARG(TGLclampf, blue);
    GLES2_ARG(TGLclampf, alpha);
    GLES2_BARRIER_ARG_NORET;

    hgl.glBlendColor(red, green, blue, alpha);
}

GLES2_CB(glBlendEquation)
{
    GLES2_ARG(TGLenum, mode);
    GLES2_BARRIER_ARG_NORET;

    hgl.glBlendEquation(mode);
}

GLES2_CB(glBlendEquationSeparate)
{
    GLES2_ARG(TGLenum, modeRGB);
    GLES2_ARG(TGLenum, modeAlpha);
    GLES2_BARRIER_ARG_NORET;

    hgl.glBlendEquationSeparate(modeRGB, modeAlpha);
}


GLES2_CB(glBlendFuncSeparate)
{
    GLES2_ARG(TGLenum, srcRGB);
    GLES2_ARG(TGLenum, dstRGB);
    GLES2_ARG(TGLenum, srcAlpha);
    GLES2_ARG(TGLenum, dstAlpha);
    GLES2_BARRIER_ARG_NORET;

    hgl.glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

GLES2_CB(glValidateProgram)
{
    GLES2_ARG(TGLuint, program);
    GLES2_BARRIER_ARG_NORET;

    hgl.glValidateProgram(program);
}

GLES2_CB(glStencilFuncSeparate)
{
    GLES2_ARG(TGLenum, face);
    GLES2_ARG(TGLenum, func);
    GLES2_ARG(TGLint, ref);
    GLES2_ARG(TGLuint, mask);
    GLES2_BARRIER_ARG_NORET;

    hgl.glStencilFuncSeparate(face, func, ref, mask);
}


GLES2_CB(glStencilMaskSeparate)
{
    GLES2_ARG(TGLenum, face);
    GLES2_ARG(TGLuint, mask);
    GLES2_BARRIER_ARG_NORET;

    hgl.glStencilMaskSeparate(face, mask);
}


GLES2_CB(glStencilOpSeparate)
{
    GLES2_ARG(TGLenum, face);
    GLES2_ARG(TGLenum, fail);
    GLES2_ARG(TGLenum, zfail);
    GLES2_ARG(TGLenum, zpass);
    GLES2_BARRIER_ARG_NORET;

    hgl.glStencilOpSeparate(face, fail, zfail, zpass);
}

GLES2_CB(glBindFramebuffer)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLuint, framebuffer);
    GLES2_BARRIER_ARG_NORET;

    hgl.glBindFramebuffer(target, framebuffer);
}

GLES2_CB(glBindRenderbuffer)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLuint, renderbuffer);
    GLES2_BARRIER_ARG_NORET;

    hgl.glBindRenderbuffer(target, renderbuffer);
}

GLES2_CB(glCheckFramebufferStatus)
{
    GLES2_ARG(TGLenum, target);
    GLES2_BARRIER_ARG;

    GLES2_BARRIER_RET;
    gles2_ret_TGLenum(s, hgl.glCheckFramebufferStatus(target));
}

GLES2_CB(glDeleteFramebuffers)
{
    GLES2_ARG(TGLsizei, n);
    GLES2_ARG(Tptr, framebuffersp);

    GLsizei i;
    GLuint* framebuffers = (GLuint*)malloc(sizeof(GLuint)*n);

    for (i = 0; i < n; ++i) {
        framebuffers[i] = gles2_get_TGLuint(s,
            framebuffersp + i*sizeof(TGLuint));
    }
    GLES2_BARRIER_ARG_NORET;

    hgl.glDeleteFramebuffers(n, framebuffers);
    free(framebuffers);
}

GLES2_CB(glDeleteRenderbuffers)
{
    GLES2_ARG(TGLsizei, n);
    GLES2_ARG(Tptr, renderbuffersp);

    GLsizei i;
    GLuint* renderbuffers = (GLuint*)malloc(sizeof(GLuint)*n);

    for (i = 0; i < n; ++i) {
        renderbuffers[i] = gles2_get_TGLuint(s,
            renderbuffersp + i*sizeof(TGLuint));
    }
    GLES2_BARRIER_ARG_NORET;

    hgl.glDeleteRenderbuffers(n, renderbuffers);
    free(renderbuffers);
}

GLES2_CB(glFramebufferRenderbuffer)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLenum, attachment);
    GLES2_ARG(TGLenum, renderbuffertarget);
    GLES2_ARG(TGLuint, renderbuffer);
    GLES2_BARRIER_ARG_NORET;

    hgl.glFramebufferRenderbuffer(target, attachment,
        renderbuffertarget, renderbuffer);
}

GLES2_CB(glFramebufferTexture2D)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLenum, attachment);
    GLES2_ARG(TGLenum, textarget);
    GLES2_ARG(TGLuint, texture);
    GLES2_ARG(TGLint, level);
    GLES2_BARRIER_ARG_NORET;

    hgl.glFramebufferTexture2D(target, attachment, textarget, texture, level);
}

GLES2_CB(glGenFramebuffers)
{
    GLES2_ARG(TGLsizei, n);
    GLES2_ARG(Tptr, framebuffersp);

    GLsizei i;
    GLuint* framebuffers = (GLuint*)malloc(sizeof(GLuint)*n);

    hgl.glGenFramebuffers(n, framebuffers);
    for(i = 0; i < n; ++i) {
        gles2_put_TGLuint(s, framebuffersp + i*sizeof(TGLuint),
            framebuffers[i]);
    }
    GLES2_BARRIER_ARG_NORET;

    free(framebuffers);
}

GLES2_CB(glGenRenderbuffers)//(GLsizei n, GLuint* renderbuffers)
{
    GLES2_ARG(TGLsizei, n);
    GLES2_ARG(Tptr, renderbuffersp);

    GLsizei i;
    GLuint* renderbuffers = (GLuint*)malloc(sizeof(GLuint)*n);

    hgl.glGenRenderbuffers(n, renderbuffers);
    for(i = 0; i < n; ++i) {
        gles2_put_TGLuint(s, renderbuffersp + i*sizeof(TGLuint),
            renderbuffers[i]);
    }
    GLES2_BARRIER_ARG_NORET;

    free(renderbuffers);
}

GLES2_CB(glGetFramebufferAttachmentParameteriv)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLenum, attachment);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    GLES2_BARRIER_ARG;

    GLint params = 0;
    hgl.glGetFramebufferAttachmentParameteriv(target, attachment, pname, &params);

    GLES2_BARRIER_RET;
    gles2_put_TGLint(s, paramsp, params);
}

GLES2_CB(glGetRenderbufferParameteriv)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    GLES2_BARRIER_ARG;

    GLint params = 0;
    hgl.glGetRenderbufferParameteriv(target, pname, &params);

    GLES2_BARRIER_RET;
    gles2_put_TGLint(s, paramsp, params);
}

GLES2_CB(glIsFramebuffer)//(GLuint framebuffer)
{
    GLES2_ARG(TGLuint, framebuffer);
    GLES2_BARRIER_ARG;

    GLES2_BARRIER_RET;
    gles2_ret_TGLboolean(s, hgl.glIsFramebuffer(framebuffer));
}

GLES2_CB(glIsRenderbuffer)//(GLuint renderbuffer)
{
    GLES2_ARG(TGLuint, renderbuffer);
    GLES2_BARRIER_ARG;

    GLES2_BARRIER_RET;
    gles2_ret_TGLboolean(s, hgl.glIsRenderbuffer(renderbuffer));
}

GLES2_CB(glRenderbufferStorage)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLenum, internalformat);
    GLES2_ARG(TGLsizei, width);
    GLES2_ARG(TGLsizei, height);
    GLES2_BARRIER_ARG_NORET;

    hgl.glRenderbufferStorage(target, internalformat, width, height);
}
