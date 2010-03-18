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
#include "EGL/degl.h"
#include "GLES2/gl2.h"

// Automatically create the prototype and function definition.
#define GLES2_CB(FUNC) \
    void gles2_##FUNC##_cb(gles2_State *s, \
        gles2_decode_t *d, gles2_Client *c); \
    void gles2_##FUNC##_cb(gles2_State *s, \
        gles2_decode_t *d, gles2_Client *c)

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
    inline void gles2_ret_##TYPE(gles2_State *s, TYPE value) \
    { gles2_ret_##SIZE(s, value); } \
    inline void gles2_put_##TYPE(gles2_State *s, target_ulong va, TYPE value); \
    inline void gles2_put_##TYPE(gles2_State *s, target_ulong va, TYPE value) \
    { gles2_put_##SIZE(s, va, value); } \
    inline TYPE gles2_get_##TYPE(gles2_State *s, target_ulong va); \
    inline TYPE gles2_get_##TYPE(gles2_State *s, target_ulong va) \
    { return (TYPE)gles2_get_##SIZE(s, va); } \
    inline TYPE gles2_arg_##TYPE(gles2_State *s, gles2_decode_t *d); \
    inline TYPE gles2_arg_##TYPE(gles2_State *s, gles2_decode_t *d) \
    { return (TYPE)gles2_arg_##SIZE(s, d); }

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

// Just one more macro for even less typing.
#define GLES2_ARG(TYPE, NAME) \
    TYPE NAME = gles2_arg_##TYPE(s, d)

//        pthread_cond_signal(&s->cond_xcode);

GLES2_CB(eglBindAPI)
{
    GLES2_ARG(TEGLenum, api);
    GLES2_BARRIER_ARG;

    GLES2_BARRIER_RET;
    gles2_ret_TEGLBoolean(s, eglBindAPI(api));
}

GLES2_CB(eglGetDisplay)
{
//	GLES2_ARG(TEGLDisplay, dpy);
//	(void)dpy;
    GLES2_BARRIER_ARG;

    GLES2_PRINT("Getting display...\n");

    EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    GLES2_PRINT("\tGot host display %p...\n", dpy);

    GLES2_BARRIER_RET;
    gles2_ret_TEGLDisplay(s, gles2_handle_create(s, dpy));
}

GLES2_CB(eglInitialize)
{
    GLES2_ARG(TEGLDisplay, dpy_);
    GLES2_ARG(Tptr, majorp);
    GLES2_ARG(Tptr, minorp);

    GLES2_BARRIER_ARG;

    EGLDisplay dpy = (EGLDisplay)gles2_handle_get(s, dpy_);

    GLES2_PRINT("Request to initialize display %p...\n", dpy);

    EGLint major, minor;
    if (eglInitialize(dpy, &major, &minor)) {
        GLES2_PRINT("Display initialized (EGL %d.%d)!\n", major, minor);
        GLES2_BARRIER_RET;
        gles2_put_TEGLint(s, majorp, major);
        gles2_put_TEGLint(s, minorp, minor);
        gles2_ret_TEGLBoolean(s, EGL_TRUE);
        return;
    }

    GLES2_PRINT("Failed to initialize...\n");
    GLES2_BARRIER_RET;
    gles2_ret_TEGLBoolean(s, EGL_FALSE);
}

GLES2_CB(eglGetConfigs)
{
    GLES2_ARG(TEGLDisplay, dpy_);
    GLES2_ARG(Tptr, configsp);
    GLES2_ARG(TEGLint, config_size);
    GLES2_ARG(Tptr, num_configp);

    GLES2_BARRIER_ARG;

    EGLDisplay dpy = (EGLDisplay)gles2_handle_get(s, dpy_);

    EGLConfig* configs = configsp ? malloc(sizeof(EGLConfig)*config_size) : NULL;

    EGLint num_config;
    EGLBoolean ret = eglGetConfigs(dpy, configs, config_size, &num_config);

    GLES2_BARRIER_RET;
    if (configs) {
        EGLint i;

        for (i = 0; i < num_config; ++i) {
            uint32_t handle;
            if (!(handle = gles2_handle_find(s, configs[i]))) {
                handle = gles2_handle_create(s, configs[i]);
            }
            gles2_put_TEGLConfig(s, configsp + i*sizeof(TEGLConfig), handle);
        }

        free(configs);
    }
    gles2_put_TEGLint(s, num_configp, num_config);

    gles2_ret_TEGLBoolean(s, ret);
}

GLES2_CB(eglChooseConfig)
{
    GLES2_ARG(TEGLDisplay, dpy_);
    GLES2_ARG(Tptr, attrib_listp);
    GLES2_ARG(Tptr, configsp);
    GLES2_ARG(TEGLint, config_size);
    GLES2_ARG(Tptr, num_configp);
    (void)config_size;
    (void)attrib_listp;

    EGLint attrib_list_n = 0;
    while (gles2_get_TEGLint(s, attrib_listp
        + attrib_list_n*sizeof(EGLint)) != EGL_NONE) {
        attrib_list_n += 2;
    }
    EGLint* attrib_list = malloc((attrib_list_n + 1)*sizeof(EGLint));
    EGLint i;

    for (i = 0; i < attrib_list_n; ++i) {
        attrib_list[i] = gles2_get_TEGLint(s, attrib_listp
            + i*sizeof(EGLint));
    }
    attrib_list[attrib_list_n] = EGL_NONE;
    GLES2_BARRIER_ARG;

    EGLDisplay dpy = (EGLDisplay)gles2_handle_get(s, dpy_);

    EGLConfig* configs = configsp ? malloc(sizeof(EGLConfig)*config_size) : NULL;

    EGLint num_config;
    EGLBoolean ret = eglChooseConfig(dpy, attrib_list, configs, config_size, &num_config);
    free(attrib_list);
    GLES2_BARRIER_RET;
    if (configs) {
        EGLint i;

        for (i = 0; i < num_config; ++i) {
            uint32_t handle;
            if (!(handle = gles2_handle_find(s, configs[i]))) {
                handle = gles2_handle_create(s, configs[i]);
            }
            gles2_put_TEGLConfig(s, configsp + i*sizeof(TEGLConfig), handle);
        }

        free(configs);
    }
    gles2_put_TEGLint(s, num_configp, num_config);

    gles2_ret_TEGLBoolean(s, ret);
}

GLES2_CB(eglGetConfigAttrib)
{
    GLES2_ARG(TEGLDisplay, dpy_);
    GLES2_ARG(TEGLConfig, config);
    GLES2_ARG(TEGLint, attribute);
    GLES2_ARG(Tptr, valuep);
    GLES2_BARRIER_ARG;

    EGLDisplay dpy = (EGLDisplay)gles2_handle_get(s, dpy_);

    EGLint value;
    EGLBoolean ret = eglGetConfigAttrib(dpy, gles2_handle_get(s, config), attribute, &value);

    GLES2_BARRIER_RET;

    gles2_put_TEGLint(s, valuep, value);
    gles2_ret_TEGLBoolean(s, ret);
}

typedef struct gles2_Surface
{
    uint32_t ddrawp;    // Pointer to the offscreen drawable in guest memory.
    DEGLDrawable ddraw; // Offscreen drawable, read from guest memory.
    EGLSurface surf;    // Pointer to the EGL surface.
    uint32_t pixelsp;   // Pointer to pixels in guest memory.
    int pixmap;         // True if surface is pixmap.
    gles2_CompiledTransfer tfr; // Framebuffer transfer.
    int valid;          // If the surface is valid.
    int id; // DEBUG!
} gles2_Surface;

// See if guest offscreen drawable was changed and if so, update host copy.
static int gles2_surface_update(gles2_State *s, gles2_Surface *surf)
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
static void gles2_eglSwapCallback(void* userdata)
{
    (void)userdata;
    GLES2_PRINT("Swap called!\n");
}

static int surf_id = 1;

GLES2_CB(eglCreateWindowSurface)
{
    GLES2_ARG(TEGLDisplay, dpy_);
    GLES2_ARG(TEGLConfig, config_);
    GLES2_ARG(Tptr, winp);
    GLES2_ARG(Tptr, attrib_listp);


    EGLDisplay dpy = (EGLDisplay)gles2_handle_get(s, dpy_);
    EGLConfig config = (EGLConfig)gles2_handle_get(s, config_);
    (void)attrib_listp;

    gles2_Surface* fsurf;

    if (!(fsurf = malloc(sizeof(*fsurf)))) {
        GLES2_PRINT("\tFake window creation failed!\n");
        GLES2_BARRIER_ARG;
        GLES2_BARRIER_RET;
        gles2_ret_TEGLSurface(s, 0);
        return;
    }

    fsurf->id = surf_id++;

    fsurf->ddrawp = winp;
    fsurf->pixmap = 0;
    gles2_surface_update(s, fsurf);
    GLES2_BARRIER_ARG;

    GLES2_PRINT("Host window creation requested, %dx%d@%d(Bpp=%d) at 0x%x, ID = %d...\n",
        fsurf->ddraw.width, fsurf->ddraw.height,
        fsurf->ddraw.depth, fsurf->ddraw.bpp, fsurf->pixelsp, fsurf->id);

    unsigned nbytes = fsurf->ddraw.width*fsurf->ddraw.height*fsurf->ddraw.bpp;
    fsurf->ddraw.pixels = malloc(nbytes);
    fsurf->ddraw.userdata = fsurf;
    fsurf->ddraw.swap = gles2_eglSwapCallback;

    if((fsurf->surf = eglCreateWindowSurface(dpy, config,
	    (EGLNativeWindowType)&fsurf->ddraw, NULL)) == EGL_NO_SURFACE)
    {
        GLES2_PRINT("\tHost window creation failed!\n");
        free(fsurf->ddraw.pixels);
        free(fsurf);
        GLES2_BARRIER_RET;
        gles2_ret_TEGLSurface(s, 0);
        return;
    }

    GLES2_PRINT("Created at %p!\n", fsurf);
    GLES2_BARRIER_RET;
    gles2_transfer_compile(&fsurf->tfr, s, fsurf->pixelsp, nbytes);
    gles2_ret_TEGLSurface(s, gles2_handle_create(s, fsurf));
}

GLES2_CB(eglCreatePixmapSurface)
{
    GLES2_ARG(TEGLDisplay, dpy_);
    GLES2_ARG(TEGLConfig, config_);
    GLES2_ARG(Tptr, pixmapp);
    GLES2_ARG(Tptr, attrib_listp);

    EGLDisplay dpy = (EGLDisplay)gles2_handle_get(s, dpy_);
    EGLConfig config = (EGLConfig)gles2_handle_get(s, config_);
    (void)attrib_listp;

    gles2_Surface* fsurf;

    if (!(fsurf = malloc(sizeof(*fsurf)))) {
        GLES2_PRINT("\tFake pixmap creation failed!\n");
        GLES2_BARRIER_ARG;
        GLES2_BARRIER_RET;
        gles2_ret_TEGLSurface(s, 0);
        return;
    }

    fsurf->id = surf_id++;
    fsurf->ddrawp = pixmapp;
    fsurf->pixmap = 1;
    gles2_surface_update(s, fsurf);
    GLES2_BARRIER_ARG;

    GLES2_PRINT("Host pixmap creation requested, %dx%d@%d(Bpp=%d) at 0x%x, ID = %d...\n",
        fsurf->ddraw.width, fsurf->ddraw.height,
        fsurf->ddraw.depth, fsurf->ddraw.bpp, fsurf->pixelsp, fsurf->id);

    unsigned nbytes = fsurf->ddraw.width*fsurf->ddraw.height*fsurf->ddraw.bpp;
    fsurf->ddraw.pixels = malloc(nbytes);
    fsurf->ddraw.userdata = fsurf;
    fsurf->ddraw.swap = gles2_eglSwapCallback;

    if((fsurf->surf = eglCreatePixmapSurface(dpy, config,
        (EGLNativeWindowType)&fsurf->ddraw, NULL)) == EGL_NO_SURFACE) {
        GLES2_PRINT("\tHost pixmap creation failed!\n");
        free(fsurf->ddraw.pixels);
        free(fsurf);
        GLES2_BARRIER_RET;
        gles2_ret_TEGLSurface(s, 0);
        return;
    }

    GLES2_PRINT("Created at %p!\n", fsurf);
    GLES2_BARRIER_RET;
    gles2_transfer_compile(&fsurf->tfr, s, fsurf->pixelsp, nbytes);
    gles2_ret_TEGLSurface(s, gles2_handle_create(s, fsurf));
}

GLES2_CB(eglDestroySurface)
{
    GLES2_ARG(TEGLDisplay, dpy_);
    GLES2_ARG(TEGLSurface, surface_);
    GLES2_BARRIER_ARG_NORET;

    EGLDisplay dpy = (EGLDisplay)gles2_handle_get(s, dpy_);
    gles2_Surface* fsurf = (EGLSurface)gles2_handle_get(s, surface_);
    gles2_handle_free(s, surface_);

    GLES2_PRINT("Destroyed surface ID = %d...\n", fsurf->id);
    fsurf->id = -1;

    eglDestroySurface(dpy, fsurf->surf);
    free(fsurf->ddraw.pixels);

//    if(fsurf->pixmap == 0) {
        gles2_transfer_free(&fsurf->tfr);
//    }
    free(fsurf);
}

GLES2_CB(eglBindTexImage)
{
    GLES2_ARG(TEGLDisplay, dpy_);
    GLES2_ARG(TEGLSurface, surface_);
    GLES2_ARG(TEGLint, buffer);
    gles2_CompiledTransfer tfr;

    EGLDisplay dpy = (EGLDisplay)gles2_handle_get(s, dpy_);
    gles2_Surface* fsurf = (gles2_Surface*)gles2_handle_get(s, surface_);

    // FIXME: Not a very clean way..
    uint32_t pixelsp = gles2_get_dword(s, fsurf->ddrawp + 4*sizeof(uint32_t));
    if (pixelsp) {
        unsigned nbytes = fsurf->ddraw.width
            * fsurf->ddraw.height*fsurf->ddraw.bpp;
        gles2_transfer_compile(&tfr, s, pixelsp, nbytes);
    }
    GLES2_BARRIER_ARG;

    if (pixelsp) {
//        gles2_transfer(s, pixelsp, nbytes, fsurf->ddraw.pixels, 0);
        gles2_transfer_exec(&tfr, s, fsurf->ddraw.pixels, 0);
    }

    GLES2_PRINT("Binding surface ID = %d!\n", fsurf->id);

    EGLBoolean ret = eglBindTexImage(dpy, fsurf->surf, buffer);
    if (pixelsp) {
        gles2_transfer_free(&tfr);
    }

    GLES2_BARRIER_RET;
    gles2_ret_TEGLBoolean(s, ret);
}

GLES2_CB(eglReleaseTexImage)
{
    GLES2_ARG(TEGLDisplay, dpy_);
    GLES2_ARG(TEGLSurface, surface_);
    GLES2_ARG(TEGLint, buffer);
    GLES2_BARRIER_ARG;

    EGLDisplay dpy = (EGLDisplay)gles2_handle_get(s, dpy_);
    gles2_Surface* fsurf = (gles2_Surface*)gles2_handle_get(s, surface_);

    GLES2_PRINT("Unbinding surface ID = %d!\n", fsurf->id);

    EGLBoolean ret = eglReleaseTexImage(dpy, fsurf->surf, buffer);
    GLES2_BARRIER_RET;
    gles2_ret_TEGLBoolean(s, ret);
}

GLES2_CB(eglCreateContext)
{
    GLES2_ARG(TEGLDisplay, dpy_);
    GLES2_ARG(TEGLConfig, config_);
    GLES2_ARG(TEGLContext, share_context_);
    GLES2_ARG(Tptr, attrib_listp);
    GLES2_BARRIER_ARG;

    EGLDisplay dpy = (EGLDisplay)gles2_handle_get(s, dpy_);
    EGLConfig config = (EGLConfig)gles2_handle_get(s, config_);
    EGLContext share_context = (EGLContext)gles2_handle_get(s, share_context_);

    GLES2_PRINT("TODO: Handle attribs...\n");
    (void)attrib_listp;

    GLES2_PRINT("Host context creation requested...\n");
    EGLContext ctx = eglCreateContext(dpy, config, share_context, NULL);
    GLES2_BARRIER_RET;
    if (ctx == EGL_NO_CONTEXT) {
        GLES2_PRINT("\tContext creation failed!\n");
        gles2_ret_TEGLContext(s, 0);
    } else {
        GLES2_PRINT("Created at %p!\n", ctx);
        gles2_ret_TEGLContext(s, gles2_handle_create(s, ctx));
    }
}

GLES2_CB(eglDestroyContext)
{
    GLES2_ARG(TEGLDisplay, dpy_);
    GLES2_ARG(TEGLContext, ctx_);
    GLES2_BARRIER_ARG;

    EGLDisplay dpy = (EGLDisplay)gles2_handle_get(s, dpy_);
    EGLContext ctx = (EGLContext)gles2_handle_get(s, ctx_);
    gles2_handle_free(s, ctx_);
    GLES2_PRINT("Destroyed %p!\n", ctx);

    GLES2_BARRIER_RET;
    gles2_ret_TEGLBoolean(s, eglDestroyContext(dpy, ctx));
}

// Host to guest vertex array copy.
struct gles2_Array
{
    GLuint indx;          // Parameter of the call.
    GLint size;           // --''--
    GLenum type;          // --''--
    GLboolean normalized; // --''--
    GLsizei stride;       // --''--
    Tptr tptr;            // Pointer in the guest memory.
    void* ptr;            // Pointer in the host memory.

    GLboolean enabled;    // State.
};

GLES2_CB(eglMakeCurrent)
{
    GLES2_ARG(TEGLDisplay, dpy_);
    GLES2_ARG(TEGLSurface, draw_);
    GLES2_ARG(TEGLSurface, read_);
    GLES2_ARG(TEGLContext, ctx_);
    GLES2_BARRIER_ARG;
    int i;

    EGLDisplay dpy = (EGLDisplay)gles2_handle_get(s, dpy_);
    gles2_Surface* draw = (EGLSurface)gles2_handle_get(s, draw_);
    gles2_Surface* read = (EGLSurface)gles2_handle_get(s, read_);
    EGLContext ctx = (EGLContext)gles2_handle_get(s, ctx_);

    GLES2_PRINT("Making host context current...\n");

    if (!eglMakeCurrent(dpy,
        draw ? draw->surf : NULL,
        read ? read->surf : NULL,
        ctx)) {
        GLES2_PRINT("\tMakeCurrent failed!\n");
        GLES2_BARRIER_RET;
        gles2_ret_TEGLBoolean(s, EGL_FALSE);
        return;
    }

    // Initialize client state.
    if (ctx) {
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &c->narrays);

        GLES2_PRINT("Maximum number of host vertex arrays: %d.\n", c->narrays);

        c->arrays = malloc(c->narrays * sizeof(*c->arrays));
        for (i = 0; i < c->narrays; ++i) {
            c->arrays[i].type = GL_NONE;
            c->arrays[i].enabled = 0;
            c->arrays[i].ptr = 0;
        }
    }

    GLES2_PRINT("Made %p current (DRAW = %d, READ = %d)!\n", ctx, draw ? draw->id : 0, read ? read->id : 0);
    GLES2_BARRIER_RET;
    gles2_ret_TEGLBoolean(s, EGL_TRUE);
}

GLES2_CB(eglSwapBuffers)
{
    GLES2_ARG(TEGLDisplay, dpy_);
    GLES2_ARG(TEGLSurface, surface_);

    EGLDisplay dpy = (EGLDisplay)gles2_handle_get(s, dpy_);
    gles2_Surface* fsurf = (EGLSurface)gles2_handle_get(s, surface_);
    if (!fsurf) {
        fprintf(stderr, "ERROR: Trying to swap NULL surface!\n");
        GLES2_BARRIER_ARG;
        GLES2_BARRIER_RET;
        gles2_ret_TEGLBoolean(s, EGL_TRUE);
        return;
    }

    if (gles2_surface_update(s, fsurf)) {
        GLES2_BARRIER_ARG;
        GLES2_PRINT("DIMENSIONS CHANGED!\n");
        glFinish();
        free(fsurf->ddraw.pixels);
        unsigned nbytes = fsurf->ddraw.width
            * fsurf->ddraw.height*fsurf->ddraw.bpp;
        fsurf->ddraw.pixels = malloc(nbytes);

        gles2_transfer_free(&fsurf->tfr);
        GLES2_BARRIER_RET;
        gles2_transfer_compile(&fsurf->tfr, s, fsurf->pixelsp, nbytes);
        eglSwapBuffers(dpy, fsurf->surf);
        gles2_ret_TEGLBoolean(s, EGL_TRUE);
        return;
    }
    GLES2_BARRIER_ARG;

    GLES2_PRINT("Swapping DGLES2 surface ID = %d!\n", fsurf->id);
    eglSwapBuffers(dpy, fsurf->surf);

    GLES2_PRINT("Transferring frame!\n");
    gles2_transfer_exec(&fsurf->tfr, s, fsurf->ddraw.pixels, 1);
    GLES2_PRINT("\tDone!\n");
    GLES2_BARRIER_RET;
    gles2_ret_TEGLBoolean(s, EGL_TRUE);
}

GLES2_CB(glClearColor)
{
    GLES2_ARG(TGLclampf, red);
    GLES2_ARG(TGLclampf, green);
    GLES2_ARG(TGLclampf, blue);
    GLES2_ARG(TGLclampf, alpha);
    GLES2_BARRIER_ARG_NORET;

    glClearColor(red, green, blue, alpha);
}


GLES2_CB(glClear)
{
    GLES2_ARG(TGLbitfield, mask);
    GLES2_BARRIER_ARG_NORET;

    glClear(mask);
}

GLES2_CB(glDisableVertexAttribArray)
{
    GLES2_ARG(TGLuint, index);
    GLES2_BARRIER_ARG_NORET;
    if (index<= c->narrays)
        c->arrays[index].enabled = 0;
    glDisableVertexAttribArray(index);
}

static void gles2_TransferArrays(gles2_State *s, gles2_Client *c,
    GLint first, GLsizei count)
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

        GLsizei j;
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

        glVertexAttribPointer(va->indx, va->size, va->type,
            va->normalized, 0, va->ptr);
    }
}

GLES2_CB(glDrawArrays)
{
    GLES2_ARG(TGLenum, mode);
    GLES2_ARG(TGLint, first);
    GLES2_ARG(TGLsizei, count);

    gles2_TransferArrays(s, c, first, count);
    GLES2_BARRIER_ARG_NORET;

    glDrawArrays(mode, 0, count);
}

GLES2_CB(glDrawElements)
{
    GLES2_ARG(TGLenum, mode);
    GLES2_ARG(TGLsizei, count);
    GLES2_ARG(TGLenum, type);
    GLES2_ARG(Tptr, indicesp);

    GLint indice_size;
    switch (type) {
        case GL_UNSIGNED_BYTE: indice_size = sizeof(TGLubyte); break;
        case GL_UNSIGNED_SHORT: indice_size = sizeof(TGLushort); break;
        default:
            fprintf(stderr, "ERROR: Invalid type %d!\n", type);
            return;
    }
    int i, first = -1, last = -1;
    void *copied_indices = malloc(indice_size * count);
    for (i = 0; i < count; i++) {
        TGLushort idx = 0;
        switch (type) {
            case GL_UNSIGNED_BYTE:
                idx = gles2_get_byte(s, indicesp++);
                ((TGLubyte *)copied_indices)[i] = (TGLubyte)idx;
                break;
            case GL_UNSIGNED_SHORT:
                idx = gles2_get_word(s, indicesp);
                ((TGLushort *)copied_indices)[i] = idx;
                indicesp += 2;
                break;
            default:
                break;
        }
        if (first < 0 || idx < first) {
            first = idx;
        }
        if (last < 0 || idx > last) {
            last = idx;
        }
    }
    gles2_TransferArrays(s, c, first, last - first + 1);
    GLES2_BARRIER_ARG;

    glDrawElements(mode, count, type, copied_indices);
    free(copied_indices);
    GLES2_BARRIER_RET;
}

GLES2_CB(glEnableVertexAttribArray)
{
    GLES2_ARG(TGLuint, index);
    GLES2_BARRIER_ARG_NORET;

    if (index<= c->narrays)
        c->arrays[index].enabled = 1;
    glEnableVertexAttribArray(index);
}

#if 0
GL_APICALL void GL_APIENTRY glGetVertexAttribfv(GLuint index, GLenum pname,
    GLfloat* params)
{
    DUMMY();
}

GL_APICALL void GL_APIENTRY glGetVertexAttribiv(GLuint index, GLenum pname,
    GLint* params)
{
    DUMMY();
}

GL_APICALL void GL_APIENTRY glGetVertexAttribPointerv(GLuint index,
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

    glVertexAttrib1f(indx, x);
}

GLES2_CB(glVertexAttrib1fv)
{
    GLES2_ARG(TGLuint, indx);
    GLES2_ARG(Tptr, valuesp);
    GLfloat x = gles2_get_float(s, valuesp);
    GLES2_BARRIER_ARG_NORET;

    glVertexAttrib1f(indx, x);
}

GLES2_CB(glVertexAttrib2f)
{
    GLES2_ARG(TGLuint, indx);
    GLES2_ARG(TGLfloat, x);
    GLES2_ARG(TGLfloat, y);
    GLES2_BARRIER_ARG_NORET;

    glVertexAttrib2f(indx, x, y);
}

GLES2_CB(glVertexAttrib2fv)
{
    GLES2_ARG(TGLuint, indx);
    GLES2_ARG(Tptr, valuesp);

    GLfloat x = gles2_get_float(s, valuesp);
    GLfloat y = gles2_get_float(s, valuesp + sizeof(TGLfloat));
    GLES2_BARRIER_ARG_NORET;

    glVertexAttrib2f(indx, x, y);
}

GLES2_CB(glVertexAttrib3f)
{
    GLES2_ARG(TGLuint, indx);
    GLES2_ARG(TGLfloat, x);
    GLES2_ARG(TGLfloat, y);
    GLES2_ARG(TGLfloat, z);
    GLES2_BARRIER_ARG_NORET;

    glVertexAttrib3f(indx, x, y, z);
}

GLES2_CB(glVertexAttrib3fv)
{
    GLES2_ARG(TGLuint, indx);
    GLES2_ARG(Tptr, valuesp);

    GLfloat x = gles2_get_float(s, valuesp + 0*sizeof(TGLfloat));
    GLfloat y = gles2_get_float(s, valuesp + 1*sizeof(TGLfloat));
    GLfloat z = gles2_get_float(s, valuesp + 2*sizeof(TGLfloat));
    GLES2_BARRIER_ARG_NORET;

    glVertexAttrib3f(indx, x, y, z);
}

GLES2_CB(glVertexAttrib4f)
{
    GLES2_ARG(TGLuint, indx);
    GLES2_ARG(TGLfloat, x);
    GLES2_ARG(TGLfloat, y);
    GLES2_ARG(TGLfloat, z);
    GLES2_ARG(TGLfloat, w);
    GLES2_BARRIER_ARG_NORET;

    glVertexAttrib4f(indx, x, y, z, w);
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

    glVertexAttrib4f(indx, x, y, z, w);
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

    gles2_Array *va = c->arrays + indx;
    va->type = type;
    va->indx = indx;
    va->size = size;
    va->normalized = normalized;
    va->stride = stride;
    va->tptr = tptr;
}

static unsigned gles2_glGetCount(TGLenum pname)
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
        case GL_COMPRESSED_TEXTURE_FORMATS: glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &count); break;
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
        case GL_SHADER_BINARY_FORMATS: glGetIntegerv(GL_NUM_SHADER_BINARY_FORMATS, &count); break;
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

