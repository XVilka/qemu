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

#undef GLES2_CB
#define GLES2_CB(func) \
    CB(func,egl)

int surf_id = 1;
/**
 * used to know the index in %gles2_Client context array
 * where contexts of type context_client_type are stored
 * @param context_client_type is the client API.
 */
static int gles2_getContextArrayIndex(EGLenum context_client_type)
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


GLES2_CB(eglGetError)
{
    GLES2_BARRIER_ARG;
    EGLint ret = eglGetError();
    GLES2_BARRIER_RET;
    gles2_ret_TGLint(s, ret);
}

GLES2_CB(eglBindAPI)
{
    GLES2_ARG(TEGLenum, api);
    GLES2_BARRIER_ARG;

    EGLBoolean ret = eglBindAPI(api);
    if(ret)
        c->rendering_api = api;
    GLES2_BARRIER_RET;
    gles2_ret_TEGLBoolean(s, ret);
}

GLES2_CB(eglGetDisplay)
{
//  GLES2_ARG(TEGLDisplay, dpy);
//  (void)dpy;
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

    GLES2_BARRIER_ARG_NORET;

    EGLDisplay dpy = (EGLDisplay)gles2_handle_get(s, dpy_);

    GLES2_PRINT("Request to initialize display %p...\n", dpy);

    EGLint major, minor;
    if (eglInitialize(dpy, &major, &minor)) {
        GLES2_PRINT("Display initialized (EGL %d.%d)!\n", major, minor);
        //GLES2_BARRIER_RET;
        gles2_put_TEGLint(s, majorp, major);
        gles2_put_TEGLint(s, minorp, minor);
        gles2_ret_TEGLBoolean(s, EGL_TRUE);
        return;
    }

    GLES2_PRINT("Failed to initialize...\n");
    //GLES2_BARRIER_RET;
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

    EGLDisplay dpy = (EGLDisplay)gles2_handle_get(s, dpy_);

    GLES2_BARRIER_ARG_NORET;
    EGLConfig* configs = configsp ? malloc(sizeof(EGLConfig)*config_size) : NULL;

    EGLint num_config;
    EGLBoolean ret = eglChooseConfig(dpy, attrib_list, configs, config_size, &num_config);
    free(attrib_list);
    //GLES2_BARRIER_RET;
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
    gles2_Context* share_context = gles2_handle_get(s, share_context_);


    EGLint *attribs= NULL;

    if (attrib_listp) {
        EGLint list_n = 1;
        EGLint name = gles2_get_TEGLint(s, attrib_listp);
        while ( name != EGL_NONE) {
            list_n += 2;
            name = gles2_get_TEGLint(s, attrib_listp
            + (list_n-1)*sizeof(EGLint));
        }
        attribs = malloc(list_n*sizeof(EGLint));
        int i;
        for (i=0; i< list_n; i++) {

                    attribs[i] = gles2_get_TEGLint(s, attrib_listp
                    + i*sizeof(EGLint));
        }
    }

    GLES2_PRINT("Host context creation requested...\n");
    EGLContext hctx = eglCreateContext(dpy, config, share_context?share_context->hctx:NULL, attribs);
    GLES2_BARRIER_RET;

    if (hctx == EGL_NO_CONTEXT) {
        GLES2_PRINT("Context creation failed!\n");
        (void)attrib_listp;
        gles2_ret_TEGLContext(s, 0);
    } else {
        gles2_Context * ctx = malloc(sizeof(gles2_Context));
        ctx->hctx = hctx;
        int version = 1;
        //if OpenGL ES check for client version from attrib_list
        if(c->rendering_api == EGL_OPENGL_ES_API && attrib_listp) {
            EGLint attrib_list_n = 0;
            EGLint attribname = gles2_get_TEGLint(s, attrib_listp
                + attrib_list_n*sizeof(EGLint));
            while ( attribname != EGL_NONE &&
                    attribname != EGL_CONTEXT_CLIENT_VERSION) {
                attrib_list_n += 2;
                attribname = gles2_get_TEGLint(s, attrib_listp
                + attrib_list_n*sizeof(EGLint));
            }
            if (attribname == EGL_CONTEXT_CLIENT_VERSION) {
                attrib_list_n++;
                version = gles2_get_TEGLint(s, attrib_listp
                    + attrib_list_n*sizeof(EGLint));
            }
        }

        ctx->client_type = c->rendering_api;
        ctx->client_version = version;
        ctx->arrays = NULL;
        ctx->narrays = 0;
        if (attribs)
            free(attribs);
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
    gles2_Context* ctx = (gles2_Context*) gles2_handle_get(s, ctx_);
    EGLBoolean ret = eglDestroyContext(dpy, ctx->hctx);
    if (ret) {
        if (ctx->arrays)
            free(ctx->arrays);
        free(ctx);
        gles2_handle_free(s, ctx_);
        GLES2_PRINT("Destroyed %p!\n", ctx);
    }

    GLES2_BARRIER_RET;
    gles2_ret_TEGLBoolean(s,ret);
}

GLES2_CB(eglMakeCurrent)
{
    GLES2_ARG(TEGLDisplay, dpy_);
    GLES2_ARG(TEGLSurface, draw_);
    GLES2_ARG(TEGLSurface, read_);
    GLES2_ARG(TEGLContext, ctx_);
    GLES2_BARRIER_ARG;

    EGLDisplay dpy = (EGLDisplay)gles2_handle_get(s, dpy_);
    gles2_Surface* draw = (EGLSurface)gles2_handle_get(s, draw_);
    gles2_Surface* read = (EGLSurface)gles2_handle_get(s, read_);
    gles2_Context* ctx = (gles2_Context*)gles2_handle_get(s, ctx_);

    GLES2_PRINT("Making host context current...\n");

    if (!eglMakeCurrent(dpy,
        draw ? draw->surf : NULL,
        read ? read->surf : NULL,
        ctx?ctx->hctx:NULL)) {
        GLES2_PRINT("\tMakeCurrent failed!\n");
        GLES2_BARRIER_RET;
        gles2_ret_TEGLBoolean(s, EGL_FALSE);
        return;
    }

    // Initialize context arrays.
    //TODO: if arrays already exist then keep them as is?
    if (ctx && !ctx->arrays) {
        if (ctx->client_type == EGL_OPENGL_ES_API) {
            if(ctx->client_version == 1) {
                ctx->narrays = 6;
                ctx->arrays = malloc(ctx->narrays * sizeof(*ctx->arrays));
            }
            else if(ctx->client_version == 2) {
        //      glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &ctx->narrays);
            //FIXME: temporary hack to avoid dependency
                ctx->narrays = 16;
                ctx->arrays = malloc(ctx->narrays * sizeof(*ctx->arrays));
                GLES2_PRINT("Maximum number of host vertex arrays: %d.\n", ctx->narrays);
            }
            int i;
            for (i = 0; i < ctx->narrays; ++i) {
                    ctx->arrays[i].type = GL_NONE;
                    ctx->arrays[i].enabled = 0;
                    ctx->arrays[i].ptr = 0;
                    ctx->arrays[i].apply = 0;
                    ctx->arrays[i].tptr = 0;
            }
        }
    }

    int context_index = gles2_getContextArrayIndex(ctx?ctx->client_type:c->rendering_api);
    //TODO: Find out if we need to do anything with old context here
    c->context[context_index] = ctx;
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
       // glFinish();
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

    GLES2_PRINT("Swapping DGLES surface ID = %d!\n", fsurf->id);
    eglSwapBuffers(dpy, fsurf->surf);

    GLES2_PRINT("Transferring frame!\n");
    gles2_transfer_exec(&fsurf->tfr, s, fsurf->ddraw.pixels, 1);
    GLES2_PRINT("\tDone!\n");
    GLES2_BARRIER_RET;
    gles2_ret_TEGLBoolean(s, EGL_TRUE);
}
