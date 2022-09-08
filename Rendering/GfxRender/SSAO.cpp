#include "stdafx.h"
#include "SSAO.h"

#include "VisualEngine.h"
#include "ShaderManager.h"
#include "SceneManager.h"
#include "ScreenSpaceEffect.h"
#include "Util.h"

#include "GfxCore/Device.h"
#include "GfxCore/Texture.h"
#include "GfxCore/Framebuffer.h"
#include "GfxCore/States.h"
#include "GfxCore/pix.h"

#include "rbx/Profiler.h"

LOGGROUP(Graphics)

namespace RBX
{
namespace Graphics
{

struct SSAO::Data
{   
    shared_ptr<Texture> ssao[2];
    shared_ptr<Framebuffer> ssaoFB[2];

    shared_ptr<Texture> depthHalf;
    shared_ptr<Framebuffer> depthHalfFB;

    unsigned width, height;
};

SSAO::SSAO(VisualEngine* visualEngine)
	: Resource(visualEngine->getDevice())
	, visualEngine(visualEngine)
	, forceDisabled(false)
    , currentLevel(ssaoNone)
{
    // Create noise texture (ref: Toy Story SSAO)
    static const int rotationIndex[4][4] =
    {
		{ 4, 1, 3, 7 }, { 8, 11, 15, 6 }, { 10, 2, 14, 5, }, { 0, 13, 12, 9 }
    };

    unsigned int noiseData[4][4];

    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x)
        {
            float rsin = ::sin(rotationIndex[y][x]*G3D::pif() / 16) * 0.5f + 0.5f;
            float rcos = ::cos(rotationIndex[y][x]*G3D::pif() / 16) * 0.5f + 0.5f;

			noiseData[y][x] = packColor(Color4(rsin, rcos, 0, 1), visualEngine->getDevice()->getCaps().colorOrderBGR);
        }

    noise = visualEngine->getDevice()->createTexture(Texture::Type_2D, Texture::Format_RGBA8, 4, 4, 1, 1, Texture::Usage_Static);
    RBXASSERT(noise);

	noise->upload(0, 0, TextureRegion(0, 0, 4, 4), noiseData, sizeof(noiseData));
}

SSAO::~SSAO()
{
}

void SSAO::update(SSAOLevel level, unsigned int width, unsigned int height)
{
    SceneManager::GBuffer* gb = visualEngine->getSceneManager()->getGBuffer();
	if (forceDisabled || level == ssaoNone || !gb)
	{
		data.reset();
	}
	else
	{
		if (!data.get() || data->width != width || data->height != height)
		{
            try
            {
                data.reset(createData(width, height));
			}
            catch (const RBX::base_exception& e)
            {
                FASTLOGS(FLog::Graphics, "SSAO: Error creating targets: %s", e.what());

				forceDisabled = true;
                data.reset();
            }
		}
	}
	currentLevel = level;
}

void SSAO::renderCompute(DeviceContext* context)
{
	RBXASSERT(currentLevel != ssaoNone);

    SceneManager::GBuffer* gb = visualEngine->getSceneManager()->getGBuffer();
    RBXASSERT(gb);

    PIX_SCOPE(context, "SSAO/renderCompute");
    RBXPROFILER_SCOPE("Render", "SSAO::renderCompute");
	RBXPROFILER_SCOPE("GPU", "SSAO::renderCompute");

    // Downsample depth
    context->bindFramebuffer(data->depthHalfFB.get());
    
    if (ScreenSpaceEffect::renderFullscreenBegin(context, visualEngine, "SSAODepthDownVS", "SSAODepthDownFS", BlendState::Mode_None, data->depthHalfFB->getWidth(), data->depthHalfFB->getHeight()))
    {
        context->bindTexture(0, gb->gbufferDepth.get(), SamplerState(SamplerState::Filter_Point, SamplerState::Address_Clamp));	
        ScreenSpaceEffect::renderFullscreenEnd(context, visualEngine);
    }

    // Compute SSAO buffer
    context->bindFramebuffer(data->ssaoFB[0].get());

    if (ScreenSpaceEffect::renderFullscreenBegin(context, visualEngine, "PassThroughVS", "SSAOFS", BlendState::Mode_None, data->ssaoFB[0]->getWidth(), data->ssaoFB[0]->getHeight()))
    {
        context->bindTexture(0, data->depthHalf.get(), SamplerState(SamplerState::Filter_Point, SamplerState::Address_Clamp));
        context->bindTexture(1, noise.get(), SamplerState::Filter_Point);

        ScreenSpaceEffect::renderFullscreenEnd(context, visualEngine);
    }

    // Blur SSAO horizontally
    context->bindFramebuffer(data->ssaoFB[1].get());

    if (ScreenSpaceEffect::renderFullscreenBegin(context, visualEngine, "SSAOBlurXVS", "SSAOBlurXFS", BlendState::Mode_None, data->ssaoFB[1]->getWidth(), data->ssaoFB[1]->getHeight()))
    {
        context->bindTexture(0, data->depthHalf.get(), SamplerState(SamplerState::Filter_Point, SamplerState::Address_Clamp));
        context->bindTexture(2, data->ssao[0].get(), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));

        ScreenSpaceEffect::renderFullscreenEnd(context, visualEngine);
    }

