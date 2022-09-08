#include "stdafx.h"
#include "VertexStreamer.h"

#include "VisualEngine.h"
#include "SceneManager.h"
#include "ShaderManager.h"
#include "TextureManager.h"

#include "Util.h"

#include "GfxCore/Device.h"
#include "GfxCore/States.h"

#include "GfxBase/RenderStats.h"

#include "rbx/Profiler.h"

FASTFLAG(GUIZFighterGPU)
FASTFLAG(UseDynamicTypesetterUTF8)

namespace RBX
{
namespace Graphics
{

static const unsigned int kVertexBufferMaxCount = 512*1024;

VertexStreamer::VertexStreamer(VisualEngine* visualEngine)
    : visualEngine(visualEngine)
{
    std::vector<VertexLayout::Element> elements;
    elements.push_back(VertexLayout::Element(0, offsetof(Vertex, pos), VertexLayout::Format_Float3, VertexLayout::Semantic_Position));
    elements.push_back(VertexLayout::Element(0, offsetof(Vertex, color), VertexLayout::Format_Color, VertexLayout::Semantic_Color));
    elements.push_back(VertexLayout::Element(0, offsetof(Vertex, uv), VertexLayout::Format_Float2, VertexLayout::Semantic_Texture));

    vertexLayout = visualEngine->getDevice()->createVertexLayout(elements);

	const DeviceCaps& caps = visualEngine->getDevice()->getCaps();

	colorOrderBGR = caps.colorOrderBGR;
	halfPixelOffset = caps.needsHalfPixelOffset;
}

VertexStreamer::~VertexStreamer()
{
}

void VertexStreamer::cleanUpFrameData()
{
	for (int cs = 0; cs < CS_NumTypes; ++cs)
	{
		chunks[cs].resize(0);
	}

	for (int i = 0; i < 10; ++i)
	{
		worldSpaceZDepthLayers[i].resize(0);
	}
    vertexData.fastClear();
}

void VertexStreamer::renderPrepare()
{
    RBXPROFILER_SCOPE("Render", "prepare");

	RBXASSERT(vertexData.size() <= kVertexBufferMaxCount);

    // allocate hardware buffer according to total # of vertices
    unsigned int bufferSize = 32;

    while (bufferSize < static_cast<unsigned int>(vertexData.size()))
        bufferSize += bufferSize / 2;

	bufferSize = std::min(bufferSize, kVertexBufferMaxCount);
    
    if (!vertexBuffer || vertexBuffer->getElementCount() < bufferSize)
    {
        vertexBuffer = visualEngine->getDevice()->createVertexBuffer(sizeof(Vertex), bufferSize, GeometryBuffer::Usage_Dynamic);

        geometry = visualEngine->getDevice()->createGeometry(vertexLayout, vertexBuffer, shared_ptr<IndexBuffer>());
    }

    if (vertexData.size() > 0)
    {
        void* bufferData = vertexBuffer->lock(GeometryBuffer::Lock_Discard);

        memcpy(bufferData, &vertexData[0], sizeof(Vertex) * vertexData.size());

        vertexBuffer->unlock();
    }

	if (chunks[CS_WorldSpaceNoDepth].size() > 0)
	{
		for (G3D::Array<VertexChunk>::iterator iter = chunks[CS_WorldSpaceNoDepth].begin(); iter != chunks[CS_WorldSpaceNoDepth].end(); ++iter)
		{
			RBXASSERT( iter->index >= 0 );
			worldSpaceZDepthLayers[iter->index].push_back(&(*iter));
		}
	}
}
 
void VertexStreamer::render3D(DeviceContext* context, const RenderCamera& camera, RenderPassStats& stats)
{
    RBXPROFILER_SCOPE("Render", "render3D");

    PIX_SCOPE(context, "3D");
    renderInternal(context, CS_WorldSpace, camera, stats);
}

void VertexStreamer::render3DNoDepth(DeviceContext* context, const RenderCamera& camera, RenderPassStats& stats, int renderIndex)
{
    RBXPROFILER_SCOPE("Render", "render3DNoDepth");

	PIX_SCOPE(context, "3D");
	renderInternal(context, CS_WorldSpaceNoDepth, camera, stats, renderIndex);
}

void VertexStreamer::render2D(DeviceContext* context, unsigned int viewWidth, unsigned int viewHeight, RenderPassStats& stats)
{
    RBXPROFILER_SCOPE("Render", "render2D");

    RenderCamera orthoCamera;
    orthoCamera.setViewMatrix(Matrix4::identity());
	orthoCamera.setProjectionOrtho(viewWidth, viewHeight, -1, 1, halfPixelOffset);

    renderInternal(context, CS_ScreenSpace, orthoCamera, stats);
}

void VertexStreamer::render2DVR(DeviceContext* context, unsigned int viewWidth, unsigned int viewHeight, RenderPassStats& stats)
{
    RBXPROFILER_SCOPE("Render", "render2DVR");

	float scaleX = float(viewWidth) / float(visualEngine->getViewWidth());
	float scaleY = float(viewHeight) / float(visualEngine->getViewHeight());

    RenderCamera orthoCamera;
    orthoCamera.setViewMatrix(Matrix4::identity());
	orthoCamera.setProjectionOrtho(viewWidth / scaleX, viewHeight / scaleY, -1, 1, halfPixelOffset);

	orthoCamera.setProjectionMatrix(Matrix4::scale(0.5, 0.5, 1) * orthoCamera.getProjectionMatrix());

    renderInternal(context, CS_ScreenSpace, orthoCamera, stats);
}

void VertexStreamer::renderFinish()
{
    cleanUpFrameData();
}

void VertexStreamer::renderInternal(DeviceContext* context, CoordinateSpace coordinateSpace, const RenderCamera& camera, RenderPassStats& stats, int renderIndex)
{
    if (chunks[coordinateSpace].size() == 0 || (coordinateSpace == CS_WorldSpaceNoDepth && worldSpaceZDepthLayers[renderIndex].size() == 0))
        return;

    // Get resources
	shared_ptr<Texture> defaultTexture = visualEngine->getTextureManager()->getFallbackTexture(TextureManager::Fallback_White);
    RBXASSERT(defaultTexture);

    static const SamplerState defaultSamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp);
    static const SamplerState wrapSamplerState(SamplerState::Filter_Linear, SamplerState::Address_Wrap);

