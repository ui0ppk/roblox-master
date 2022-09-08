#include "ContextGL.h"

#include "HeadersGL.h"

#include "rbx/Debug.h"

#include <android/native_window.h>
#include <EGL/egl.h>

LOGGROUP(Graphics)

// GL extensions
static GLvoid (GL_APIENTRY * glBindVertexArrayPtr) (GLuint array);
static GLvoid (GL_APIENTRY * glDeleteVertexArraysPtr) (GLsizei n, const GLuint* arrays);
static GLvoid (GL_APIENTRY * glGenVertexArraysPtr) (GLsizei n, const GLuint* arrays);
static GLvoid * (GL_APIENTRY * glMapBufferPtr) (GLenum target, GLenum access);
static GLboolean (GL_APIENTRY * glUnmapBufferPtr) (GLenum target);
static GLvoid * (GL_APIENTRY * glMapBufferRangePtr) (GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
static GLvoid (GL_APIENTRY * glTexImage3DPtr) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
static GLvoid (GL_APIENTRY * glTexSubImage3DPtr) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels);
static GLvoid (GL_APIENTRY * glTexStorage2DPtr) (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
static GLvoid (GL_APIENTRY * glTexStorage3DPtr) (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
static GLsync (GL_APIENTRY * glFenceSyncPtr) (GLenum condition, GLbitfield flags);
static void (GL_APIENTRY * glDeleteSyncPtr) (GLsync sync);
static GLenum (GL_APIENTRY * glClientWaitSyncPtr) (GLsync sync, GLbitfield flags, GLuint64 timeout);
static void (GL_APIENTRY * glWaitSyncPtr) (GLsync sync, GLbitfield flags, GLuint64 timeout);
static GLvoid (GL_APIENTRY * glBlitFramebufferPtr) (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
static GLvoid (GL_APIENTRY * glRenderbufferStorageMultisamplePtr) (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
static GLvoid (GL_APIENTRY * glInvalidateFramebufferPtr) (GLenum target, GLsizei numAttachments, const GLenum* attachments);

template <typename T> static void loadExtensionGL(T& ptr, const char* namecore, const char* nameext)
{
	if (!ptr)
		ptr = reinterpret_cast<T>(eglGetProcAddress(namecore));

	if (!ptr)
		ptr = reinterpret_cast<T>(eglGetProcAddress(nameext));
}

#define LOAD_EXTENSION_GL(name, suffix) loadExtensionGL(name ## Ptr, #name, #name #suffix)

// GL stubs
GLvoid glBindVertexArray(GLuint array)
{
    glBindVertexArrayPtr(array);
}

GLvoid glDeleteVertexArrays(GLsizei n, const GLuint *arrays)
{
    glDeleteVertexArraysPtr(n, arrays);
}

GLvoid glGenVertexArrays(GLsizei n, GLuint *arrays)
{
    glGenVertexArraysPtr(n, arrays);
}

GLvoid *glMapBuffer(GLenum target, GLenum access)
{
    return glMapBufferPtr(target, access);
}

GLboolean glUnmapBuffer(GLenum target)
{
    return glUnmapBufferPtr(target);
}

GLvoid *glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access)
{
    return glMapBufferRangePtr(target, offset, length, access);
}

GLvoid glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
{
    glTexStorage2DPtr(target, levels, internalformat, width, height);
}

GLvoid glTexImage3D(GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
    glTexImage3DPtr(target, level, internalFormat, width, height, depth, border, format, type, pixels);
}

GLvoid glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels)
{
    glTexSubImage3DPtr(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}

GLvoid glTexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth)
{
    glTexStorage3DPtr(target, levels, internalformat, width, height, depth);
}

GLsync glFenceSync(GLenum condition, GLbitfield flags)
{
	return glFenceSyncPtr(condition, flags);
}

void glDeleteSync(GLsync sync)
{
	glDeleteSyncPtr(sync);
}

GLenum glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout)
{
	return glClientWaitSyncPtr(sync, flags, timeout);
}

void glWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout)
{
	glWaitSyncPtr(sync, flags, timeout);
}

GLvoid glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)
{
	glBlitFramebufferPtr(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}

GLvoid glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height)
{
	glRenderbufferStorageMultisamplePtr(target, samples, internalformat, width, height);
}

GLvoid glInvalidateFramebuffer(GLenum target, GLsizei numAttachments, const GLenum* attachments)
{
	glInvalidateFramebufferPtr(target, numAttachments, attachments);
}

