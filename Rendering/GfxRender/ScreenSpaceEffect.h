#pragma once

#include "GlobalShaderData.h"
#include "GfxCore/Resource.h"
#include "rbx/Boost.hpp"

#include "v8datamodel/Lighting.h"

#include "TextureRef.h"

namespace RBX
{
namespace Graphics
{
    class VisualEngine;
    class DeviceContext;
    class Framebuffer;
    class BlendState;
    class RasterizerState;
    class ShaderProgram;

    class ScreenSpaceEffect
    {
    public:
        static ShaderProgram* renderFullscreenBegin(DeviceContext* context, VisualEngine* visualEngine, const char* vsName, const char* fsName, const BlendState& blendState, unsigned fbWidth, unsigned fbHeight);
        static void renderFullscreenEnd(DeviceContext* context, VisualEngine* visualEngine);

        static void renderBlur(DeviceContext* context, VisualEngine* visualEngine, Framebuffer* fb, Framebuffer* intermediateFB, Texture* texture, Texture* intermediate, float strength);
    };

}
}
