#include "ContextGL.h"

#include "HeadersGL.h"

#include "rbx/Debug.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <gl/wglew.h>

GLenum GLEWAPIENTRY wglewContextInit();

FASTFLAG(DebugGraphicsGL)

namespace RBX
{
namespace Graphics
{
class ContextGLWin32: public ContextGL
{
public:
    ContextGLWin32(void* windowHandle)
        : hwnd(reinterpret_cast<HWND>(windowHandle))
        , hdc(GetDC(hwnd))
        , hglrc(NULL)
    {
        PIXELFORMATDESCRIPTOR pfd = {};
        pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;
		pfd.cAlphaBits = 8;
        pfd.cDepthBits = 24;
        pfd.cStencilBits = 8;
        pfd.iLayerType = PFD_MAIN_PLANE;

        int pf = ChoosePixelFormat(hdc, &pfd);

        if (!pf)
			throw RBX::runtime_error("Error choosing pixel format: %x", GetLastError());

        if (!SetPixelFormat(hdc, pf, &pfd))
			throw RBX::runtime_error("Error setting pixel format: %x", GetLastError());

        hglrc = wglCreateContext(hdc);

        if (!hglrc)
			throw RBX::runtime_error("Error creating context: %x", GetLastError());

        if (!wglMakeCurrent(hdc, hglrc))
			throw RBX::runtime_error("Error changing context: %x", GetLastError());

		if (FFlag::DebugGraphicsGL)
        {
            // We need to initialize the context with wgl function, but you can only get the wgl function after you initialize the context.
            // Let's just reinitialize it.
            wglewContextInit();

			if (WGLEW_ARB_create_context)
			{
                wglMakeCurrent(NULL, NULL);
                wglDeleteContext(hglrc);

                const int attribs[4] = { WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB, 0, 0 };

                hglrc = wglCreateContextAttribsARB(hdc, NULL, attribs);

                if (!hglrc)
                    throw RBX::runtime_error("Error creating context: %x", GetLastError());

                if (!wglMakeCurrent(hdc, hglrc))
                    throw RBX::runtime_error("Error changing context: %x", GetLastError());
			}
        }

        glewInitRBX();

		if (wglSwapIntervalEXT)
			wglSwapIntervalEXT(0);
    }

    ~ContextGLWin32()
    {
        if (wglGetCurrentContext() == hglrc)
            wglMakeCurrent(NULL, NULL);

        wglDeleteContext(hglrc);
    }

    virtual void setCurrent()
    {
        if (wglGetCurrentContext() != hglrc)
        {
            BOOL result = wglMakeCurrent(hdc, hglrc);
            RBXASSERT(result);
        }
    }

    virtual void swapBuffers()
    {
        RBXASSERT(wglGetCurrentContext() == hglrc);

        SwapBuffers(hdc);
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
        RECT rect = {};
        GetClientRect(hwnd, &rect);

        return std::make_pair(rect.right - rect.left, rect.bottom - rect.top);
    }

private:
    HWND hwnd;
    HDC hdc;
    HGLRC hglrc;
};

ContextGL* ContextGL::create(void* windowHandle)
{
    return new ContextGLWin32(windowHandle);
}

}
}
