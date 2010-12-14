#include "GLES2/gl2.h"

#define GLES2_HGL_FUNCS \
    GLES2_HGL_FUNC(void,glActiveTexture,(GLenum texture)) \
    GLES2_HGL_FUNC(void,glAttachShader,(GLuint program, GLuint shader)) \
    GLES2_HGL_FUNC(void,glBindAttribLocation,(GLuint program, GLuint index, const char* name)) \
    GLES2_HGL_FUNC(void,glBindBuffer,(GLenum target, GLuint buffer)) \
    GLES2_HGL_FUNC(void,glBindFramebuffer,(GLenum target, GLuint framebuffer)) \
    GLES2_HGL_FUNC(void,glBindRenderbuffer,(GLenum target, GLuint renderbuffer)) \
    GLES2_HGL_FUNC(void,glBindTexture,(GLenum target, GLuint texture)) \
    GLES2_HGL_FUNC(void,glBlendColor,(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)) \
    GLES2_HGL_FUNC(void,glBlendEquation,( GLenum mode )) \
    GLES2_HGL_FUNC(void,glBlendEquationSeparate,(GLenum modeRGB, GLenum modeAlpha)) \
    GLES2_HGL_FUNC(void,glBlendFunc,(GLenum sfactor, GLenum dfactor)) \
    GLES2_HGL_FUNC(void,glBlendFuncSeparate,(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)) \
    GLES2_HGL_FUNC(void,glBufferData,(GLenum target, GLsizeiptr size, const void* data, GLenum usage)) \
    GLES2_HGL_FUNC(void,glBufferSubData,(GLenum target, GLintptr offset, GLsizeiptr size, const void* data)) \
    GLES2_HGL_FUNC(GLenum,glCheckFramebufferStatus,(GLenum target)) \
    GLES2_HGL_FUNC(void,glClear,(GLbitfield mask)) \
    GLES2_HGL_FUNC(void,glClearColor,(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)) \
    GLES2_HGL_FUNC(void,glClearDepthf,(GLclampf depth)) \
    GLES2_HGL_FUNC(void,glClearStencil,(GLint s)) \
    GLES2_HGL_FUNC(void,glColorMask,(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)) \
    GLES2_HGL_FUNC(void,glCompileShader,(GLuint shader)) \
    GLES2_HGL_FUNC(void,glCompressedTexImage2D,(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data)) \
    GLES2_HGL_FUNC(void,glCompressedTexSubImage2D,(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data)) \
    GLES2_HGL_FUNC(void,glCopyTexImage2D,(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)) \
    GLES2_HGL_FUNC(void,glCopyTexSubImage2D,(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)) \
    GLES2_HGL_FUNC(GLuint,glCreateProgram,(void)) \
    GLES2_HGL_FUNC(GLuint,glCreateShader,(GLenum type)) \
    GLES2_HGL_FUNC(void,glCullFace,(GLenum mode)) \
    GLES2_HGL_FUNC(void,glDeleteBuffers,(GLsizei n, const GLuint* buffers)) \
    GLES2_HGL_FUNC(void,glDeleteFramebuffers,(GLsizei n, const GLuint* framebuffers)) \
    GLES2_HGL_FUNC(void,glDeleteProgram,(GLuint program)) \
    GLES2_HGL_FUNC(void,glDeleteRenderbuffers,(GLsizei n, const GLuint* renderbuffers)) \
    GLES2_HGL_FUNC(void,glDeleteShader,(GLuint shader)) \
    GLES2_HGL_FUNC(void,glDeleteTextures,(GLsizei n, const GLuint* textures)) \
    GLES2_HGL_FUNC(void,glDepthFunc,(GLenum func)) \
    GLES2_HGL_FUNC(void,glDepthMask,(GLboolean flag)) \
    GLES2_HGL_FUNC(void,glDepthRangef,(GLclampf zNear, GLclampf zFar)) \
    GLES2_HGL_FUNC(void,glDetachShader,(GLuint program, GLuint shader)) \
    GLES2_HGL_FUNC(void,glDisable,(GLenum cap)) \
    GLES2_HGL_FUNC(void,glDisableVertexAttribArray,(GLuint index)) \
    GLES2_HGL_FUNC(void,glDrawArrays,(GLenum mode, GLint first, GLsizei count)) \
    GLES2_HGL_FUNC(void,glDrawElements,(GLenum mode, GLsizei count, GLenum type, const void* indices)) \
    GLES2_HGL_FUNC(void,glEnable,(GLenum cap)) \
    GLES2_HGL_FUNC(void,glEnableVertexAttribArray,(GLuint index)) \
    GLES2_HGL_FUNC(void,glFinish,(void)) \
    GLES2_HGL_FUNC(void,glFlush,(void)) \
    GLES2_HGL_FUNC(void,glFramebufferRenderbuffer,(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)) \
    GLES2_HGL_FUNC(void,glFramebufferTexture2D,(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)) \
    GLES2_HGL_FUNC(void,glFrontFace,(GLenum mode)) \
    GLES2_HGL_FUNC(void,glGenBuffers,(GLsizei n, GLuint* buffers)) \
    GLES2_HGL_FUNC(void,glGenerateMipmap,(GLenum target)) \
    GLES2_HGL_FUNC(void,glGenFramebuffers,(GLsizei n, GLuint* framebuffers)) \
    GLES2_HGL_FUNC(void,glGenRenderbuffers,(GLsizei n, GLuint* renderbuffers)) \
    GLES2_HGL_FUNC(void,glGenTextures,(GLsizei n, GLuint* textures)) \
    GLES2_HGL_FUNC(void,glGetActiveAttrib,(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name)) \
    GLES2_HGL_FUNC(void,glGetActiveUniform,(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name)) \
    GLES2_HGL_FUNC(void,glGetAttachedShaders,(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders)) \
    GLES2_HGL_FUNC(int,glGetAttribLocation,(GLuint program, const char* name)) \
    GLES2_HGL_FUNC(void,glGetBooleanv,(GLenum pname, GLboolean* params)) \
    GLES2_HGL_FUNC(void,glGetBufferParameteriv,(GLenum target, GLenum pname, GLint* params)) \
    GLES2_HGL_FUNC(GLenum,glGetError,(void)) \
    GLES2_HGL_FUNC(void,glGetFloatv,(GLenum pname, GLfloat* params)) \
    GLES2_HGL_FUNC(void,glGetFramebufferAttachmentParameteriv,(GLenum target, GLenum attachment, GLenum pname, GLint* params)) \
    GLES2_HGL_FUNC(void,glGetIntegerv,(GLenum pname, GLint* params)) \
    GLES2_HGL_FUNC(void,glGetProgramiv,(GLuint program, GLenum pname, GLint* params)) \
    GLES2_HGL_FUNC(void,glGetProgramInfoLog,(GLuint program, GLsizei bufsize, GLsizei* length, char* infolog)) \
    GLES2_HGL_FUNC(void,glGetRenderbufferParameteriv,(GLenum target, GLenum pname, GLint* params)) \
    GLES2_HGL_FUNC(void,glGetShaderiv,(GLuint shader, GLenum pname, GLint* params)) \
    GLES2_HGL_FUNC(void,glGetShaderInfoLog,(GLuint shader, GLsizei bufsize, GLsizei* length, char* infolog)) \
    GLES2_HGL_FUNC(void,glGetShaderPrecisionFormat,(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision)) \
    GLES2_HGL_FUNC(void,glGetShaderSource,(GLuint shader, GLsizei bufsize, GLsizei* length, char* source)) \
    GLES2_HGL_FUNC(const GLubyte*,glGetString,(GLenum name)) \
    GLES2_HGL_FUNC(void,glGetTexParameterfv,(GLenum target, GLenum pname, GLfloat* params)) \
    GLES2_HGL_FUNC(void,glGetTexParameteriv,(GLenum target, GLenum pname, GLint* params)) \
    GLES2_HGL_FUNC(void,glGetUniformfv,(GLuint program, GLint location, GLfloat* params)) \
    GLES2_HGL_FUNC(void,glGetUniformiv,(GLuint program, GLint location, GLint* params)) \
    GLES2_HGL_FUNC(int,glGetUniformLocation,(GLuint program, const char* name)) \
    GLES2_HGL_FUNC(void,glGetVertexAttribfv,(GLuint index, GLenum pname, GLfloat* params)) \
    GLES2_HGL_FUNC(void,glGetVertexAttribiv,(GLuint index, GLenum pname, GLint* params)) \
    GLES2_HGL_FUNC(void,glGetVertexAttribPointerv,(GLuint index, GLenum pname, void** pointer)) \
    GLES2_HGL_FUNC(void,glHint,(GLenum target, GLenum mode)) \
    GLES2_HGL_FUNC(GLboolean,glIsBuffer,(GLuint buffer)) \
    GLES2_HGL_FUNC(GLboolean,glIsEnabled,(GLenum cap)) \
    GLES2_HGL_FUNC(GLboolean,glIsFramebuffer,(GLuint framebuffer)) \
    GLES2_HGL_FUNC(GLboolean,glIsProgram,(GLuint program)) \
    GLES2_HGL_FUNC(GLboolean,glIsRenderbuffer,(GLuint renderbuffer)) \
    GLES2_HGL_FUNC(GLboolean,glIsShader,(GLuint shader)) \
    GLES2_HGL_FUNC(GLboolean,glIsTexture,(GLuint texture)) \
    GLES2_HGL_FUNC(void,glLineWidth,(GLfloat width)) \
    GLES2_HGL_FUNC(void,glLinkProgram,(GLuint program)) \
    GLES2_HGL_FUNC(void,glPixelStorei,(GLenum pname, GLint param)) \
    GLES2_HGL_FUNC(void,glPolygonOffset,(GLfloat factor, GLfloat units)) \
    GLES2_HGL_FUNC(void,glReadPixels,(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels)) \
    GLES2_HGL_FUNC(void,glReleaseShaderCompiler,(void)) \
    GLES2_HGL_FUNC(void,glRenderbufferStorage,(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)) \
    GLES2_HGL_FUNC(void,glSampleCoverage,(GLclampf value, GLboolean invert)) \
    GLES2_HGL_FUNC(void,glScissor,(GLint x, GLint y, GLsizei width, GLsizei height)) \
    GLES2_HGL_FUNC(void,glShaderBinary,(GLsizei n, const GLuint* shaders, GLenum binaryformat, const void* binary, GLsizei length)) \
    GLES2_HGL_FUNC(void,glShaderSource,(GLuint shader, GLsizei count, const char** string, const GLint* length)) \
    GLES2_HGL_FUNC(void,glStencilFunc,(GLenum func, GLint ref, GLuint mask)) \
    GLES2_HGL_FUNC(void,glStencilFuncSeparate,(GLenum face, GLenum func, GLint ref, GLuint mask)) \
    GLES2_HGL_FUNC(void,glStencilMask,(GLuint mask)) \
    GLES2_HGL_FUNC(void,glStencilMaskSeparate,(GLenum face, GLuint mask)) \
    GLES2_HGL_FUNC(void,glStencilOp,(GLenum fail, GLenum zfail, GLenum zpass)) \
    GLES2_HGL_FUNC(void,glStencilOpSeparate,(GLenum face, GLenum fail, GLenum zfail, GLenum zpass)) \
    GLES2_HGL_FUNC(void,glTexImage2D,(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels)) \
    GLES2_HGL_FUNC(void,glTexParameterf,(GLenum target, GLenum pname, GLfloat param)) \
    GLES2_HGL_FUNC(void,glTexParameterfv,(GLenum target, GLenum pname, const GLfloat* params)) \
    GLES2_HGL_FUNC(void,glTexParameteri,(GLenum target, GLenum pname, GLint param)) \
    GLES2_HGL_FUNC(void,glTexParameteriv,(GLenum target, GLenum pname, const GLint* params)) \
    GLES2_HGL_FUNC(void,glTexSubImage2D,(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels)) \
    GLES2_HGL_FUNC(void,glUniform1f,(GLint location, GLfloat x)) \
    GLES2_HGL_FUNC(void,glUniform1fv,(GLint location, GLsizei count, const GLfloat* v)) \
    GLES2_HGL_FUNC(void,glUniform1i,(GLint location, GLint x)) \
    GLES2_HGL_FUNC(void,glUniform1iv,(GLint location, GLsizei count, const GLint* v)) \
    GLES2_HGL_FUNC(void,glUniform2f,(GLint location, GLfloat x, GLfloat y)) \
    GLES2_HGL_FUNC(void,glUniform2fv,(GLint location, GLsizei count, const GLfloat* v)) \
    GLES2_HGL_FUNC(void,glUniform2i,(GLint location, GLint x, GLint y)) \
    GLES2_HGL_FUNC(void,glUniform2iv,(GLint location, GLsizei count, const GLint* v)) \
    GLES2_HGL_FUNC(void,glUniform3f,(GLint location, GLfloat x, GLfloat y, GLfloat z)) \
    GLES2_HGL_FUNC(void,glUniform3fv,(GLint location, GLsizei count, const GLfloat* v)) \
    GLES2_HGL_FUNC(void,glUniform3i,(GLint location, GLint x, GLint y, GLint z)) \
    GLES2_HGL_FUNC(void,glUniform3iv,(GLint location, GLsizei count, const GLint* v)) \
    GLES2_HGL_FUNC(void,glUniform4f,(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w)) \
    GLES2_HGL_FUNC(void,glUniform4fv,(GLint location, GLsizei count, const GLfloat* v)) \
    GLES2_HGL_FUNC(void,glUniform4i,(GLint location, GLint x, GLint y, GLint z, GLint w)) \
    GLES2_HGL_FUNC(void,glUniform4iv,(GLint location, GLsizei count, const GLint* v)) \
    GLES2_HGL_FUNC(void,glUniformMatrix2fv,(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)) \
    GLES2_HGL_FUNC(void,glUniformMatrix3fv,(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)) \
    GLES2_HGL_FUNC(void,glUniformMatrix4fv,(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)) \
    GLES2_HGL_FUNC(void,glUseProgram,(GLuint program)) \
    GLES2_HGL_FUNC(void,glValidateProgram,(GLuint program)) \
    GLES2_HGL_FUNC(void,glVertexAttrib1f,(GLuint indx, GLfloat x)) \
    GLES2_HGL_FUNC(void,glVertexAttrib1fv,(GLuint indx, const GLfloat* values)) \
    GLES2_HGL_FUNC(void,glVertexAttrib2f,(GLuint indx, GLfloat x, GLfloat y)) \
    GLES2_HGL_FUNC(void,glVertexAttrib2fv,(GLuint indx, const GLfloat* values)) \
    GLES2_HGL_FUNC(void,glVertexAttrib3f,(GLuint indx, GLfloat x, GLfloat y, GLfloat z)) \
    GLES2_HGL_FUNC(void,glVertexAttrib3fv,(GLuint indx, const GLfloat* values)) \
    GLES2_HGL_FUNC(void,glVertexAttrib4f,(GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w)) \
    GLES2_HGL_FUNC(void,glVertexAttrib4fv,(GLuint indx, const GLfloat* values)) \
    GLES2_HGL_FUNC(void,glVertexAttribPointer,(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* ptr)) \
    GLES2_HGL_FUNC(void,glViewport,(GLint x, GLint y, GLsizei width, GLsizei height))

    typedef struct HGL {
    #define GLES2_HGL_FUNC(ret,name,attr) ret GL_APIENTRY (*name)attr;
    GLES2_HGL_FUNCS
    #undef GLES2_HGL_FUNC
    } HGL;
