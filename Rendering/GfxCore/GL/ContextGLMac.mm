#include "ContextGL.h"

#include "HeadersGL.h"

#include "rbx/Debug.h"

#include <AppKit/AppKit.h>
#include <OpenGL/OpenGL.h>

LOGGROUP(Graphics)

FASTFLAGVARIABLE(UpdateContextOnFollowingFrame, false)

FASTFLAG(GraphicsGL3)

namespace RBX
{
namespace Graphics
{

static NSOpenGLPixelFormat* createPixelFormatWithApi(unsigned int api)
{
    NSOpenGLPixelFormatAttribute attribs[32];
    int ai = 0;
    
    // Specify the display ID to associate the GL context with main display. Useful if there is ambiguity
    attribs[ai++] = NSOpenGLPFAScreenMask;
    attribs[ai++] = CGDisplayIDToOpenGLDisplayMask(CGMainDisplayID());
    
    // Specifying "NoRecovery" gives us a context that cannot fall back to the software renderer.
    attribs[ai++] = NSOpenGLPFANoRecovery;
    
    attribs[ai++] = NSOpenGLPFAAccelerated;
    attribs[ai++] = NSOpenGLPFADoubleBuffer;
    
    attribs[ai++] = NSOpenGLPFAColorSize;
    attribs[ai++] = 24;
    
    attribs[ai++] = NSOpenGLPFAAlphaSize;
    attribs[ai++] = 8;
    
    attribs[ai++] = NSOpenGLPFAStencilSize;
    attribs[ai++] = 8;
    
    attribs[ai++] = NSOpenGLPFADepthSize;
    attribs[ai++] = 16;
    
    if (api)
    {
        attribs[ai++] = NSOpenGLPFAOpenGLProfile;
        attribs[ai++] = api;
    }
    
    attribs[ai++] = 0;
    
    return [[NSOpenGLPixelFormat alloc] initWithAttributes: attribs];
}

class ContextGLNSGL: public ContextGL
{
public:
    ContextGLNSGL(void* windowHandle)
    : view(0)
    , pixelFormat(0)
    , context(0)
    , updateRequired(false)
    {
        view = [(NSView*)windowHandle retain];
        RBXASSERT(view);
        
        if (FFlag::GraphicsGL3)
        {
            // OSX 10.7 graphics drivers crash on compiling some (valid) shaders
            if (NSAppKitVersionNumber >= NSAppKitVersionNumber10_8)
                pixelFormat = createPixelFormatWithApi(NSOpenGLProfileVersion3_2Core);

            if (!pixelFormat)
                pixelFormat = createPixelFormatWithApi(0);

            if (!pixelFormat)
                throw std::runtime_error("Error creating pixel format");
        }
        else
        {
            pixelFormat = createPixelFormatWithApi(0);
            if (!pixelFormat)
                throw std::runtime_error("Error creating pixel format");
        }
        
        context = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];
        if (!context)
            throw std::runtime_error("Error creating context");
        
        GLint swapInterval = 0;
        [context setValues:&swapInterval forParameter:NSOpenGLCPSwapInterval];
        
        [context setView:view];
        [context makeCurrentContext];
        
        if (CGLError err = CGLEnable(static_cast<CGLContextObj>([context CGLContextObj]), kCGLCEMPEngine))
            FASTLOG1(FLog::Graphics, "Error enabling MP engine: %x", err);
        
        glewInitRBX();
    }

    ~ContextGLNSGL()
    {
        if ([NSOpenGLContext currentContext] == context)
            [NSOpenGLContext clearCurrentContext];
        
        [context clearDrawable];
        
        [context release];
        [pixelFormat release];
        [view release];
    }

    virtual void setCurrent()
    {
        if ([NSOpenGLContext currentContext] != context)
            [context makeCurrentContext];
    }

    virtual void swapBuffers()
    {
        [context flushBuffer];
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
        NSRect b = [view bounds];
        
        if (FFlag::UpdateContextOnFollowingFrame && updateRequired)
        {
            [context update];
            updateRequired = false;
        }

        if (width != b.size.width || height != b.size.height)
        {
            if (!FFlag::UpdateContextOnFollowingFrame)
                [context update];
            updateRequired = true;
        }
        
        return std::make_pair(b.size.width, b.size.height);
    }

private:
    NSView* view;
    NSOpenGLPixelFormat* pixelFormat;
    NSOpenGLContext* context;
    
    bool updateRequired;
 };

ContextGL* ContextGL::create(void* windowHandle)
{
    return new ContextGLNSGL(windowHandle);
}

}
}