GLES2_CB(glGetBooleanv)
{
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    GLES2_BARRIER_ARG;

    GLboolean params[4];
    glGetBooleanv(pname, params);
    unsigned const count = gles2_glGetCount(pname);
    unsigned i;
    GLES2_BARRIER_RET;
    for(i = 0; i < count; ++i) {
        gles2_put_TGLboolean(s, paramsp + i*sizeof(TGLboolean), params[i]);
    }
}

GLES2_CB(glGetError)
{
    GLES2_BARRIER_ARG;

    GLenum error = glGetError();

    GLES2_BARRIER_RET;

    if (error != GL_NO_ERROR) {
        fprintf(stderr, "WARNING: Previous call to function %s caused an error %d!\n",
            c->prev_call->name, error);
    }
    gles2_ret_TGLenum(s, error);
}

GLES2_CB(eglGetError)
{
    GLES2_BARRIER_ARG;
    GLES2_BARRIER_RET;
    gles2_ret_TGLint(s, eglGetError());
}

GLES2_CB(glGetFloatv)
{
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    GLES2_BARRIER_ARG;

    GLfloat params[4];
    glGetFloatv(pname, params);
    unsigned const count = gles2_glGetCount(pname);
    unsigned i;
    GLES2_BARRIER_RET;
    for(i = 0; i < count; ++i) {
        gles2_put_TGLfloat(s, paramsp + i*sizeof(TGLfloat), params[i]);
    }
}

