#include "GLES/gl.h"

#define GLES2_HGL_FUNCS \
    GLES2_HGL_FUNC(void,glAlphaFunc,(GLenum func, GLclampf ref)) \
    GLES2_HGL_FUNC(void,glClearColor,(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)) \
    GLES2_HGL_FUNC(void,glClearDepthf,(GLclampf depth)) \
    GLES2_HGL_FUNC(void,glClipPlanef,(GLenum plane, const GLfloat *equation)) \
    GLES2_HGL_FUNC(void,glColor4f,(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)) \
    GLES2_HGL_FUNC(void,glDepthRangef,(GLclampf zNear, GLclampf zFar)) \
    GLES2_HGL_FUNC(void,glFogf,(GLenum pname, GLfloat param)) \
    GLES2_HGL_FUNC(void,glFogfv,(GLenum pname, const GLfloat *params)) \
    GLES2_HGL_FUNC(void,glFrustumf,(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar)) \
    GLES2_HGL_FUNC(void,glGetClipPlanef,(GLenum pname, GLfloat eqn[4])) \
    GLES2_HGL_FUNC(void,glGetFloatv,(GLenum pname, GLfloat *params)) \
    GLES2_HGL_FUNC(void,glGetLightfv,(GLenum light, GLenum pname, GLfloat *params)) \
    GLES2_HGL_FUNC(void,glGetMaterialfv,(GLenum face, GLenum pname, GLfloat *params)) \
    GLES2_HGL_FUNC(void,glGetTexEnvfv,(GLenum env, GLenum pname, GLfloat *params)) \
    GLES2_HGL_FUNC(void,glGetTexParameterfv,(GLenum target, GLenum pname, GLfloat *params)) \
    GLES2_HGL_FUNC(void,glLightModelf,(GLenum pname, GLfloat param)) \
    GLES2_HGL_FUNC(void,glLightModelfv,(GLenum pname, const GLfloat *params)) \
    GLES2_HGL_FUNC(void,glLightf,(GLenum light, GLenum pname, GLfloat param)) \
    GLES2_HGL_FUNC(void,glLightfv,(GLenum light, GLenum pname, const GLfloat *params)) \
    GLES2_HGL_FUNC(void,glLineWidth,(GLfloat width)) \
    GLES2_HGL_FUNC(void,glLoadMatrixf,(const GLfloat *m)) \
    GLES2_HGL_FUNC(void,glMaterialf,(GLenum face, GLenum pname, GLfloat param)) \
    GLES2_HGL_FUNC(void,glMaterialfv,(GLenum face, GLenum pname, const GLfloat *params)) \
    GLES2_HGL_FUNC(void,glMultMatrixf,(const GLfloat *m)) \
    GLES2_HGL_FUNC(void,glMultiTexCoord4f,(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q)) \
    GLES2_HGL_FUNC(void,glNormal3f,(GLfloat nx, GLfloat ny, GLfloat nz)) \
    GLES2_HGL_FUNC(void,glOrthof,(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar)) \
    GLES2_HGL_FUNC(void,glPointParameterf,(GLenum pname, GLfloat param)) \
    GLES2_HGL_FUNC(void,glPointParameterfv,(GLenum pname, const GLfloat *params)) \
    GLES2_HGL_FUNC(void,glPointSize,(GLfloat size)) \
    GLES2_HGL_FUNC(void,glPolygonOffset,(GLfloat factor, GLfloat units)) \
    GLES2_HGL_FUNC(void,glRotatef,(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)) \
    GLES2_HGL_FUNC(void,glScalef,(GLfloat x, GLfloat y, GLfloat z)) \
    GLES2_HGL_FUNC(void,glTexEnvf,(GLenum target, GLenum pname, GLfloat param)) \
    GLES2_HGL_FUNC(void,glTexEnvfv,(GLenum target, GLenum pname, const GLfloat *params)) \
    GLES2_HGL_FUNC(void,glTexParameterf,(GLenum target, GLenum pname, GLfloat param)) \
    GLES2_HGL_FUNC(void,glTexParameterfv,(GLenum target, GLenum pname, const GLfloat *params)) \
    GLES2_HGL_FUNC(void,glTranslatef,(GLfloat x, GLfloat y, GLfloat z)) \
    GLES2_HGL_FUNC(void,glActiveTexture,(GLenum texture)) \
    GLES2_HGL_FUNC(void,glAlphaFuncx,(GLenum func, GLclampx ref)) \
    GLES2_HGL_FUNC(void,glBindBuffer,(GLenum target, GLuint buffer)) \
    GLES2_HGL_FUNC(void,glBindTexture,(GLenum target, GLuint texture)) \
    GLES2_HGL_FUNC(void,glBlendFunc,(GLenum sfactor, GLenum dfactor)) \
    GLES2_HGL_FUNC(void,glBufferData,(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage)) \
    GLES2_HGL_FUNC(void,glBufferSubData,(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data)) \
    GLES2_HGL_FUNC(void,glClear,(GLbitfield mask)) \
    GLES2_HGL_FUNC(void,glClearColorx,(GLclampx red, GLclampx green, GLclampx blue, GLclampx alpha)) \
    GLES2_HGL_FUNC(void,glClearDepthx,(GLclampx depth)) \
    GLES2_HGL_FUNC(void,glClearStencil,(GLint s)) \
    GLES2_HGL_FUNC(void,glClientActiveTexture,(GLenum texture)) \
    GLES2_HGL_FUNC(void,glClipPlanex,(GLenum plane, const GLfixed *equation)) \
    GLES2_HGL_FUNC(void,glColor4ub,(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)) \
    GLES2_HGL_FUNC(void,glColor4x,(GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha)) \
    GLES2_HGL_FUNC(void,glColorMask,(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)) \
    GLES2_HGL_FUNC(void,glColorPointer,(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)) \
    GLES2_HGL_FUNC(void,glCompressedTexImage2D,(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data)) \
    GLES2_HGL_FUNC(void,glCompressedTexSubImage2D,(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data)) \
    GLES2_HGL_FUNC(void,glCopyTexImage2D,(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)) \
    GLES2_HGL_FUNC(void,glCopyTexSubImage2D,(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)) \
    GLES2_HGL_FUNC(void,glCullFace,(GLenum mode)) \
    GLES2_HGL_FUNC(void,glDeleteBuffers,(GLsizei n, const GLuint *buffers)) \
    GLES2_HGL_FUNC(void,glDeleteTextures,(GLsizei n, const GLuint *textures)) \
    GLES2_HGL_FUNC(void,glDepthFunc,(GLenum func)) \
    GLES2_HGL_FUNC(void,glDepthMask,(GLboolean flag)) \
    GLES2_HGL_FUNC(void,glDepthRangex,(GLclampx zNear, GLclampx zFar)) \
    GLES2_HGL_FUNC(void,glDisable,(GLenum cap)) \
    GLES2_HGL_FUNC(void,glDisableClientState,(GLenum array)) \
    GLES2_HGL_FUNC(void,glDrawArrays,(GLenum mode, GLint first, GLsizei count)) \
    GLES2_HGL_FUNC(void,glDrawElements,(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)) \
    GLES2_HGL_FUNC(void,glEnable,(GLenum cap)) \
    GLES2_HGL_FUNC(void,glEnableClientState,(GLenum array)) \
    GLES2_HGL_FUNC(void,glFinish,(void)) \
    GLES2_HGL_FUNC(void,glFlush,(void)) \
    GLES2_HGL_FUNC(void,glFogx,(GLenum pname, GLfixed param)) \
    GLES2_HGL_FUNC(void,glFogxv,(GLenum pname, const GLfixed *params)) \
    GLES2_HGL_FUNC(void,glFrontFace,(GLenum mode)) \
    GLES2_HGL_FUNC(void,glFrustumx,(GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar)) \
    GLES2_HGL_FUNC(void,glGetBooleanv,(GLenum pname, GLboolean *params)) \
    GLES2_HGL_FUNC(void,glGetBufferParameteriv,(GLenum target, GLenum pname, GLint *params)) \
    GLES2_HGL_FUNC(void,glGetClipPlanex,(GLenum pname, GLfixed eqn[4])) \
    GLES2_HGL_FUNC(void,glGenBuffers,(GLsizei n, GLuint *buffers)) \
    GLES2_HGL_FUNC(void,glGenTextures,(GLsizei n, GLuint *textures)) \
    GLES2_HGL_FUNC(GLenum,glGetError,(void)) \
    GLES2_HGL_FUNC(void,glGetFixedv,(GLenum pname, GLfixed *params)) \
    GLES2_HGL_FUNC(void,glGetIntegerv,(GLenum pname, GLint *params)) \
    GLES2_HGL_FUNC(void,glGetLightxv,(GLenum light, GLenum pname, GLfixed *params)) \
    GLES2_HGL_FUNC(void,glGetMaterialxv,(GLenum face, GLenum pname, GLfixed *params)) \
    GLES2_HGL_FUNC(void,glGetPointerv,(GLenum pname, GLvoid **params)) \
    GLES2_HGL_FUNC(const GLubyte *,glGetString,(GLenum name)) \
    GLES2_HGL_FUNC(void,glGetTexEnviv,(GLenum env, GLenum pname, GLint *params)) \
    GLES2_HGL_FUNC(void,glGetTexEnvxv,(GLenum env, GLenum pname, GLfixed *params)) \
    GLES2_HGL_FUNC(void,glGetTexParameteriv,(GLenum target, GLenum pname, GLint *params)) \
    GLES2_HGL_FUNC(void,glGetTexParameterxv,(GLenum target, GLenum pname, GLfixed *params)) \
    GLES2_HGL_FUNC(void,glHint,(GLenum target, GLenum mode)) \
    GLES2_HGL_FUNC(GLboolean,glIsBuffer,(GLuint buffer)) \
    GLES2_HGL_FUNC(GLboolean,glIsEnabled,(GLenum cap)) \
    GLES2_HGL_FUNC(GLboolean,glIsTexture,(GLuint texture)) \
    GLES2_HGL_FUNC(void,glLightModelx,(GLenum pname, GLfixed param)) \
    GLES2_HGL_FUNC(void,glLightModelxv,(GLenum pname, const GLfixed *params)) \
    GLES2_HGL_FUNC(void,glLightx,(GLenum light, GLenum pname, GLfixed param)) \
    GLES2_HGL_FUNC(void,glLightxv,(GLenum light, GLenum pname, const GLfixed *params)) \
    GLES2_HGL_FUNC(void,glLineWidthx,(GLfixed width)) \
    GLES2_HGL_FUNC(void,glLoadIdentity,(void)) \
    GLES2_HGL_FUNC(void,glLoadMatrixx,(const GLfixed *m)) \
    GLES2_HGL_FUNC(void,glLogicOp,(GLenum opcode)) \
    GLES2_HGL_FUNC(void,glMaterialx,(GLenum face, GLenum pname, GLfixed param)) \
    GLES2_HGL_FUNC(void,glMaterialxv,(GLenum face, GLenum pname, const GLfixed *params)) \
    GLES2_HGL_FUNC(void,glMatrixMode,(GLenum mode)) \
    GLES2_HGL_FUNC(void,glMultMatrixx,(const GLfixed *m)) \
    GLES2_HGL_FUNC(void,glMultiTexCoord4x,(GLenum target, GLfixed s, GLfixed t, GLfixed r, GLfixed q)) \
    GLES2_HGL_FUNC(void,glNormal3x,(GLfixed nx, GLfixed ny, GLfixed nz)) \
    GLES2_HGL_FUNC(void,glNormalPointer,(GLenum type, GLsizei stride, const GLvoid *pointer)) \
    GLES2_HGL_FUNC(void,glOrthox,(GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar)) \
    GLES2_HGL_FUNC(void,glPixelStorei,(GLenum pname, GLint param)) \
    GLES2_HGL_FUNC(void,glPointParameterx,(GLenum pname, GLfixed param)) \
    GLES2_HGL_FUNC(void,glPointParameterxv,(GLenum pname, const GLfixed *params)) \
    GLES2_HGL_FUNC(void,glPointSizex,(GLfixed size)) \
    GLES2_HGL_FUNC(void,glPolygonOffsetx,(GLfixed factor, GLfixed units)) \
    GLES2_HGL_FUNC(void,glPopMatrix,(void)) \
    GLES2_HGL_FUNC(void,glPushMatrix,(void)) \
    GLES2_HGL_FUNC(void,glReadPixels,(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)) \
    GLES2_HGL_FUNC(void,glRotatex,(GLfixed angle, GLfixed x, GLfixed y, GLfixed z)) \
    GLES2_HGL_FUNC(void,glSampleCoverage,(GLclampf value, GLboolean invert)) \
    GLES2_HGL_FUNC(void,glSampleCoveragex,(GLclampx value, GLboolean invert)) \
    GLES2_HGL_FUNC(void,glScalex,(GLfixed x, GLfixed y, GLfixed z)) \
    GLES2_HGL_FUNC(void,glScissor,(GLint x, GLint y, GLsizei width, GLsizei height)) \
    GLES2_HGL_FUNC(void,glShadeModel,(GLenum mode)) \
    GLES2_HGL_FUNC(void,glStencilFunc,(GLenum func, GLint ref, GLuint mask)) \
    GLES2_HGL_FUNC(void,glStencilMask,(GLuint mask)) \
    GLES2_HGL_FUNC(void,glStencilOp,(GLenum fail, GLenum zfail, GLenum zpass)) \
    GLES2_HGL_FUNC(void,glTexCoordPointer,(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)) \
    GLES2_HGL_FUNC(void,glTexEnvi,(GLenum target, GLenum pname, GLint param)) \
    GLES2_HGL_FUNC(void,glTexEnvx,(GLenum target, GLenum pname, GLfixed param)) \
    GLES2_HGL_FUNC(void,glTexEnviv,(GLenum target, GLenum pname, const GLint *params)) \
    GLES2_HGL_FUNC(void,glTexEnvxv,(GLenum target, GLenum pname, const GLfixed *params)) \
    GLES2_HGL_FUNC(void,glTexImage2D,(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)) \
    GLES2_HGL_FUNC(void,glTexParameteri,(GLenum target, GLenum pname, GLint param)) \
    GLES2_HGL_FUNC(void,glTexParameterx,(GLenum target, GLenum pname, GLfixed param)) \
    GLES2_HGL_FUNC(void,glTexParameteriv,(GLenum target, GLenum pname, const GLint *params)) \
    GLES2_HGL_FUNC(void,glTexParameterxv,(GLenum target, GLenum pname, const GLfixed *params)) \
    GLES2_HGL_FUNC(void,glTexSubImage2D,(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)) \
    GLES2_HGL_FUNC(void,glTranslatex,(GLfixed x, GLfixed y, GLfixed z)) \
    GLES2_HGL_FUNC(void,glVertexPointer,(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)) \
    GLES2_HGL_FUNC(void,glViewport,(GLint x, GLint y, GLsizei width, GLsizei height)) \
    GLES2_HGL_FUNC(void,glPointSizePointerOES,(GLenum type, GLsizei stride, const GLvoid *pointer))

    typedef struct HGL {
    #define GLES2_HGL_FUNC(ret,name,attr) ret GL_APIENTRY (*name)attr;
    GLES2_HGL_FUNCS
    #undef GLES2_HGL_FUNC
    } HGL;
