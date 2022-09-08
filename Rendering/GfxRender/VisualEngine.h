#pragma once

#include "GfxBase/ViewBase.h"

#include "rbx/Boost.hpp"
#include <string>
#include <boost/unordered_map.hpp>

#include "GfxBase/Type.h"
#include "RbxG3D/Frustum.h"
#include "RenderCamera.h"

namespace RBX
{
	class ContentProvider;
	class MeshContentProvider;
    class Lighting;

	class RenderStats;

	class FrameRateManager;
    class Camera;
    class RenderCaps;

	class Typesetter;
}

namespace RBX
{
namespace Graphics
{

class Device;

class Water;

class TextureCompositor;
class ShaderManager;
class TextureManager;
class SceneUpdater;
class SceneManager;
class TextureAtlas;

class MaterialGenerator;

class LightGrid;

class AdornRender;
class VertexStreamer;

class TypesetterBitmap;

class Material;

class VertexLayout;
struct EmitterShared;
	
class VisualEngine 
{
public:
	VisualEngine(Device* device, CRenderSettings* renderSettings);
    virtual ~VisualEngine();

    void bindWorkspace(const shared_ptr<DataModel>& dm);
    
    void setViewport(int width, int height);
    void setCamera(const Camera& value, const G3D::Vector3& poi);

    void reloadShaders();
    void queueAssetReload(const std::string& filePath);
	void immediateAssetReload(const std::string& filePath);
    void reloadQueuedAssets();

    Device* getDevice() { return device; }
    SceneManager *getSceneManager() { return sceneManager.get(); }
    FrameRateManager* getFrameRateManager() { return frameRateManager.get(); }

	RenderCamera& getCameraMutable() { return camera; }

	const RenderCamera& getCamera() { return camera; }
	const RenderCamera& getCameraCull() { return cameraCull; }
	const RenderCamera& getCameraCullFrm() { return cameraCullFrm; }

	int getViewWidth() const { return viewWidth; }
	int getViewHeight() const { return viewHeight; }

    AdornRender* getAdorn() { return adorn.get(); }
    VertexStreamer* getVertexStreamer() { return vertexStreamer.get(); }
    TextureCompositor * getTextureCompositor() { return textureCompositor.get(); }
    TextureManager* getTextureManager() { return textureManager.get(); }
	EmitterShared*  getEmitterSharedState() { return emitterShared.get(); }
    
    ShaderManager* getShaderManager() { return shaderManager.get(); }
    LightGrid* getLightGrid() { return lightGrid.get(); }
    Water* getWater() { return water.get(); }
    SceneUpdater* getSceneUpdater() { return sceneUpdater.get(); }
    CRenderSettings* getSettings() { return settings; };

    // returns area of interest that needs to be computed for next frame. slighly bigger than camera frustrum.
    // rendering of primitives outside this area is not allowed, since objects outside this area are not guaranteed to
    // be valid.
    const Frustum* getUpdateFrustum() { return &updateFrustum; }

    RenderStats* getRenderStats() { return renderStats.get(); }

    ContentProvider* getContentProvider() { return contentProvider; }
    MeshContentProvider* getMeshContentProvider() { return meshContentProvider; }
    Lighting* getLighting() { return lighting; }

    const shared_ptr<Typesetter>& getTypesetter(Text::Font font);

    const RenderCaps* getRenderCaps() { return renderCaps.get(); }

    MaterialGenerator* getMaterialGenerator() { return materialGenerator.get(); }

    shared_ptr<VertexLayout>& getFastClusterLayout() { return fastClusterLayout; }
    shared_ptr<VertexLayout>& getFastClusterShadowLayout() { return fastClusterShadowLayout; }

    TextureAtlas* getGlyphAtlas() const { return glyphAtlas.get(); }

private:
    Device* device;

	RenderCamera camera;
	RenderCamera cameraCull;
	RenderCamera cameraCullFrm;

    int viewWidth, viewHeight;
    
    Frustum updateFrustum;

    scoped_ptr<AdornRender> adorn;
	scoped_ptr<SceneUpdater> sceneUpdater;

    scoped_ptr<VertexStreamer> vertexStreamer;
    scoped_ptr<TextureCompositor> textureCompositor;
    scoped_ptr<TextureManager> textureManager;
    scoped_ptr<ShaderManager> shaderManager;
    scoped_ptr<SceneManager> sceneManager;
    scoped_ptr<FrameRateManager> frameRateManager;
	scoped_ptr<EmitterShared> emitterShared;

    RBX::CRenderSettings* settings;

    RBX::MeshContentProvider* meshContentProvider;
    RBX::ContentProvider* contentProvider;
    RBX::Lighting* lighting;

    scoped_ptr<RenderCaps> renderCaps;

    scoped_ptr<RBX::RenderStats> renderStats;

    scoped_ptr<LightGrid> lightGrid;
    scoped_ptr<Water> water;

    shared_ptr<Typesetter> typesetters[Text::FONT_LAST];
    scoped_ptr<TextureAtlas> glyphAtlas;

    scoped_ptr<MaterialGenerator> materialGenerator;

    shared_ptr<VertexLayout> fastClusterLayout;
    shared_ptr<VertexLayout> fastClusterShadowLayout;

    typedef boost::unordered_map<std::string, unsigned> FilenameCountdown;
    FilenameCountdown assetsToReload;
};

}
}