GLES2_CB(glGetIntegerv)
{
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    GLES2_BARRIER_ARG;

    GLint params[4];
    glGetIntegerv(pname, params);
    unsigned const count = gles2_glGetCount(pname);
    unsigned i;
    GLES2_BARRIER_RET;
    for(i = 0; i < count; ++i) {
        gles2_put_TGLint(s, paramsp + i*sizeof(TGLint), params[i]);
    }
}

GLES2_CB(glColorMask)
{
    GLES2_ARG(TGLboolean, red);
    GLES2_ARG(TGLboolean, green);
    GLES2_ARG(TGLboolean, blue);
    GLES2_ARG(TGLboolean, alpha);
    GLES2_BARRIER_ARG_NORET;

    glColorMask(red, green, blue, alpha);
}

GLES2_CB(glCullFace)
{
    GLES2_ARG(TGLenum, mode);
    GLES2_BARRIER_ARG_NORET;

    glCullFace(mode);
}

GLES2_CB(glDisable)
{
    GLES2_ARG(TGLenum, cap);
    GLES2_BARRIER_ARG;

    glDisable(cap);
    GLES2_BARRIER_RET;
}

GLES2_CB(glEnable)
{
    GLES2_ARG(TGLenum, cap);
    GLES2_BARRIER_ARG;

    glEnable(cap);

    GLES2_BARRIER_RET;
}

