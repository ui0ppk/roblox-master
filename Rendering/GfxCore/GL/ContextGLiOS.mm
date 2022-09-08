#ifdef RBX_PLATFORM_IOS
#include "ContextGL.h"

#include "HeadersGL.h"

#include "rbx/Debug.h"

#include <UIKit/UIKit.h>

LOGGROUP(Graphics)

FASTFLAG(GraphicsGL3)

// GL imports
namespace GLES3
{
    extern "C" GLvoid glTexImage3D(GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
    extern "C" GLvoid glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels);
    extern "C" GLvoid glTexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
    extern "C" GLvoid glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
    extern "C" GLvoid glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
    extern "C" GLvoid glInvalidateFramebuffer(GLenum target, GLsizei numAttachments, const GLenum* attachments);
}

// GL stubs
GLvoid glBindVertexArray(GLuint array)
{
    glBindVertexArrayOES(array);
}

GLvoid glDeleteVertexArrays(GLsizei n, const GLuint *arrays)
{
    glDeleteVertexArraysOES(n, arrays);
}

GLvoid glGenVertexArrays(GLsizei n, GLuint *arrays)
{
    glGenVertexArraysOES(n, arrays);
}

GLvoid *glMapBuffer(GLenum target, GLenum access)
{
    return glMapBufferOES(target, access);
}

GLboolean glUnmapBuffer(GLenum target)
{
    return glUnmapBufferOES(target);
}

GLvoid *glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access)
{
    return glMapBufferRangeEXT(target, offset, length, access);
}

GLvoid glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
{
    glTexStorage2DEXT(target, levels, internalformat, width, height);
}

GLvoid glTexImage3D(GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
    GLES3::glTexImage3D(target, level, internalFormat, width, height, depth, border, format, type, pixels);
}

GLvoid glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels)
{
    GLES3::glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}

GLvoid glTexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth)
{
    GLES3::glTexStorage3D(target, levels, internalformat, width, height, depth);
}

GLsync glFenceSync(GLenum condition, GLbitfield flags)
{
    return glFenceSyncAPPLE(condition, flags);
}

void glDeleteSync(GLsync sync)
{
    glDeleteSyncAPPLE(sync);
}

GLenum glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout)
{
    return glClientWaitSyncAPPLE(sync, flags, timeout);
}

void glWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout)
{
    glWaitSyncAPPLE(sync, flags, timeout);
}

GLvoid glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)
{
	GLES3::glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}

GLvoid glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height)
{
	GLES3::glRenderbufferStorageMultisample(target, samples, internalformat, width, height);
}

GLvoid glInvalidateFramebuffer(GLenum target, GLsizei numAttachments, const GLenum* attachments)
{
	GLES3::glInvalidateFramebuffer(target, numAttachments, attachments);
}

namespace RBX
{
namespace Graphics
{

class ContextGLEAGL: public ContextGL
{
public:
    ContextGLEAGL(void* windowHandle)
    {
        view = [(UIView*)windowHandle retain];
        RBXASSERT(view);
        RBXASSERT([view isKindOfClass:[UIView class]]);
      
        layer = [(CAEAGLLayer*)view.layer retain];
        RBXASSERT(layer);
        RBXASSERT([layer isKindOfClass:[CAEAGLLayer class]]);
        
        // Create context
		if (FFlag::GraphicsGL3)
		{
			context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];

			if (!context)
				context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];

			if (!context)
				throw std::runtime_error("Error creating context");
		}
		else
		{
			context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
			if (!context)
				throw std::runtime_error("Error creating context");
		}
        
        // Enable Retina if available
        if (isMainFramebufferRetina())
            view.contentScaleFactor = 2.0;
        
        // Perform basic layer configuration
        [layer setOpaque:YES];
        [layer setDrawableProperties: [NSDictionary dictionaryWithObjectsAndKeys:
                                      [NSNumber numberWithBool:NO], kEAGLDrawablePropertyRetainedBacking,
                                      kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil]];
        
        if (![EAGLContext setCurrentContext:context])
            throw std::runtime_error("Error changing context");
        
        // Create main color/depth buffer
        glGenRenderbuffers(1, &colorRenderbufferId);
        glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbufferId);
        
        if (![context renderbufferStorage:GL_RENDERBUFFER fromDrawable:layer])
            throw std::runtime_error("Error allocating main renderbuffer");
        
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &renderbufferWidth);
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &renderbufferHeight);
        
        FASTLOG4(FLog::Graphics, "Initialized EAGLContext %p (view %p) with renderbuffer %dx%d", context, view, renderbufferWidth, renderbufferHeight);
        
        glGenRenderbuffers(1, &depthRenderbufferId);
        glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbufferId);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8_OES, renderbufferWidth, renderbufferHeight);
        
        // Create main framebuffer
        glGenFramebuffers(1, &framebufferId);
        glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRenderbufferId);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbufferId);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthRenderbufferId);
        
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        
        if (status != GL_FRAMEBUFFER_COMPLETE)
            throw RBX::runtime_error("Unsupported main framebuffer configuration: error %x", status);
    }

    ~ContextGLEAGL()
    {
        setCurrent();
        
        glDeleteFramebuffers(1, &framebufferId);
        glDeleteRenderbuffers(1, &colorRenderbufferId);
        glDeleteRenderbuffers(1, &depthRenderbufferId);
        
        [EAGLContext setCurrentContext:nil];
        
        [context release];
        [layer release];
        [view release];
    }

    virtual void setCurrent()
    {
        if ([EAGLContext currentContext] != context)
        {
            bool result = [EAGLContext setCurrentContext:context];
            RBXASSERT(result);
        }
    }

    virtual void swapBuffers()
    {
        glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbufferId);
        [context presentRenderbuffer:GL_RENDERBUFFER];
    }

    virtual unsigned int getMainFramebufferId()
    {
        return framebufferId;
    }

    virtual bool isMainFramebufferRetina()
    {
        // We can render with Retina on all devices that support Retina with iOS 6+ and GLES3
        // iOS 5.1 has a non-conforming OpenGL ES implementation that does not allow us to render into NPOT targets
        return
            [[UIScreen mainScreen] scale] >= 2.0 &&
            [[[UIDevice currentDevice] systemVersion] compare:@"6.0" options:NSNumericSearch] != NSOrderedAscending &&
            context.API >= kEAGLRenderingAPIOpenGLES3;
    }

    virtual std::pair<unsigned int, unsigned int> updateMainFramebuffer(unsigned int width, unsigned int height)
    {
        return std::make_pair(renderbufferWidth, renderbufferHeight);
    }

private:
    UIView* view;
    CAEAGLLayer* layer;
    EAGLContext* context;
    
    unsigned int colorRenderbufferId;
    unsigned int depthRenderbufferId;
    unsigned int framebufferId;
    
    int renderbufferWidth;
    int renderbufferHeight;
};

ContextGL* ContextGL::create(void* windowHandle)
{
    return new ContextGLEAGL(windowHandle);
}

}
}
#endif
