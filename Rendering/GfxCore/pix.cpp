#include "GfxCore/pix.h"

#define _CRT_SECURE_NO_WARNINGS 1

#if PIX_ENABLED

#include "GfxCore/Device.h"
#include <stdio.h>
#include <stdarg.h>
#include "FastLog.h"

FASTFLAG(GraphicsDebugMarkersEnable)

namespace RBX { namespace Graphics {


void PixMarker(DeviceContext* ctx, const char* fmt, ...)
{
    if (FFlag::GraphicsDebugMarkersEnable)
    {
        va_list va;
        va_start(va, fmt);

        char buffer[512];
        vsnprintf(buffer, 512, fmt, va);
        buffer[511] = 0;
        ctx->setDebugMarker(buffer);

        va_end(va);
    }
}

PixScope::PixScope(DeviceContext* ctx, const char* fmt, ...)
{
    devctx = ctx;
    if (FFlag::GraphicsDebugMarkersEnable)
    {
        va_list va;
        va_start(va, fmt);

        char buffer[512];
        vsnprintf(buffer, 512, fmt, va);
        buffer[511] = 0;
        ctx->pushDebugMarkerGroup(buffer);
        
        va_end(va);
    }
}
    
PixScope::~PixScope()
{
    if (FFlag::GraphicsDebugMarkersEnable)
    {
        devctx->popDebugMarkerGroup();
    }
}

}}

#else
static int dummy0001;
#endif