GLES2_CB(glFinish)
{
    // Important to do this way, so that we don't return too early.
    GLES2_BARRIER_ARG;
    glFinish();
    GLES2_BARRIER_RET;
}

GLES2_CB(glFlush)
{
    GLES2_BARRIER_ARG_NORET;
    glFlush();
}

GLES2_CB(glFrontFace)
{
    GLES2_ARG(TGLenum, mode);
    GLES2_BARRIER_ARG_NORET;

    glFrontFace(mode);
}

GLES2_CB(glIsEnabled)
{
    GLES2_ARG(TGLenum, cap);
    GLES2_BARRIER_ARG;

    GLES2_BARRIER_RET;
    gles2_ret_TGLboolean(s, glIsEnabled(cap));
}

GLES2_CB(glHint)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLenum, mode);
    GLES2_BARRIER_ARG_NORET;

    if(s->quality <= 75)
    {
        switch(target)
        {
            default: mode = GL_FASTEST; break;
        }
    }

    glHint(target, mode);
}

GLES2_CB(glLineWidth)
{
    GLES2_ARG(TGLfloat, width);
    GLES2_BARRIER_ARG_NORET;

    glLineWidth(width);
}

GLES2_CB(glPolygonOffset)
{
    GLES2_ARG(TGLfloat, factor);
    GLES2_ARG(TGLfloat, units);
    GLES2_BARRIER_ARG_NORET;

    glPolygonOffset(factor, units);
}