    std::string vsName = (coordinateSpace == CS_WorldSpace) ? "UIFogVS" : "UIVS";
    std::string fsName = (coordinateSpace == CS_WorldSpace) ? "UIFogFS" : "UIFS";

    shared_ptr<ShaderProgram> shaderProgram = visualEngine->getShaderManager()->getProgramOrFFP(vsName, fsName);

    if (!shaderProgram)
        return;
    
    GlobalShaderData shaderData = visualEngine->getSceneManager()->readGlobalShaderData();

    shaderData.setCamera(camera);

    context->updateGlobalConstants(&shaderData, sizeof(GlobalShaderData));

    context->bindProgram(shaderProgram.get());

    // init luminance sampling
    static const float zFighterOffset = FFlag::GUIZFighterGPU ? 0.001f : 0;
    static const Vector4 luminanceSamplingOff = Vector4(0.0f, 0.0f, 0.0f, zFighterOffset);
    static const Vector4 luminanceSamplingOn = Vector4(1.0f, 1.0f, 1.0f, zFighterOffset);
    int uiParamsHandle = shaderProgram->getConstantHandle("UIParams");
    BatchTextureType currentTextureType = BatchTextureType_Count; // invalid so it will be set at first iteration
    
    context->setRasterizerState(RasterizerState::Cull_Back);
    context->setBlendState(BlendState(BlendState::Mode_AlphaBlend));
    context->setDepthState(DepthState(coordinateSpace == CS_WorldSpace ? DepthState::Function_LessEqual : DepthState::Function_Always, false));

    // Only required for FFP
    context->setWorldTransforms4x3(NULL, 0);

    stats.passChanges++;

    Texture* currentTexture = NULL;
	