namespace RBX
{
namespace Graphics
{

class ContextGLAndroid: public ContextGL
{
public:
    ContextGLAndroid(void* windowHandle)
    {
        aNativeWindow = static_cast<ANativeWindow*>(windowHandle);

        ANativeWindow_acquire(aNativeWindow);

        FASTLOG2(FLog::Graphics, "Window size: %dx%d", ANativeWindow_getWidth(aNativeWindow), ANativeWindow_getHeight(aNativeWindow));

        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (display == EGL_NO_DISPLAY)
            throw runtime_error("Error creating context: eglGetDisplay %x", eglGetError());
        
        if (!eglInitialize(display, 0, 0))
            throw runtime_error("Error creating context: eglInitialize %x", eglGetError());

        EGLConfig config;

        if (
            !tryChooseConfig(display, &config, 8, 8, 8, 24, 0) &&
            !tryChooseConfig(display, &config, 8, 8, 8, 16, 0) &&
            !tryChooseConfig(display, &config, 5, 6, 5, 16, 0) &&
            !tryChooseConfig(display, &config, 8, 8, 8, 24, 1) &&
            !tryChooseConfig(display, &config, 8, 8, 8, 16, 1) &&
            !tryChooseConfig(display, &config, 5, 6, 5, 16, 1))
            throw runtime_error("Error creating context: could not find suitable config (%x)", eglGetError());

        surface = eglCreateWindowSurface(display, config, aNativeWindow, 0);

        if (!surface)
        {
            throw runtime_error("Error creating context: eglCreateWindowSurface %x", eglGetError());
        }

        static const EGLint contextAttrs[] =
        {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
        };

        context = eglCreateContext(display, config, NULL, contextAttrs);

        if (!context)
            throw runtime_error("Error creating context: eglCreateContext %x", eglGetError());

        if (!eglMakeCurrent(display, surface, surface, context))
            throw runtime_error("Error creating context: eglMakeCurrent %x", eglGetError());

        eglQuerySurface(display, surface, EGL_WIDTH, &surfaceWidth);
        eglQuerySurface(display, surface, EGL_HEIGHT, &surfaceHeight);

        FASTLOG4(FLog::Graphics, "Initialized EGL context %p (surface %p) with renderbuffer %dx%d", context, surface, surfaceWidth, surfaceHeight);

        EGLint minSwapInterval = -1;
        eglGetConfigAttrib( display, config, EGL_MIN_SWAP_INTERVAL, &minSwapInterval );
        FASTLOG1(FLog::Graphics, "EGL_MIN_SWAP_INTERVAL: %d", minSwapInterval );

        if( eglSwapInterval( display, 0 ) == EGL_FALSE  )
        {
        	// This should be impossible as the interval is silently clamped to EGL_MIN_SWAP_INTERVAL.
        	FASTLOG(FLog::Graphics, "*** eglSwapInterval EGL_FALSE" );
        }

        LOAD_EXTENSION_GL(glBindVertexArray, OES);
        LOAD_EXTENSION_GL(glDeleteVertexArrays, OES);
        LOAD_EXTENSION_GL(glGenVertexArrays, OES);
        LOAD_EXTENSION_GL(glMapBuffer, OES);
        LOAD_EXTENSION_GL(glUnmapBuffer, OES);
        LOAD_EXTENSION_GL(glMapBufferRange, EXT);
        LOAD_EXTENSION_GL(glTexImage3D, OES);
        LOAD_EXTENSION_GL(glTexSubImage3D, OES);
        LOAD_EXTENSION_GL(glTexStorage2D, EXT);
        LOAD_EXTENSION_GL(glTexStorage3D, EXT);
        LOAD_EXTENSION_GL(glFenceSync, EXT);
        LOAD_EXTENSION_GL(glDeleteSync, EXT);
        LOAD_EXTENSION_GL(glClientWaitSync, EXT);
        LOAD_EXTENSION_GL(glWaitSync, EXT);
        LOAD_EXTENSION_GL(glBlitFramebuffer, EXT);
        LOAD_EXTENSION_GL(glRenderbufferStorageMultisample, EXT);
        LOAD_EXTENSION_GL(glInvalidateFramebuffer, EXT);
    }

    ~ContextGLAndroid()
    {
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        eglDestroyContext(display, context);
        eglDestroySurface(display, surface);

        eglTerminate(display);

        ANativeWindow_release(aNativeWindow);
    }

    virtual void setCurrent()
    {
        bool result = eglMakeCurrent(display, surface, surface, context);
        RBXASSERT(result);
    }

    virtual void swapBuffers()
    {
        bool result = eglSwapBuffers(display, surface);
        RBXASSERT(result);
    }

    virtual unsigned int getMainFramebufferId()
    {
        return 0;
    }

    virtual bool isMainFramebufferRetina()
    {
        return false;
    }

    virtual std::pair<unsigned int, unsigned int> updateMainFramebuffer(unsigned int width, unsigned int height)
    {
        return std::make_pair(surfaceWidth, surfaceHeight);
    }

private:
    ANativeWindow* aNativeWindow;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    EGLint surfaceWidth;
    EGLint surfaceHeight;

    static bool tryChooseConfig(EGLDisplay display, EGLConfig* config, int redBits, int greenBits, int blueBits, int depthBits, int swapInterval )
    {
        const EGLint attribs[] =
        {
        	EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        	EGL_MIN_SWAP_INTERVAL, swapInterval,
            EGL_RED_SIZE, redBits,
            EGL_GREEN_SIZE, greenBits,
            EGL_BLUE_SIZE, blueBits,
            EGL_DEPTH_SIZE, depthBits,
            EGL_NONE
        };

        FASTLOG4(FLog::Graphics, "Trying to choose EGL config r%d g%d b%d d%d", redBits, greenBits, blueBits, depthBits);

        EGLint numConfigs = 0;
        return eglChooseConfig(display, attribs, config, 1, &numConfigs) && numConfigs > 0;
    }
};

ContextGL* ContextGL::create(void* windowHandle)
{
    return new ContextGLAndroid(windowHandle);
}

}
}