GLES2_CB(glSampleCoverage)
{
    GLES2_ARG(TGLclampf, value);
    GLES2_ARG(TGLboolean, invert);
    GLES2_BARRIER_ARG_NORET;

    glSampleCoverage(value, invert);
}

GLES2_CB(glScissor)
{
    GLES2_ARG(TGLint, x);
    GLES2_ARG(TGLint, y);
    GLES2_ARG(TGLsizei, width);
    GLES2_ARG(TGLsizei, height);
    GLES2_BARRIER_ARG_NORET;

    glScissor(x, y, width, height);
}

GLES2_CB(glViewport)
{
    GLES2_ARG(TGLint, x);
    GLES2_ARG(TGLint, y);
    GLES2_ARG(TGLsizei, width);
    GLES2_ARG(TGLsizei, height);
    GLES2_BARRIER_ARG_NORET;

    glViewport(x, y, width, height);
}

GLES2_CB(glActiveTexture)
{
    GLES2_ARG(TGLenum, texture);
    GLES2_BARRIER_ARG_NORET;

    glActiveTexture(texture);
}

GLES2_CB(glBindTexture)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLuint, texture);
    GLES2_BARRIER_ARG_NORET;

    glBindTexture(target, texture);
}

#if 0
GL_APICALL void GL_APIENTRY glCompressedTexImage2D(GLenum target,
    GLint level, GLenum internalformat, GLsizei width, GLsizei height,
    GLint border, GLsizei imageSize, const void* data)
{
    DUMMY();
}

GL_APICALL void GL_APIENTRY glCompressedTexSubImage2D(GLenum target,
    GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
    GLenum format, GLsizei imageSize, const void* data)
{
    DUMMY();
}

GL_APICALL void GL_APIENTRY glCopyTexImage2D(GLenum target, GLint level,
    GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height,
    GLint border)
{
    DUMMY();
}

GL_APICALL void GL_APIENTRY glCopyTexSubImage2D(GLenum target, GLint level,
    GLint xoffset, GLint yoffset, GLint x, GLint y,
    GLsizei width, GLsizei height)
{
    DUMMY();
}
#endif // 0

GLES2_CB(glDeleteTextures)
{
    GLES2_ARG(TGLsizei, n);
    GLES2_ARG(Tptr, texturesp);

    GLsizei i;
    GLuint* textures = (GLuint*)malloc(sizeof(GLuint)*n);
    for(i = 0; i < n; ++i) {
        textures[i] = gles2_get_TGLuint(s, texturesp + i*sizeof(TGLuint));
    }
    GLES2_BARRIER_ARG_NORET;

    glDeleteTextures(n, textures);
    free(textures);
}

GLES2_CB(glGenerateMipmap)
{
    GLES2_ARG(TGLenum, target);
    GLES2_BARRIER_ARG_NORET;

    glGenerateMipmap(target);
}

GLES2_CB(glGenTextures)
{
    GLES2_ARG(TGLsizei, n);
    GLES2_ARG(Tptr, texturesp);
    GLES2_BARRIER_ARG;

    GLsizei i;
    GLuint* textures = (GLuint*)malloc(sizeof(GLuint)*n);
    glGenTextures(n, textures);
    GLES2_BARRIER_RET;
    for(i = 0; i < n; ++i) {
        gles2_put_TGLuint(s, texturesp + i*sizeof(TGLuint), textures[i]);
    }
    free(textures);
}

static unsigned gles2_glTexParameterCount(GLenum pname)
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

GLES2_CB(glGetTexParameterfv)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    GLES2_BARRIER_ARG;

    GLfloat params[4];
    glGetTexParameterfv(target, pname, params);
    unsigned const count = gles2_glTexParameterCount(pname);
    unsigned i;
    GLES2_BARRIER_RET;
    for(i = 0; i < count; ++i) {
        gles2_put_TGLfloat(s, paramsp + i*sizeof(TGLfloat), params[i]);
    }
}

GLES2_CB(glGetTexParameteriv)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    GLES2_BARRIER_ARG;

    GLint params[4];
    glGetTexParameteriv(target, pname, params);
    unsigned const count = gles2_glTexParameterCount(pname);
    unsigned i;
    GLES2_BARRIER_RET;
    for(i = 0; i < count; ++i) {
        gles2_put_TGLint(s, paramsp + i*sizeof(TGLint), params[i]);
    }
}

GLES2_CB(glIsTexture)
{
    GLES2_ARG(TGLuint, texture);
    GLES2_BARRIER_ARG;

    GLES2_BARRIER_RET;
    gles2_ret_TGLboolean(s, glIsTexture(texture));
}