	int arraySize = coordinateSpace == CS_WorldSpaceNoDepth ? worldSpaceZDepthLayers[renderIndex].size() : chunks[coordinateSpace].size();
	for (int i = 0; i < arraySize; ++i)
    {
		const VertexChunk& currChunk = coordinateSpace == CS_WorldSpaceNoDepth ? *worldSpaceZDepthLayers[renderIndex][i] : chunks[coordinateSpace][i];

		if (currChunk.index >= 0)
			context->setDepthState(DepthState(currChunk.alwaysOnTop ? DepthState::Function_Always : DepthState::Function_LessEqual, false));

        Texture* texture = currChunk.texture.get();
        
        // Change texture
        if (texture != currentTexture || i == 0)
        {
            currentTexture = texture;
            
            if (currentTextureType != currChunk.batchTextureType)
            {
                if (currChunk.batchTextureType == BatchTextureType_Font)
                    context->setConstant(uiParamsHandle, &luminanceSamplingOn[0], 1);
                else
                    context->setConstant(uiParamsHandle, &luminanceSamplingOff[0], 1);

                currentTextureType = currChunk.batchTextureType;
            }

            if (currChunk.batchTextureType == BatchTextureType_Font && FFlag::UseDynamicTypesetterUTF8)
                context->bindTexture(0, texture ? texture : defaultTexture.get(), wrapSamplerState);
            else
                context->bindTexture(0, texture ? texture : defaultTexture.get(), defaultSamplerState);
        }

        // Render!
        context->draw(geometry.get(), currChunk.primitive, currChunk.vertexStart, currChunk.vertexCount, 0, 0);

        stats.batches++;
        stats.faces += currChunk.vertexCount / 3;
        stats.vertices += currChunk.vertexCount;
    }
}

VertexStreamer::VertexChunk* VertexStreamer::prepareChunk(const shared_ptr<Texture>& texture, Geometry::Primitive primitive, unsigned int vertexCount, CoordinateSpace coordinateSpace, BatchTextureType batchTexType, bool ignoreTexture, int zIndex, bool forceTop)
{
    unsigned int vertexStart = vertexData.size();

	if (vertexStart + vertexCount > kVertexBufferMaxCount)
        return NULL;

    const shared_ptr<Texture>& actualTexture = ignoreTexture ? visualEngine->getTextureManager()->getFallbackTexture(TextureManager::Fallback_White) : texture;
    G3D::Array<VertexChunk>& orderedChunks = chunks[coordinateSpace];

    bool newsection = true;
    if (orderedChunks.size() > 0)
    {
        VertexChunk& last = orderedChunks.back();
        newsection =
            (last.texture != actualTexture) ||
            last.primitive != primitive ||
            last.vertexStart + last.vertexCount != vertexStart ||
			last.index != zIndex ||
			last.alwaysOnTop != forceTop;
    }

    if (newsection)
    {
        orderedChunks.push_back(VertexChunk(actualTexture, primitive, vertexStart, vertexCount, batchTexType, zIndex, forceTop));
    }
	else
	{
        RBXASSERT(orderedChunks.back().batchTextureType == batchTexType);
		orderedChunks.back().vertexCount += vertexCount;
	}

    return &orderedChunks.back();
}

void VertexStreamer::rectBlt(
    const shared_ptr<Texture>& texptr, const Color4& color4,
    const Vector2& x0y0, const Vector2& x1y0, const Vector2& x0y1, const Vector2& x1y1,
    const Vector2& tex0, const Vector2& tex1, BatchTextureType batchTexType, bool ignoreTexture)
{
    if (prepareChunk(texptr, Geometry::Primitive_Triangles, 6, CS_ScreenSpace, batchTexType, ignoreTexture))
	{
        float z = 0;
        unsigned int color = packColor(color4, colorOrderBGR);

		vertexData.push_back(Vertex(Vector3(x0y1.x, x0y1.y, z), color, Vector2(tex0.x, tex1.y)));
        vertexData.push_back(Vertex(Vector3(x1y0.x, x1y0.y, z), color, Vector2(tex1.x, tex0.y)));
        vertexData.push_back(Vertex(Vector3(x0y0.x, x0y0.y, z), color, Vector2(tex0.x, tex0.y)));
        vertexData.push_back(Vertex(Vector3(x0y1.x, x0y1.y, z), color, Vector2(tex0.x, tex1.y)));
        vertexData.push_back(Vertex(Vector3(x1y1.x, x1y1.y, z), color, Vector2(tex1.x, tex1.y)));
        vertexData.push_back(Vertex(Vector3(x1y0.x, x1y0.y, z), color, Vector2(tex1.x, tex0.y)));
	}
} 

void VertexStreamer::spriteBlt3D(const shared_ptr<Texture>& texptr, const Color4& color4, BatchTextureType batchTexType,
    const Vector3& x0y0, const Vector3& x1y0, const Vector3& x0y1, const Vector3& x1y1,
    const Vector2& tex0, const Vector2& tex1, int zIndex, bool alwaysOnTop)
{
    if (prepareChunk(texptr, Geometry::Primitive_Triangles, 6, alwaysOnTop || zIndex >= 0 ? CS_WorldSpaceNoDepth : CS_WorldSpace, batchTexType, false, zIndex, alwaysOnTop))
	{
        unsigned int color = packColor(color4, colorOrderBGR);

        vertexData.push_back(Vertex(x0y1, color, Vector2(tex0.x, tex1.y)));
        vertexData.push_back(Vertex(x1y0, color, Vector2(tex1.x, tex0.y)));
        vertexData.push_back(Vertex(x0y0, color, Vector2(tex0.x, tex0.y)));
        vertexData.push_back(Vertex(x0y1, color, Vector2(tex0.x, tex1.y)));
        vertexData.push_back(Vertex(x1y1, color, Vector2(tex1.x, tex1.y)));
        vertexData.push_back(Vertex(x1y0, color, Vector2(tex1.x, tex0.y)));
	}
}

void VertexStreamer::triangleList2d(const Color4& color4, const Vector2* v, int vcount, const short* indices, int icount)
{
    if (prepareChunk(shared_ptr<Texture>(), Geometry::Primitive_Triangles, icount, CS_ScreenSpace, BatchTextureType_Color))
	{
        unsigned int color = packColor(color4, colorOrderBGR);

        RBXASSERT(icount % 3 == 0);
        //todo: we currently just ignore the nice indexing work done. todo: support indexing.
        for (int i = 0; i < icount; ++i)
        {
			vertexData.push_back(Vertex(Vector3(v[indices[i]],0), color, Vector2()));
        }
	}
}

void VertexStreamer::triangleList(const Color4& color4, const CoordinateFrame& cframe, const Vector3* v, int vcount, const short* indices, int icount)
{
    if (prepareChunk(shared_ptr<Texture>(), Geometry::Primitive_Triangles, icount, CS_WorldSpace, BatchTextureType_Color ))
	{
        unsigned int color = packColor(color4, colorOrderBGR);

        RBXASSERT(icount % 3 == 0);
        //todo: we currently just ignore the nice indexing work done. todo: support indexing.
        for (int i = 0; i < icount; ++i)
        {
			vertexData.push_back(Vertex(cframe.pointToWorldSpace(v[indices[i]]), color, Vector2()));
        }
	}
}

void VertexStreamer::line(float x1, float y1, float x2, float y2, const Color4& color4)
{
    if (prepareChunk(shared_ptr<Texture>(), Geometry::Primitive_Lines, 2, CS_ScreenSpace, BatchTextureType_Color))
	{
        float z = 0;
        unsigned int color = packColor(color4, colorOrderBGR);

        vertexData.push_back(Vertex(Vector3(x1, y1, z), color, Vector2()));
        vertexData.push_back(Vertex(Vector3(x2, y2, z), color, Vector2()));
	}
}

void VertexStreamer::line3d(float x1, float y1, float z1, float x2, float y2, float z2, const Color4& color4)
{
    if (prepareChunk(shared_ptr<Texture>(), Geometry::Primitive_Lines, 2, CS_WorldSpace, BatchTextureType_Color))
	{
        unsigned int color = packColor(color4, colorOrderBGR);

        vertexData.push_back(Vertex(Vector3(x1, y1, z1), color, Vector2()));
        vertexData.push_back(Vertex(Vector3(x2, y2, z2), color, Vector2()));
	}
}

}
}
