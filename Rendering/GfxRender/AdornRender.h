#pragma once

#include "GfxBase/Adorn.h"
#include "GfxBase/TextureProxyBase.h"
#include "util/G3DCore.h"
#include "VertexStreamer.h"

namespace RBX
{
    class DataModel;
    struct RenderPassStats;
}

namespace RBX
{
namespace Graphics
{

class VisualEngine;
class Texture;
class ShaderProgram;
class GeometryBatch;
class DeviceContext;

struct AdornMesh
{
    Adorn::Material material;
    const GeometryBatch* batch;

    float distanceKey;
    float lineThickness;        // this is only AA-line specific, but not worth creating special mesh type for it

	int zIndex;
    bool alwaysOnTop;

    Color4 color;

    CoordinateFrame cframe;
};

class AdornRender : public Adorn
{
public:
    AdornRender(VisualEngine* visualEngine, const DataModel* dataModel);
    virtual ~AdornRender();
    
    virtual void prepareRenderPass();
    virtual void finishRenderPass();

    void preSubmitPass();
    void postSubmitPass();

    void render(DeviceContext* context, RenderPassStats& stats);
	void renderNoDepth(DeviceContext* context, RenderPassStats& stats, int renderIndex);

    virtual RBX::Rect2D getViewport() const;
    virtual const Camera* getCamera() const;

    TextureProxyBaseRef createTextureProxy(const ContentId& id, bool& waiting, bool bBlocking = false, const std::string& context = "");

    virtual void setTexture(int id, const RBX::TextureProxyBaseRef& t);
    virtual Rect2D getTextureSize(const TextureProxyBaseRef& texture) const;

    virtual void rect2dImpl(const Vector2& x0y0, const Vector2& x1y0, const Vector2& x0y1, const Vector2& x1y1, const Vector2& tex0, const Vector2& tex1, const Color4 & color);

    virtual void line2d(const RBX::Vector2& p0,	const RBX::Vector2& p1, const RBX::Color4& color);

    virtual Vector2 get2DStringBounds(const std::string& s, float size, Text::Font font, const Vector2& availableSpace) const;

    virtual Vector2 drawFont2DImpl(
        Adorn*				target,
        const std::string&  s,
        const Vector2&      position,
        float               size,
        bool                autoScale,
        const Color4&       color,
        const Color4&       outline,
        Text::Font			font,
        Text::XAlign        xalign,
        Text::YAlign        yalign,
        const Vector2&		availableSpace,
        const Rect2D&		clippingRect,
        const Rotation2D&   rotation);

    virtual void setObjectToWorldMatrix(const CoordinateFrame& c);

    virtual void box(const AABox& box, const Color4& solidColor);
	virtual void box(const CoordinateFrame& cFrame, const Vector3& size, const Color4& color, int zIndex, bool alwaysOnTop);
    virtual void sphere(const Sphere& sphere, const Color4& solidColor);
	virtual void sphere(const CoordinateFrame& cFrame, float radius, const Color4& color, int zIndex, bool alwaysOnTop);
    virtual void cylinderAlongX(float radius, float length, const Color4& solidColor, bool cap);
	virtual void cylinder(const CoordinateFrame& cFrame, float radius, float height, const Color4& color, int zIndex, bool alwaysOnTop);
	virtual void cone(const CoordinateFrame& cFrame, float radius, float height, const Color4& color, int zIndex, bool alwaysOnTop);

    virtual void ray(const RbxRay& ray, const Color4& color);
    virtual void axes(const Color4& xColor, const Color4& yColor, const Color4& zColor, float scale);
    virtual void extrusion(I3DLinearFunc* trajectory, int trajectorysegments, I3DLinearFunc* profile, int profilesegments, const Color4& color, bool closeTrajectory, bool closeProfile);

    virtual void explosion(const Sphere& sphere);

    virtual void line3d(const Vector3& startPoint, const Vector3& endPoint, const RBX::Color4& color);
    virtual void line3dAA(const Vector3& startPoint, const Vector3& endPoint, const RBX::Color4& color, float thickness, int zIndex, bool alwaysOnTop);

    virtual void quad(const Vector3& v0, const Vector3& v1, const Vector3& v2, const Vector3& v3, const Color4& color, const Vector2& v0tex, const Vector2& v2tex, int zIndex, bool alwaysOnTop);

    virtual void convexPolygon2d(const Vector2* v, int countv, const Color4& color);

    virtual void convexPolygon(const Vector3* v, int countv, const Color4& color);

    virtual bool isVisible(const Extents& extents, const CoordinateFrame& cframe);

private:
    struct AdornMaterial
	{
        shared_ptr<ShaderProgram> program;
        int colorHandle;
        int pixelInfoHandle;

		AdornMaterial();
        AdornMaterial(const shared_ptr<ShaderProgram>& program);
	};

    VisualEngine* visualEngine;
    const DataModel* dataModel;

    rbx::signal<void()> unbindResourcesSignal;

    float currentHeight;
    CoordinateFrame currentCFrame;
    shared_ptr<Texture> currentTexture;
    BatchTextureType currentTextureType;

    rbx::signal<void()>& getUnbindResourcesSignal();

    std::vector<AdornMesh> meshesOpaque;
    std::vector<AdornMesh> meshesTransparent;
    std::vector<AdornMesh> meshesNoDepthTest[Adorn::maximumZIndex + 1];

    scoped_ptr<GeometryBatch> batchBox;
    scoped_ptr<GeometryBatch> batchCylinderX;
    scoped_ptr<GeometryBatch> batchSphere;
    scoped_ptr<GeometryBatch> batchCone;
    scoped_ptr<GeometryBatch> batchTorus;
    scoped_ptr<GeometryBatch> batchAALineCylinderX;

	AdornMaterial materials[Material_Count];

    void submitMesh(const GeometryBatch& batch, Material material, const Vector3& translation, const Matrix3& rotation, const Vector3& scale, const Color4& color, const Sphere& worldBounds, float thickness = 0, const int zIndex = -1, const bool alwaysOnTop = false);

	void renderMeshes(DeviceContext* context, const std::vector<AdornMesh>& meshes, RenderPassStats& stats);
};

}
}