GLES2_CB(glReadPixels)
{
    GLES2_ARG(TGLint, x);
    GLES2_ARG(TGLint, y);
    GLES2_ARG(TGLsizei, width);
    GLES2_ARG(TGLsizei, height);
    GLES2_ARG(TGLenum, format);
    GLES2_ARG(TGLenum, type);
    GLES2_ARG(Tptr, pixelsp);
    GLES2_BARRIER_ARG;

    unsigned bpp;
    switch (format) {
        case GL_ALPHA: bpp = 1; break;
        case GL_RGB: bpp = (type == GL_UNSIGNED_BYTE) ? 3 : 2; break;
        case GL_RGBA: bpp = (type == GL_UNSIGNED_BYTE) ? 4 : 2; break;
        case GL_LUMINANCE: bpp = 1; break;
        case GL_LUMINANCE_ALPHA: bpp = 2; break;
        default:
            GLES2_PRINT("ERROR: Unknown format 0x%x...\n", format);
            bpp = 1;
            break;
    }

    GLES2_PRINT("Reading %dx%dx%d image at %d,%d...\n",
        width, height, bpp, x, y);
    char* pixels = NULL;
    unsigned nbytes = width*height*bpp;
    pixels = malloc(nbytes);

    glReadPixels(x, y, width, height, format, type, pixels);
    GLES2_BARRIER_RET;
    gles2_transfer(s, pixelsp, nbytes, pixels, 1);
    free(pixels);
}

GLES2_CB(glTexImage2D)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLint, level);
    GLES2_ARG(TGLint, internalformat);
    GLES2_ARG(TGLsizei, width);
    GLES2_ARG(TGLsizei, height);
    GLES2_ARG(TGLint, border);
    GLES2_ARG(TGLenum, format);
    GLES2_ARG(TGLenum, type);
    GLES2_ARG(Tptr, pixelsp);

    unsigned bpp;

    switch(format) {
        case GL_ALPHA: bpp = 1; break;
        case GL_RGB: bpp = (type == GL_UNSIGNED_BYTE) ? 3 : 2; break;
        case GL_RGBA: bpp = (type == GL_UNSIGNED_BYTE) ? 4 : 2; break;
        case GL_LUMINANCE: bpp = 1; break;
        case GL_LUMINANCE_ALPHA: bpp = 2; break;
        default:
            GLES2_PRINT("ERROR: Unknown format 0x%x...\n", format);
            bpp = 1;
            break;
    }

    GLES2_PRINT("Uploading %dx%dx%d image...\n", width, height, bpp);
    char* pixels = NULL;
    if (pixelsp) {
        unsigned nbytes = width*height*bpp;
        pixels = malloc(nbytes);
        gles2_transfer(s, pixelsp, nbytes, pixels, 0);
    }
    GLES2_BARRIER_ARG_NORET;

    glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
    free(pixels);
}

GLES2_CB(glTexParameterf)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(TGLfloat, param);
    GLES2_BARRIER_ARG_NORET;

    glTexParameterf(target, pname, param);
}

GLES2_CB(glTexParameterfv)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);

    GLfloat params[4];
    unsigned const count = gles2_glTexParameterCount(pname);
    unsigned i;
    for (i = 0; i < count; ++i) {
        params[i] = gles2_get_TGLfloat(s, paramsp + i*sizeof(TGLfloat));
    }
    GLES2_BARRIER_ARG_NORET;

    glTexParameterfv(target, pname, params);
}

GLES2_CB(glTexParameteri)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(TGLint, param);
    GLES2_BARRIER_ARG_NORET;

    if(s->quality <= 50)
    {
        switch(pname)
        {
            case GL_TEXTURE_MIN_FILTER: param = GL_NEAREST; break;
            case GL_TEXTURE_MAG_FILTER: param = GL_NEAREST; break;
            default: break;
        }
    }

    glTexParameteri(target, pname, param);
}

GLES2_CB(glTexParameteriv)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);

    GLint params[4];
    unsigned const count = gles2_glTexParameterCount(pname);
    unsigned i;
    for(i = 0; i < count; ++i) {
        params[i] = gles2_get_TGLint(s, paramsp + i*sizeof(GLint));
    }
    GLES2_BARRIER_ARG_NORET;

    glTexParameteriv(target, pname, params);
}

GLES2_CB(glTexSubImage2D)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLint, level);
    GLES2_ARG(TGLint, xoffset);
    GLES2_ARG(TGLint, yoffset);
    GLES2_ARG(TGLsizei, width);
    GLES2_ARG(TGLsizei, height);
    GLES2_ARG(TGLenum, format);
    GLES2_ARG(TGLenum, type);
    GLES2_ARG(Tptr, pixelsp);

    unsigned bpp;
    switch (format) {
        case GL_ALPHA: bpp = 1; break;
        case GL_RGB: bpp = (type == GL_UNSIGNED_BYTE) ? 3 : 2; break;
        case GL_RGBA: bpp = (type == GL_UNSIGNED_BYTE) ? 4 : 2; break;
        case GL_LUMINANCE: bpp = 1; break;
        case GL_LUMINANCE_ALPHA: bpp = 2; break;
        default:
            GLES2_PRINT("ERROR: Unknown format 0x%x...\n", format);
            bpp = 1;
            break;
    }

    GLES2_PRINT("Uploading partial %dx%dx%d image at %d,%d...\n",
    width, height, bpp, xoffset, yoffset);

    unsigned nbytes = width*height*bpp;
    char* pixels = malloc(nbytes);
    gles2_transfer(s, pixelsp, nbytes, pixels, 0);
    GLES2_BARRIER_ARG_NORET;

    glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
    free(pixels);
}


GLES2_CB(glCompileShader)
{
    GLES2_ARG(TGLuint, shader);
    GLES2_BARRIER_ARG_NORET;

    glCompileShader(shader);
}

GLES2_CB(glCreateShader)
{
    GLES2_ARG(TGLenum, type);
    GLES2_BARRIER_ARG;

    GLES2_BARRIER_RET;
    gles2_ret_TGLuint(s, glCreateShader(type));
}

GLES2_CB(glDeleteShader)
{
    GLES2_ARG(TGLuint, shader);
    GLES2_BARRIER_ARG_NORET;

    glDeleteShader(shader);
}

GLES2_CB(glIsShader)
{
    GLES2_ARG(TGLuint, shader);
    GLES2_BARRIER_ARG;

    GLES2_BARRIER_RET;
    gles2_ret_TGLboolean(s, glIsShader(shader));
}

GLES2_CB(glGetShaderiv)
{
    GLES2_ARG(TGLuint, shader);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    GLES2_BARRIER_ARG;

    GLint param;
    glGetShaderiv(shader, pname, &param);
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
    glGetShaderInfoLog(shader, bufsize, &length, infolog);
    gles2_transfer(s, infologp, length, infolog, 1);
    gles2_put_TGLsizei(s, lengthp, length);
    GLES2_BARRIER_ARG_NORET;

    GLES2_PRINT("shader %d infolog:\n%.*s\n", shader, length, infolog);
    free(infolog);
}

#if 0
GL_APICALL void GL_APIENTRY glGetShaderPrecisionFormat(GLenum shadertype,
    GLenum precisiontype, GLint* range, GLint* precision)
{
    DUMMY();
}

GL_APICALL void GL_APIENTRY glGetShaderSource(GLuint shader, GLsizei bufsize,
    GLsizei* length, char* source)
{
    DUMMY();
}

GL_APICALL GLboolean GL_APIENTRY glIsShader(GLuint shader)
{
    DUMMY();
}