    // Blur SSAO vertically
    context->bindFramebuffer(data->ssaoFB[0].get());

    if (ScreenSpaceEffect::renderFullscreenBegin(context, visualEngine, "SSAOBlurYVS", "SSAOBlurYFS", BlendState::Mode_None, data->ssaoFB[0]->getWidth(), data->ssaoFB[0]->getHeight()))
    {
        context->bindTexture(0, data->depthHalf.get(), SamplerState(SamplerState::Filter_Point, SamplerState::Address_Clamp));
        context->bindTexture(2, data->ssao[1].get(), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));
        context->bindTexture(3, gb->gbufferDepth.get(), SamplerState(SamplerState::Filter_Point, SamplerState::Address_Clamp));

        ScreenSpaceEffect::renderFullscreenEnd(context, visualEngine);
    }
}

void SSAO::renderApply(DeviceContext* context)
{
    RBXASSERT(currentLevel != ssaoNone);
    SceneManager::GBuffer* gb = visualEngine->getSceneManager()->getGBuffer();
    RBXASSERT(gb);
	
    PIX_SCOPE(context, "SSAO/renderApply");
    RBXPROFILER_SCOPE("Render", "SSAO::renderApply");
	RBXPROFILER_SCOPE("GPU", "SSAO::renderApply");

    // Composite SSAO
    if (currentLevel == ssaoFullBlank)
    {
        if (ScreenSpaceEffect::renderFullscreenBegin(context, visualEngine, "PassThroughVS", "PassThroughFS", BlendState::Mode_None, data->width, data->height))
        {
            context->bindTexture(0, gb->gbufferColor.get(), SamplerState(SamplerState::Filter_Point, SamplerState::Address_Clamp));
            ScreenSpaceEffect::renderFullscreenEnd(context, visualEngine);
        }
    }
    else
    {
        if (ScreenSpaceEffect::renderFullscreenBegin(context, visualEngine, "SSAOCompositVS", "SSAOCompositFS", BlendState::Mode_None, data->width, data->height))
        {
            context->bindTexture(0, data->depthHalf.get(), SamplerState(SamplerState::Filter_Point, SamplerState::Address_Clamp));
            context->bindTexture(2, data->ssao[0].get(), SamplerState(SamplerState::Filter_Point, SamplerState::Address_Clamp));
            context->bindTexture(3, gb->gbufferDepth.get(), SamplerState(SamplerState::Filter_Point, SamplerState::Address_Clamp));
            context->bindTexture(4, gb->gbufferColor.get(), SamplerState(SamplerState::Filter_Point, SamplerState::Address_Clamp));

            ScreenSpaceEffect::renderFullscreenEnd(context, visualEngine);
        }
    }
}

void SSAO::renderComposit(DeviceContext* context)
{
    RBXASSERT(currentLevel != ssaoNone);
    SceneManager::GBuffer* gb = visualEngine->getSceneManager()->getGBuffer();
    RBXASSERT(gb);

    PIX_SCOPE(context, "SSAO/renderComposit");
    RBXPROFILER_SCOPE("Render", "SSAO::renderComposit");
	RBXPROFILER_SCOPE("GPU", "SSAO::renderComposit");

    if (ScreenSpaceEffect::renderFullscreenBegin(context, visualEngine, "PassThroughVS", "PassThroughFS", BlendState::Mode_None, data->width, data->height))
    {
        context->bindTexture(0, gb->mainColor.get(), SamplerState(SamplerState::Filter_Point, SamplerState::Address_Clamp));
        ScreenSpaceEffect::renderFullscreenEnd(context, visualEngine);
    }
}

void SSAO::onDeviceLost()
{
	data.reset();
}


SSAO::Data* SSAO::createData(unsigned int width, unsigned int height)
{
    unsigned int widthHalf = std::max(width / 2, 1u);
    unsigned int heightHalf = std::max(height / 2, 1u);

    std::auto_ptr<SSAO::Data> result(new SSAO::Data());

    Device* device = visualEngine->getDevice();

    for (int i = 0; i < 2; ++i)
    {
        result->ssao[i] = device->createTexture(Texture::Type_2D, Texture::Format_RGBA8, widthHalf, heightHalf, 1, 1, Texture::Usage_Renderbuffer);
		result->ssaoFB[i] = device->createFramebuffer(result->ssao[i]->getRenderbuffer(0, 0));
    }

    result->depthHalf = device->createTexture(Texture::Type_2D, Texture::Format_RG16, widthHalf, heightHalf, 1, 1, Texture::Usage_Renderbuffer);
	result->depthHalfFB = device->createFramebuffer(result->depthHalf->getRenderbuffer(0, 0));
    result->width = width;
    result->height = height;

    return result.release();
}

}
}
