#pragma once

#define PIX_ENABLED 1

#if PIX_ENABLED


#define PIX_CONCAT(a,b) PIX_CONCAT2(a,b)
#define PIX_CONCAT2(a,b) PIX_CONCAT3(a,b)
#define PIX_CONCAT3(a,b) a##b

#define PIX_MARKER(ctx,...)   RBX::Graphics::PixMarker(ctx,__VA_ARGS__)
#define PIX_SCOPE(ctx,...)    RBX::Graphics::PixScope PIX_CONCAT(pixScopeVar,__LINE__)(ctx,__VA_ARGS__)

// PIX support, D3D-only
// pix.cpp is in GfxCore/D3D9

namespace RBX { namespace Graphics {

class DeviceContext;
void PixMarker(DeviceContext* ctx, const char* fmt, ...);
struct PixScope
{
    PixScope(DeviceContext* ctx, const char* fmt, ...);
    ~PixScope();
    DeviceContext* devctx;
};

}}

#else

#define PIX_MARKER(...)
#define PIX_SCOPE(...)

#endif