GL_APICALL void GL_APIENTRY glReleaseShaderCompiler(void)
{
    DUMMY();
}

GL_APICALL void GL_APIENTRY glShaderBinary(GLsizei n, const GLuint* shaders,
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

    glShaderSource(shader, (GLsizei)count,
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

    glAttachShader(program, shader);
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
    glBindAttribLocation(program, index, name);
}

GLES2_CB(glCopyTexImage2D)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLint, level);
    GLES2_ARG(TGLenum, internalformat);
    GLES2_ARG(TGLint, x);
    GLES2_ARG(TGLint, y);
    GLES2_ARG(TGLsizei, width);
    GLES2_ARG(TGLsizei, height);
    GLES2_ARG(TGLint, border);

    GLES2_BARRIER_ARG;

    glCopyTexImage2D(target, level, internalformat, x, y, width, height, border);

    GLES2_BARRIER_RET;
}


GLES2_CB(glCopyTexSubImage2D)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLint, level);
    GLES2_ARG(TGLint, xoffset);
    GLES2_ARG(TGLint, yoffset);
    GLES2_ARG(TGLint, x);
    GLES2_ARG(TGLint, y);
    GLES2_ARG(TGLsizei, width);
    GLES2_ARG(TGLsizei, height);

    GLES2_BARRIER_ARG;

    glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);

    GLES2_BARRIER_RET;
}

GLES2_CB(glCreateProgram)//(void)
{
    GLES2_BARRIER_ARG;

    GLES2_BARRIER_RET;
    gles2_ret_TGLuint(s, glCreateProgram());
}

GLES2_CB(glDeleteProgram)//(GLuint program)
{
    GLES2_ARG(TGLuint, program);
    GLES2_BARRIER_ARG_NORET;

    glDeleteProgram(program);
}

#if 0
GL_APICALL void GL_APIENTRY glDetachShader(GLuint program, GLuint shader)
{
    DUMMY();
}

GL_APICALL void GL_APIENTRY glGetActiveAttrib (GLuint program, GLuint index,
    GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name)
{
    DUMMY();
}
#endif // 0

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

    glGetActiveUniform(program, index, bufsize, &length, &size, &type, name);

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

#if 0
GL_APICALL void GL_APIENTRY glGetAttachedShaders (GLuint program,
    GLsizei maxcount, GLsizei* count, GLuint* shaders)
{
    DUMMY();
}
#endif // 0

GLES2_CB(glGetAttribLocation)
{
    GLES2_ARG(TGLuint, program);
    GLES2_ARG(Tptr, namep);

    char name[120];
    Tptr i;

    for (i = 0; (name[i] = gles2_get_byte(s, namep + i)) ; ++i);

    GLES2_PRINT("Getting attribute %s location...\n", name);

    gles2_ret_TGLint(s, glGetAttribLocation(program, name));
    GLES2_BARRIER_ARG_NORET;
}

GLES2_CB(glGetProgramiv)
{
    GLES2_ARG(TGLuint, program);
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(Tptr, paramsp);
    GLES2_BARRIER_ARG;

    GLint param;

    glGetProgramiv(program, pname, &param);
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
    glGetProgramInfoLog(program, bufsize, &length, infolog);
    gles2_transfer(s, infologp, length, infolog, 1);
    gles2_put_TGLsizei(s, lengthp, length);
    GLES2_BARRIER_ARG_NORET;
    GLES2_PRINT("program %d infolog:\n%.*s\n", program, length, infolog);
    free(infolog);
}

#if 0
GL_APICALL void GL_APIENTRY glGetUniformfv(GLuint program, GLint location, GLfloat* params)
{
    DUMMY();
}

GL_APICALL void GL_APIENTRY glGetUniformiv(GLuint program, GLint location, GLint* params)
{
    DUMMY();
}
#endif // 0

GLES2_CB(glGetUniformLocation)
{
    GLES2_ARG(TGLuint, program);
    GLES2_ARG(Tptr, namep);

    char name[120];
    Tptr i;

    for (i = 0; (name[i] = gles2_get_byte(s, namep + i)) ; ++i);

    GLES2_PRINT("Getting uniform %s location...\n", name);

    gles2_ret_TGLint(s, glGetUniformLocation(program, name));
    GLES2_BARRIER_ARG_NORET;
}

GLES2_CB(glIsProgram)
{
    GLES2_ARG(TGLuint, program);
    GLES2_BARRIER_ARG;

    GLES2_BARRIER_RET;
    gles2_ret_TGLboolean(s, glIsProgram(program));
}

GLES2_CB(glLinkProgram)
{
    GLES2_ARG(TGLuint, program);
    GLES2_BARRIER_ARG_NORET;

    glLinkProgram(program);
}

GLES2_CB(glUniform1f)
{
    GLES2_ARG(TGLint, location);
    GLES2_ARG(TGLfloat, x);
    GLES2_BARRIER_ARG_NORET;

    glUniform1f(location, x);
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

    glUniform1fv(location, count, v);
    free(v);
}

GLES2_CB(glUniform1i)//(GLint location, GLint x)
{
    GLES2_ARG(TGLint, location);
    GLES2_ARG(TGLint, x);
    GLES2_BARRIER_ARG_NORET;

    glUniform1i(location, x);
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

    glUniform1iv(location, count, v);
    free(v);
}

GLES2_CB(glUniform2f)
{
    GLES2_ARG(TGLint, location);
    GLES2_ARG(TGLfloat, x);
    GLES2_ARG(TGLfloat, y);
    GLES2_BARRIER_ARG_NORET;

    glUniform2f(location, x, y);
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

    glUniform2fv(location, count, v);
    free(v);
}

GLES2_CB(glUniform2i)
{
    GLES2_ARG(TGLint, location);
    GLES2_ARG(TGLint, x);
    GLES2_ARG(TGLint, y);
    GLES2_BARRIER_ARG_NORET;

    glUniform2i(location, x, y);
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

    glUniform2iv(location, count, v);
    free(v);
}

GLES2_CB(glUniform3f)
{
    GLES2_ARG(TGLint, location);
    GLES2_ARG(TGLfloat, x);
    GLES2_ARG(TGLfloat, y);
    GLES2_ARG(TGLfloat, z);
    GLES2_BARRIER_ARG_NORET;

    glUniform3f(location, x, y, z);
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

    glUniform3fv(location, count, v);
    free(v);
}

GLES2_CB(glUniform3i)
{
    GLES2_ARG(TGLint, location);
    GLES2_ARG(TGLint, x);
    GLES2_ARG(TGLint, y);
    GLES2_ARG(TGLint, z);
    GLES2_BARRIER_ARG_NORET;

    glUniform3i(location, x, y, z);
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

    glUniform3iv(location, count, v);
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

    glUniform4f(location, x, y, z, w);
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
    glUniform4fv(location, count, v);
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

    glUniform4i(location, x, y, z, w);
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
    glUniform4iv(location, count, v);
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

    glUniformMatrix2fv(location, count, transpose, value);
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

    glUniformMatrix3fv(location, count, transpose, value);
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

    glUniformMatrix4fv(location, count, transpose, value);
    free(value);
}

GLES2_CB(glUseProgram)
{
    GLES2_ARG(TGLuint, program);
    GLES2_BARRIER_ARG_NORET;

    glUseProgram(program);
}

GLES2_CB(glBlendColor)
{
    GLES2_ARG(TGLclampf, red);
    GLES2_ARG(TGLclampf, green);
    GLES2_ARG(TGLclampf, blue);
    GLES2_ARG(TGLclampf, alpha);
    GLES2_BARRIER_ARG_NORET;

    glBlendColor(red, green, blue, alpha);
}

GLES2_CB(glBlendEquation)
{
    GLES2_ARG(TGLenum, mode);
    GLES2_BARRIER_ARG_NORET;

    glBlendEquation(mode);
}

GLES2_CB(glBlendEquationSeparate)
{
    GLES2_ARG(TGLenum, modeRGB);
    GLES2_ARG(TGLenum, modeAlpha);
    GLES2_BARRIER_ARG_NORET;

    glBlendEquationSeparate(modeRGB, modeAlpha);
}

GLES2_CB(glBlendFunc)
{
    GLES2_ARG(TGLenum, sfactor);
    GLES2_ARG(TGLenum, dfactor);
    GLES2_BARRIER_ARG_NORET;

    glBlendFunc(sfactor, dfactor);
}

GLES2_CB(glBlendFuncSeparate)
{
    GLES2_ARG(TGLenum, srcRGB);
    GLES2_ARG(TGLenum, dstRGB);
    GLES2_ARG(TGLenum, srcAlpha);
    GLES2_ARG(TGLenum, dstAlpha);
    GLES2_BARRIER_ARG_NORET;

    glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

GLES2_CB(glPixelStorei)
{
    GLES2_ARG(TGLenum, pname);
    GLES2_ARG(TGLint, param);
    GLES2_BARRIER_ARG_NORET;

    glPixelStorei(pname, param);
}

#if 0
GL_APICALL void GL_APIENTRY glValidateProgram (GLuint program)
{
    DUMMY();
}
#endif // 0

GLES2_CB(glClearStencil)
{
    GLES2_ARG(TGLint, s_);
    GLES2_BARRIER_ARG_NORET;

    glClearStencil(s_);
}

GLES2_CB(glStencilFunc)
{
    GLES2_ARG(TGLenum, func);
    GLES2_ARG(TGLint, ref);
    GLES2_ARG(TGLuint, mask);
    GLES2_BARRIER_ARG_NORET;

    glStencilFunc(func, ref, mask);
}

GLES2_CB(glStencilFuncSeparate)
{
    GLES2_ARG(TGLenum, face);
    GLES2_ARG(TGLenum, func);
    GLES2_ARG(TGLint, ref);
    GLES2_ARG(TGLuint, mask);
    GLES2_BARRIER_ARG_NORET;

    glStencilFuncSeparate(face, func, ref, mask);
}

GLES2_CB(glStencilMask)
{
    GLES2_ARG(TGLuint, mask);
    GLES2_BARRIER_ARG_NORET;

    glStencilMask(mask);
}

GLES2_CB(glStencilMaskSeparate)
{
    GLES2_ARG(TGLenum, face);
    GLES2_ARG(TGLuint, mask);
    GLES2_BARRIER_ARG_NORET;

    glStencilMaskSeparate(face, mask);
}

GLES2_CB(glStencilOp)
{
    GLES2_ARG(TGLenum, fail);
    GLES2_ARG(TGLenum, zfail);
    GLES2_ARG(TGLenum, zpass);
    GLES2_BARRIER_ARG_NORET;

    glStencilOp(fail, zfail, zpass);
}

GLES2_CB(glStencilOpSeparate)
{
    GLES2_ARG(TGLenum, face);
    GLES2_ARG(TGLenum, fail);
    GLES2_ARG(TGLenum, zfail);
    GLES2_ARG(TGLenum, zpass);
    GLES2_BARRIER_ARG_NORET;

    glStencilOpSeparate(face, fail, zfail, zpass);
}

GLES2_CB(glBindFramebuffer)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLuint, framebuffer);
    GLES2_BARRIER_ARG_NORET;

    glBindFramebuffer(target, framebuffer);
}

GLES2_CB(glBindRenderbuffer)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLuint, renderbuffer);
    GLES2_BARRIER_ARG_NORET;

    glBindRenderbuffer(target, renderbuffer);
}

GLES2_CB(glCheckFramebufferStatus)
{
    GLES2_ARG(TGLenum, target);
    GLES2_BARRIER_ARG;

    GLES2_BARRIER_RET;
    gles2_ret_TGLenum(s, glCheckFramebufferStatus(target));
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

    glDeleteFramebuffers(n, framebuffers);
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

    glDeleteRenderbuffers(n, renderbuffers);
    free(renderbuffers);
}

GLES2_CB(glFramebufferRenderbuffer)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLenum, attachment);
    GLES2_ARG(TGLenum, renderbuffertarget);
    GLES2_ARG(TGLuint, renderbuffer);
    GLES2_BARRIER_ARG_NORET;

    glFramebufferRenderbuffer(target, attachment,
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

    glFramebufferTexture2D(target, attachment, textarget, texture, level);
}

GLES2_CB(glGenFramebuffers)
{
    GLES2_ARG(TGLsizei, n);
    GLES2_ARG(Tptr, framebuffersp);

    GLsizei i;
    GLuint* framebuffers = (GLuint*)malloc(sizeof(GLuint)*n);

    glGenFramebuffers(n, framebuffers);
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

    glGenRenderbuffers(n, renderbuffers);
    for(i = 0; i < n; ++i) {
        gles2_put_TGLuint(s, renderbuffersp + i*sizeof(TGLuint),
            renderbuffers[i]);
    }
    GLES2_BARRIER_ARG_NORET;

    free(renderbuffers);
}

#if 0
GLES2_CB(glGetFramebufferAttachmentParameteriv)//(GLenum target,
    GLenum attachment, GLenum pname, GLint* params)
{
	DUMMY();
}

GLES2_CB(glGetRenderbufferParameteriv)//(GLenum target, GLenum pname,
    GLint* params)
{
	DUMMY();
}

GLES2_CB(glIsFramebuffer)//(GLuint framebuffer)
{
	DUMMY();
}

GLES2_CB(glIsRenderbuffer)//(GLuint renderbuffer)
{
	DUMMY();
}
#endif // 0

GLES2_CB(glBindBuffer)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLuint, buffer);
    GLES2_BARRIER_ARG_NORET;

    glBindTexture(target, buffer);
}


GLES2_CB(glRenderbufferStorage)
{
    GLES2_ARG(TGLenum, target);
    GLES2_ARG(TGLenum, internalformat);
    GLES2_ARG(TGLsizei, width);
    GLES2_ARG(TGLsizei, height);
    GLES2_BARRIER_ARG_NORET;

    glRenderbufferStorage(target, internalformat, width, height);
}

GLES2_CB(glDepthFunc)
{
	GLES2_ARG(TGLenum, func);
	GLES2_BARRIER_ARG_NORET;

	glDepthFunc(func);
}

GLES2_CB(glDepthMask)
{
	GLES2_ARG(TGLboolean, flag);
	GLES2_BARRIER_ARG_NORET;

	glDepthMask(flag);
}

GLES2_CB(glDepthRangef)
{
	GLES2_ARG(TGLclampf, zNear);
	GLES2_ARG(TGLclampf, zFar);
	GLES2_BARRIER_ARG_NORET;

	glDepthRangef(zNear, zFar);
}

GLES2_CB(glClearDepthf)
{
	GLES2_ARG(TGLclampf, depth);
	GLES2_BARRIER_ARG_NORET;

	glClearDepthf(depth);
}
