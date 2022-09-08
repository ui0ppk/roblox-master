#include "stdafx.h"
#include "AdornRender.h"

#include "v8datamodel/DataModel.h"
#include "v8datamodel/Workspace.h"

#include "VisualEngine.h"
#include "VertexStreamer.h"
#include "TypesetterBitmap.h"
#include "TextureManager.h"
#include "ShaderManager.h"

#include "GfxBase/MeshGen.h"
#include "GfxBase/FrameRateManager.h"
#include "GfxBase/RenderStats.h"

#include "GfxCore/Texture.h"
#include "GfxCore/Device.h"
#include "GfxCore/States.h"
#include "GfxCore/Shader.h"

#include "util/IndexBox.h"
#include "util/Rotation2d.h"

#include "rbx/Profiler.h"

#ifdef _WIN32
#define alloca _alloca
#endif

namespace RBX
{
namespace Graphics
{

struct AdornVertex
{
    Vector3 position;
    Vector2 uv;
    Vector3 normal;

    AdornVertex()
	{
	}

    AdornVertex(const Vector3& position, const Vector3& normal)
		: position(position)
        , normal(normal)
	{
	}
};

static GeometryBatch* createBatch(Device* device, const shared_ptr<VertexLayout>& layout, const std::vector<AdornVertex>& vertices, const std::vector<unsigned short>& indices)
{
	shared_ptr<VertexBuffer> vbuf = device->createVertexBuffer(sizeof(AdornVertex), vertices.size(), GeometryBuffer::Usage_Static);

    vbuf->upload(0, &vertices[0], vertices.size() * sizeof(AdornVertex));

	shared_ptr<IndexBuffer> ibuf = device->createIndexBuffer(sizeof(unsigned short), indices.size(), GeometryBuffer::Usage_Static);

	ibuf->upload(0, &indices[0], indices.size() * sizeof(unsigned short));

	return new GeometryBatch(device->createGeometry(layout, vbuf, ibuf), Geometry::Primitive_Triangles, indices.size(), vertices.size());
}

static GeometryBatch* createBox(Device* device, const shared_ptr<VertexLayout>& layout)
{
	IndexBox box(-Vector3::one(), Vector3::one());

    std::vector<AdornVertex> vertices;
    std::vector<unsigned short> indices;

    for (int face = 0; face < 6; ++face)
	{
		Vector3 n = box.getFaceNormal(face);
        Vector3 v0, v1, v2, v3;
		box.getFaceCorners(face, v0, v1, v2, v3);

		vertices.push_back(AdornVertex(v0, n));
		vertices.push_back(AdornVertex(v1, n));
		vertices.push_back(AdornVertex(v2, n));
		vertices.push_back(AdornVertex(v3, n));

		indices.push_back(face * 4 + 0);
		indices.push_back(face * 4 + 1);
		indices.push_back(face * 4 + 2);

		indices.push_back(face * 4 + 0);
		indices.push_back(face * 4 + 2);
		indices.push_back(face * 4 + 3);
	}

	return createBatch(device, layout, vertices, indices);
}

static GeometryBatch* createCylinderX(Device* device, const shared_ptr<VertexLayout>& layout, int sides, bool zeroNormalBottom)
{
    std::vector<AdornVertex> vertices;
    std::vector<unsigned short> indices;

	RotationAngle increment(360.f / sides);
    RotationAngle current;

    // center vertices for caps
    vertices.push_back(AdornVertex(Vector3(-1, 0, 0), zeroNormalBottom ? Vector3::zero() : Vector3(-1, 0, 0)));
    vertices.push_back(AdornVertex(Vector3(+1, 0, 0), zeroNormalBottom ? Vector3::zero() : Vector3(+1, 0, 0)));

    size_t vertexOffset = vertices.size();
    size_t verticesPerSide = 4;

    for (int side = 0; side < sides; ++side)
	{
		Vector3 vcur(0, current.getSin(), current.getCos());

        vertices.push_back(AdornVertex(vcur - Vector3(1, 0, 0), vcur));
        vertices.push_back(AdornVertex(vcur + Vector3(1, 0, 0), vcur));

        vertices.push_back(AdornVertex(vcur - Vector3(1, 0, 0), zeroNormalBottom ? vcur : Vector3(-1, 0, 0)));
        vertices.push_back(AdornVertex(vcur + Vector3(1, 0, 0), zeroNormalBottom ? vcur : Vector3(+1, 0, 0)));

        current = current.combine(increment);
	}

    for (int side = 0; side < sides; ++side)
	{
        int side0 = side;
        int side1 = (side + 1) % sides;

        // side quad
        indices.push_back(vertexOffset + side0 * verticesPerSide + 0);
        indices.push_back(vertexOffset + side0 * verticesPerSide + 1);
        indices.push_back(vertexOffset + side1 * verticesPerSide + 1);

        indices.push_back(vertexOffset + side0 * verticesPerSide + 0);
        indices.push_back(vertexOffset + side1 * verticesPerSide + 1);
        indices.push_back(vertexOffset + side1 * verticesPerSide + 0);

        // caps
        indices.push_back(0);
        indices.push_back(vertexOffset + side0 * verticesPerSide + 2);
        indices.push_back(vertexOffset + side1 * verticesPerSide + 2);

        indices.push_back(1);
        indices.push_back(vertexOffset + side1 * verticesPerSide + 3);
        indices.push_back(vertexOffset + side0 * verticesPerSide + 3);
    }

	return createBatch(device, layout, vertices, indices);
}

static GeometryBatch* createSphere(Device* device, const shared_ptr<VertexLayout>& layout)
{
    const int sidesU = 18;
    const int sidesV = 9;

    std::vector<AdornVertex> vertices;
    std::vector<unsigned short> indices;

	RotationAngle incrementU(360.f / sidesU);
	RotationAngle incrementV(180.f / sidesV);

	Vector3 avg;

	for (int i = 0; i < 4; ++i)
	{
		RotationAngle u = (i & 1) ? incrementU : RotationAngle();
		RotationAngle v = (i & 2) ? incrementV : RotationAngle();

		Vector3 pos(v.getSin() * u.getCos(), v.getSin() * u.getSin(), v.getCos());

		avg += pos;
	}

	float radius = 1 / (avg / 4).length();

    RotationAngle currentU;

    for (int u = 0; u < sidesU; ++u)
	{
        RotationAngle currentV;

        for (int v = 0; v <= sidesV; ++v)
        {
            Vector3 pos(currentV.getSin() * currentU.getCos(), currentV.getSin() * currentU.getSin(), currentV.getCos());

			vertices.push_back(AdornVertex(pos * radius, pos));

			currentV = currentV.combine(incrementV);
        }

		currentU = currentU.combine(incrementU);
	}

    for (int u = 0; u < sidesU; ++u)
	{
        int u0 = u;
        int u1 = (u + 1) % sidesU;

        for (int v = 0; v < sidesV; ++v)
        {
            int v0 = v;
            int v1 = v + 1;

            indices.push_back(u0 * (sidesV + 1) + v0);
            indices.push_back(u1 * (sidesV + 1) + v1);
            indices.push_back(u1 * (sidesV + 1) + v0);

            indices.push_back(u0 * (sidesV + 1) + v0);
            indices.push_back(u0 * (sidesV + 1) + v1);
            indices.push_back(u1 * (sidesV + 1) + v1);
        }
	}

	return createBatch(device, layout, vertices, indices);
}

static GeometryBatch* createCone(Device* device, const shared_ptr<VertexLayout>& layout)
{
    const int sides = 12;

    std::vector<AdornVertex> vertices;
    std::vector<unsigned short> indices;

	RotationAngle increment(360.f / sides);
    RotationAngle current;

	vertices.push_back(AdornVertex(Vector3(0, 0, 0), Vector3(-1, 0, 0)));

    for (int side = 0; side < sides; ++side)
	{
		RotationAngle next = (side + 1 < sides) ? current.combine(increment) : RotationAngle();

		Vector3 vcur(0, current.getSin(), current.getCos());
		Vector3 vnext(0, next.getSin(), next.getCos());
        Vector3 apex(1, 0, 0);

        size_t vertexOffset = vertices.size();

        vertices.push_back(AdornVertex(vcur, vcur));
        vertices.push_back(AdornVertex(vnext, vnext));
        vertices.push_back(AdornVertex(apex, (vcur + vnext).unit()));

		vertices.push_back(AdornVertex(vcur, Vector3(-1, 0, 0)));
		vertices.push_back(AdornVertex(vnext, Vector3(-1, 0, 0)));

        // side
		indices.push_back(vertexOffset + 0);
        indices.push_back(vertexOffset + 2);
        indices.push_back(vertexOffset + 1);

        // base
		indices.push_back(0);
        indices.push_back(vertexOffset + 3);
        indices.push_back(vertexOffset + 4);

        current = next;
    }

	return createBatch(device, layout, vertices, indices);
}

static GeometryBatch* createTorus(Device* device, const shared_ptr<VertexLayout>& layout)
{
    const int sides = 12;

    std::vector<AdornVertex> vertices;
    std::vector<unsigned short> indices;

	RotationAngle increment(360.f / sides);
    RotationAngle current;

	vertices.push_back(AdornVertex(Vector3(0, 0, 0), Vector3(-1, 0, 0)));

    for (int side = 0; side < sides; ++side)
	{
		RotationAngle next = (side + 1 < sides) ? current.combine(increment) : RotationAngle();

		Vector3 vcur(0, current.getSin(), current.getCos());
		Vector3 vnext(0, next.getSin(), next.getCos());
        Vector3 apex(1, 0, 0);

        size_t vertexOffset = vertices.size();

        vertices.push_back(AdornVertex(vcur, vcur));
        vertices.push_back(AdornVertex(vnext, vnext));
        vertices.push_back(AdornVertex(apex, (vcur + vnext).unit()));

		vertices.push_back(AdornVertex(vcur, Vector3(-1, 0, 0)));
		vertices.push_back(AdornVertex(vnext, Vector3(-1, 0, 0)));

        // side
		indices.push_back(vertexOffset + 0);
        indices.push_back(vertexOffset + 2);
        indices.push_back(vertexOffset + 1);

        // base
		indices.push_back(0);
        indices.push_back(vertexOffset + 3);
        indices.push_back(vertexOffset + 4);

        current = next;
    }

	return createBatch(device, layout, vertices, indices);
}


class TextureProxy: public RBX::TextureProxyBase 
{
public:
    TextureProxy(const TextureRef& texture)
        : texture(texture)
    {
    }

    const shared_ptr<Texture>& getTexture() const { return texture.getTexture(); }
    
    virtual G3D::Vector2 getOriginalSize()
    {
		const ImageInfo& info = texture.getInfo();

		return G3D::Vector2(info.width, info.height);
    }

private:
    TextureRef texture;
};

AdornRender::AdornMaterial::AdornMaterial()
    : colorHandle(-1)
    , pixelInfoHandle(-1)
{
}

AdornRender::AdornMaterial::AdornMaterial(const shared_ptr<ShaderProgram>& program)
	: program(program)
	, colorHandle(program ? program->getConstantHandle("Color") : -1)
    , pixelInfoHandle(program ? program->getConstantHandle("PixelInfo") : -1)
{
}

AdornRender::AdornRender(VisualEngine* visualEngine, const DataModel* dataModel) 
    : visualEngine(visualEngine)
    , dataModel(dataModel)
    , currentTextureType(BatchTextureType_Color)
{
	std::vector<VertexLayout::Element> elements;
	elements.push_back(VertexLayout::Element(0, offsetof(AdornVertex, position), VertexLayout::Format_Float3, VertexLayout::Semantic_Position));
	elements.push_back(VertexLayout::Element(0, offsetof(AdornVertex, uv), VertexLayout::Format_Float2, VertexLayout::Semantic_Texture));
	elements.push_back(VertexLayout::Element(0, offsetof(AdornVertex, normal), VertexLayout::Format_Float3, VertexLayout::Semantic_Normal));

	shared_ptr<VertexLayout> layout = visualEngine->getDevice()->createVertexLayout(elements);

	batchBox.reset(createBox(visualEngine->getDevice(), layout));
	batchCylinderX.reset(createCylinderX(visualEngine->getDevice(), layout, 12, false));
	batchSphere.reset(createSphere(visualEngine->getDevice(), layout));
	batchCone.reset(createCone(visualEngine->getDevice(), layout));
    batchAALineCylinderX.reset(createCylinderX(visualEngine->getDevice(), layout, 12, true));    

	materials[Material_Default] = visualEngine->getShaderManager()->getProgramOrFFP("AdornLightingVS", "AdornFS");
	materials[Material_NoLighting] = visualEngine->getShaderManager()->getProgramOrFFP("AdornVS", "AdornFS");
	materials[Material_SelfLit] = visualEngine->getShaderManager()->getProgramOrFFP("AdornSelfLitVS", "AdornFS");
	materials[Material_SelfLitHighlight] = visualEngine->getShaderManager()->getProgramOrFFP("AdornSelfLitHighlightVS", "AdornFS");
    materials[Material_AALine] = visualEngine->getShaderManager()->getProgramOrFFP("AdornAALineVS", "AdornAALineFS");
    materials[Material_Outline] = visualEngine->getShaderManager()->getProgramOrFFP("AdornOutlineVS", "AdornOutlineFS");
}

RBX::Rect2D AdornRender::getViewport() const
{
	return Rect2D::xywh(0, 0, visualEngine->getViewWidth(), visualEngine->getViewHeight());
}

const Camera* AdornRender::getCamera() const
{
    return dataModel->getWorkspace()->getConstCamera();
}

void AdornRender::setTexture(int id, const RBX::TextureProxyBaseRef& t)
{
    if (t)
    {
        currentTexture = boost::polymorphic_downcast<TextureProxy*>(t.get())->getTexture();
    }
    else
    {
        currentTexture.reset();
    }
}

Rect2D AdornRender::getTextureSize(const RBX::TextureProxyBaseRef& texture) const
{
    return RBX::Rect2D(texture->getOriginalSize());
}

void AdornRender::rect2dImpl(const Vector2& x0y0, const Vector2& x1y0, const Vector2& x0y1, const Vector2& x1y1, const Vector2& tex0, const Vector2& tex1, const Color4& color)
{
    Vector2 px0y0(x0y0);
    Vector2 px1y0(x1y0);
    Vector2 px0y1(x0y1);
    Vector2 px1y1(x1y1);

	float height = currentHeight;

    px0y0.y = height - px0y0.y;
    px1y0.y = height - px1y0.y;
    px0y1.y = height - px0y1.y;
    px1y1.y = height - px1y1.y;

    visualEngine->getVertexStreamer()->rectBlt(currentTexture, color, px0y0, px1y0, px0y1, px1y1, tex0, tex1, currentTextureType, getIgnoreTexture());
}

void AdornRender::line2d(const RBX::Vector2& p0, const RBX::Vector2& p1, const RBX::Color4 &color)
{
	visualEngine->getVertexStreamer()->line(p0.x, currentHeight-p0.y, p1.x, currentHeight-p1.y, &color[0]);
}

Vector2 AdornRender::drawFont2DImpl(
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
    const Rotation2D&   rotation)
{
    const shared_ptr<Typesetter>& typesetter = visualEngine->getTypesetter(font);

    if (!typesetter) 
        return Vector2::zero();

    const shared_ptr<Texture>& tex = typesetter->getTexture();
    
    shared_ptr<Texture> oldTexture = tex;
    oldTexture.swap(currentTexture);
    currentTextureType = BatchTextureType_Font;

    Vector2 result = typesetter->draw(target, s, position, size, autoScale, color, outline, xalign, yalign, availableSpace, clippingRect, rotation);

    currentTextureType = BatchTextureType_Color;
    oldTexture.swap(currentTexture);

    return result;
}

Vector2 AdornRender::get2DStringBounds(const std::string& s, float size, Text::Font font, const Vector2& availableSpace) const 
{
    const shared_ptr<Typesetter>& typesetter = visualEngine->getTypesetter(font);

    if (!typesetter) 
        return Vector2::zero();
    
    return typesetter->measure(s, size, availableSpace, NULL);
}

TextureProxyBaseRef AdornRender::createTextureProxy(const ContentId& id, bool& waiting, bool bBlocking, const std::string& context)
{
    TextureRef texture = visualEngine->getTextureManager()->load(id, TextureManager::Fallback_None, context);

    waiting = (texture.getStatus() == TextureRef::Status_Waiting);

    if (texture.getStatus() == TextureRef::Status_Loaded)
    {
        return TextureProxyBaseRef(new TextureProxy(texture));
    }
	else
	{
        return TextureProxyBaseRef();
	}
}

rbx::signal<void()>& AdornRender::getUnbindResourcesSignal()
{
    return unbindResourcesSignal;
}

void AdornRender::setObjectToWorldMatrix(const CoordinateFrame& c) 
{
    currentCFrame = c;
}

static const float kSqrt3 = 1.7320508f;

void AdornRender::box(const AABox& box, const Color4& solidColor)
{
    Vector3 center = currentCFrame.pointToWorldSpace(box.center());
	Vector3 extent = box.extent();

	submitMesh(*batchBox, Material_NoLighting, center, currentCFrame.rotation, extent * 0.5f, solidColor, Sphere(center, extent.max() / 2 * kSqrt3));
}

void AdornRender::box(const CoordinateFrame& cFrame, const Vector3& size, const Color4& color, int zIndex, bool alwaysOnTop)
{
	submitMesh(*batchBox, Material_NoLighting, cFrame.translation, cFrame.rotation, size, color, Sphere(cFrame.translation, size.max() / 2 * kSqrt3), 0.0f, zIndex, alwaysOnTop);
}

void AdornRender::cylinder(const CoordinateFrame& cFrame, const float radius, const float height, const Color4& color, const int zIndex, const bool alwaysOnTop)
{
	submitMesh(*batchCylinderX, Material_NoLighting, cFrame.translation, cFrame.rotation, Vector3(height / 2, radius, radius), color, Sphere(cFrame.translation, std::max(radius, height / 2) * kSqrt3), 0.0f, zIndex, alwaysOnTop);
}

void AdornRender::cylinderAlongX(float radius, float length, const Color4& solidColor, bool cap)
{
	Vector3 center = currentCFrame.translation;

	submitMesh(*batchCylinderX, getMaterial(), center, currentCFrame.rotation, Vector3(length / 2, radius, radius), solidColor, Sphere(center, std::max(radius, length / 2) * kSqrt3));
}

void AdornRender::sphere(const Sphere& sphere, const Color4& solidColor)
{
	Vector3 center = currentCFrame.pointToWorldSpace(sphere.center);
    float radius = sphere.radius;

	submitMesh(*batchSphere, getMaterial(), center, currentCFrame.rotation, Vector3(radius, radius, radius), solidColor, Sphere(center, radius));
}

void AdornRender::sphere(const CoordinateFrame& cFrame, float radius, const Color4& color, int zIndex, bool alwaysOnTop)
{
	submitMesh(*batchSphere, Material_NoLighting, cFrame.translation, cFrame.rotation, Vector3(radius, radius, radius), color, Sphere(cFrame.translation, radius), 0.0f, zIndex, alwaysOnTop);
}

void AdornRender::extrusion(I3DLinearFunc* trajectory, int trajectorysegments, I3DLinearFunc* profile, int profilesegments, const Color4& color, bool closeTrajectory, bool closeProfile)
{
    float radius = profile->eval(0).length();

	for (int i = 0; i < trajectorysegments; ++i)
	{
		Vector3 from = currentCFrame.pointToWorldSpace(trajectory->eval(static_cast<float>(i) / trajectorysegments));
		Vector3 to = currentCFrame.pointToWorldSpace(trajectory->eval((i + 1 == trajectorysegments && closeTrajectory) ? 0.f : static_cast<float>(i + 1) / trajectorysegments));

        Vector3 axisX = (to - from).unit();
        Vector3 axisY = (G3D::abs(axisX.y) < 0.7f ? Vector3(0, 1, 0) : Vector3(1, 0, 0)).unitCross(axisX);
        Vector3 axisZ = axisX.unitCross(axisY);

        Matrix3 rotation;
        rotation.setColumn(0, axisX);
        rotation.setColumn(1, axisY);
        rotation.setColumn(2, axisZ);

        Vector3 center = (from + to) / 2;
        float length = (to - from).length();

		submitMesh(*batchCylinderX, getMaterial(), center, rotation, Vector3(length / 2, radius, radius), color, Sphere(center, length / 2));
	}
}

void AdornRender::axes(const Color4& xColor, const Color4& yColor, const Color4& zColor, float scale)
{
}

void AdornRender::cone(const CoordinateFrame& cFrame, float radius, float height, const Color4& color, int zIndex, bool alwaysOnTop)
{
	submitMesh(*batchCone, Material_NoLighting, cFrame.translation, cFrame.rotation, Vector3(height, radius, radius), color, Sphere(cFrame.translation, std::max(radius, height) * kSqrt3), 0.0f, zIndex, alwaysOnTop);
}

void AdornRender::ray(const RbxRay& ray, const Color4& color)
{
	float length = ray.direction().length();

	Sphere worldBounds(currentCFrame.pointToWorldSpace(ray.origin() + ray.direction() / 2), length / 2);

    const float axisStalkLength = 1.f;
    const float axisHeadLength = 0.3f;
	const float axisLength = axisStalkLength + axisHeadLength;
    const float axisHeadBaseRadius = 0.075f;
	const float axisStalkRadius = axisHeadBaseRadius / 3;

    float scale = length / axisLength;

	Vector3 stalkCenter = currentCFrame.pointToWorldSpace(ray.origin() + ray.direction() / 2 * (axisStalkLength / axisLength));
	float stalkScaleX = axisStalkLength / 2 * scale;
	float stalkScaleYZ = axisStalkRadius * scale;

	Vector3 coneOrigin = currentCFrame.pointToWorldSpace(ray.origin() + ray.direction() * (axisStalkLength / axisLength));
	float coneScaleX = axisHeadLength * scale;
	float coneScaleYZ = axisHeadBaseRadius * scale;

	Vector3 axisX = currentCFrame.vectorToWorldSpace(ray.direction().unit());
	Vector3 axisY = (G3D::abs(axisX.y) < 0.7f ? Vector3(0, 1, 0) : Vector3(1, 0, 0)).unitCross(axisX);
	Vector3 axisZ = axisX.unitCross(axisY);

    Matrix3 rotation;
	rotation.setColumn(0, axisX);
	rotation.setColumn(1, axisY);
	rotation.setColumn(2, axisZ);

	submitMesh(*batchCylinderX, getMaterial(), stalkCenter, rotation, Vector3(stalkScaleX, stalkScaleYZ, stalkScaleYZ), color, worldBounds);
	submitMesh(*batchCone, getMaterial(), coneOrigin, rotation, Vector3(coneScaleX, coneScaleYZ, coneScaleYZ), color, worldBounds);
}

void AdornRender::line3d(const Vector3& startPoint, const Vector3& endPoint, const RBX::Color4& color)
{
    const Vector3 p0 = currentCFrame.vectorToWorldSpace(startPoint) + currentCFrame.translation;
    const Vector3 p1 = currentCFrame.vectorToWorldSpace(endPoint) + currentCFrame.translation;
    visualEngine->getVertexStreamer()->line3d(p0.x,p0.y,p0.z,p1.x,p1.y,p1.z,&color[0]);
}

void AdornRender::line3dAA(const Vector3& startPoint, const Vector3& endPoint, const RBX::Color4& color, float thickness, int zIndex, bool alwaysOnTop)
{
    // these points cant be behind the camera position, lets project them on infinite plane defined by cam near plane
    const RenderCamera camera = visualEngine->getCamera();
    Vector3 camHeading = camera.getViewMatrix().upper3x3().row(2); 
    Plane pl = Plane(camHeading, camera.getPosition() + camHeading * -0.5f);
    
    float distanceStart = pl.distance(startPoint);
    float distanceEnd = pl.distance(endPoint);
    if (distanceStart > 0 && distanceEnd > 0)
        return; // nothing to do here

    Vector3 startP = startPoint;
    if (distanceStart > 0)
    {
        Vector3 dir = (endPoint - startPoint).direction();
        RbxRay ray = RbxRay(startPoint, dir);
        startP = ray.intersectionPlane(pl);
    }

    Vector3 endP = endPoint;
    if (distanceEnd > 0)
    {
        Vector3 dir = (startPoint - endPoint).direction();
        RbxRay ray = RbxRay(endPoint, dir);
        endP = ray.intersectionPlane(pl);
    }

    Vector3 startToEnd = endP - startP;

    Vector3 center = startP + startToEnd * 0.5f;
    Vector3 scale = Vector3(startToEnd.length() * 0.5f, 1, 1);

    Matrix3 rotMat = Matrix3::identity();
    float lngth = fabs(startToEnd.direction().unit().dot(Vector3(1,0,0)));
    if (lngth < 1)
    {
        Vector3 rotAxis = startToEnd.direction().cross(Vector3(1,0,0));
        float rotAngle = -acos(startToEnd.direction().dot(Vector3(1,0,0)));
        rotMat =  Matrix3::fromAxisAngle(rotAxis, rotAngle);
    }

    submitMesh(*batchAALineCylinderX, Material_AALine, center, rotMat, scale, color, Sphere(center, scale.max() / 2 * kSqrt3), thickness, zIndex, alwaysOnTop);
}

void AdornRender::quad(const Vector3& v0, const Vector3& v1, const Vector3& v2, const Vector3& v3, const Color4& color, const Vector2& v0tex, const Vector2& v2tex, int zIndex, bool alwaysOnTop)
{
    if ((v2 - v0).isZero())
        return;

    visualEngine->getVertexStreamer()->spriteBlt3D(currentTexture, &color[0], currentTextureType,
        currentCFrame.pointToWorldSpace(v0),
        currentCFrame.pointToWorldSpace(v1),
        currentCFrame.pointToWorldSpace(v2),
        currentCFrame.pointToWorldSpace(v3),
        v0tex, v2tex, zIndex, alwaysOnTop && zIndex >= 0);
}

void AdornRender::convexPolygon2d(const Vector2* v, int countv, const Color4& color)
{
    // swap from UI convention (0,0 top left), to GFX convention (0,0 bottom left)
    Vector2* vertices = (Vector2*) alloca(sizeof(Vector2) * countv);
    for(int i = 0; i < countv; ++i)
    {
        vertices[i] = Vector2(v[i].x, currentHeight-v[i].y);
    }

    short* indices = (short*) alloca(sizeof(short) * (countv-2) * 3);
    short start = 0;
    int icount = 0;
    for(int i = 1; i < countv -1; ++i)
    {
        indices[icount++] = start;
        indices[icount++] = i;
        indices[icount++] = i+1;
    }
    RBXASSERT(icount == (countv-2) * 3);
    RBXASSERT(icount <= 0xFFFF);
    visualEngine->getVertexStreamer()->triangleList2d(color, vertices, countv, indices, icount);
}

void AdornRender::convexPolygon(const Vector3* v, int countv, const Color4& color)
{
    short* indices = (short*) alloca(sizeof(short) * (countv-2) * 3);
    short start = 0;
    int icount = 0;
    for(int i = 1; i < countv -1; ++i)
    {
        indices[icount++] = start;
        indices[icount++] = i;
        indices[icount++] = i+1;
    }
    RBXASSERT(icount == (countv-2) * 3);
    RBXASSERT(icount <= 0xFFFF);
    visualEngine->getVertexStreamer()->triangleList(color, currentCFrame, v, countv, indices, icount);
}

bool AdornRender::isVisible(const Extents& extents, const CoordinateFrame& cframe)
{
    return visualEngine->getCameraCullFrm().isVisible(extents, cframe);
}

AdornRender::~AdornRender()
{
    unbindResourcesSignal();
}

void AdornRender::explosion(const Sphere& sphere)  
{
}

void AdornRender::preSubmitPass()
{
	currentHeight = visualEngine->getViewHeight();
	vr = (visualEngine->getDevice()->getVR() != NULL);
}

void AdornRender::postSubmitPass()
{
	currentTexture.reset();
}

struct AdornMeshMaterialComparator
{
    bool operator()(const AdornMesh& lhs, const AdornMesh& rhs) const
	{
		return (lhs.material != rhs.material) ? lhs.material < rhs.material : memcmp(&lhs.color, &rhs.color, sizeof(lhs.color)) < 0;
	}
};

struct AdornMeshDistanceComparator
{
    bool operator()(const AdornMesh& lhs, const AdornMesh& rhs) const
	{
		return lhs.distanceKey > rhs.distanceKey;
	}
};

void AdornRender::prepareRenderPass()
{
	std::sort(meshesOpaque.begin(), meshesOpaque.end(), AdornMeshMaterialComparator());
	std::sort(meshesTransparent.begin(), meshesTransparent.end(), AdornMeshDistanceComparator());
	for (unsigned i = 0; i < Adorn::maximumZIndex + 1; ++i)
		std::sort(meshesNoDepthTest[i].begin(), meshesNoDepthTest[i].end(), AdornMeshDistanceComparator());
}

void AdornRender::finishRenderPass()
{
	meshesOpaque.clear();
	meshesTransparent.clear();

	for (unsigned i = 0; i < Adorn::maximumZIndex + 1; ++i)
		meshesNoDepthTest[i].clear();

    visualEngine->getVertexStreamer()->cleanUpFrameData();
}

void AdornRender::render(DeviceContext* context, RenderPassStats& stats)
{
    RBXPROFILER_SCOPE("Render", "Adorns");
    RBXPROFILER_SCOPE("GPU", "Adorns");

    shared_ptr<Texture> defaultTexture = visualEngine->getTextureManager()->getFallbackTexture(TextureManager::Fallback_White);
	RBXASSERT(defaultTexture);

    PIX_SCOPE(context, "AR::Adorns");

	context->bindTexture(0, defaultTexture.get(), SamplerState::Filter_Linear);

    context->setDepthState(DepthState(DepthState::Function_LessEqual, true));
	context->setRasterizerState(RasterizerState::Cull_Back);
	context->setBlendState(BlendState::Mode_None);

	renderMeshes(context, meshesOpaque, stats);

	context->setBlendState(BlendState::Mode_AlphaBlend);

	renderMeshes(context, meshesTransparent, stats);
}

void AdornRender::renderNoDepth(DeviceContext* context, RenderPassStats& stats, int renderIndex)
{
	shared_ptr<Texture> defaultTexture = visualEngine->getTextureManager()->getFallbackTexture(TextureManager::Fallback_White);
	RBXASSERT(defaultTexture);

	PIX_SCOPE(context, "AR::Adorns");

	context->bindTexture(0, defaultTexture.get(), SamplerState::Filter_Linear);

	context->setRasterizerState(RasterizerState::Cull_Back);
	context->setBlendState(BlendState::Mode_AlphaBlend);

    renderMeshes(context, meshesNoDepthTest[renderIndex], stats);
}

void AdornRender::submitMesh(const GeometryBatch& batch, Material material, const Vector3& translation, const Matrix3& rotation, const Vector3& scale, const Color4& color, const Sphere& worldBounds, float thickness, int zIndex, bool alwaysOnTop)
{
	const RenderCamera& camera = visualEngine->getCamera();

    // Skip meshes if we don't know how to render them
	if (materials[material].colorHandle < 0)
        return;

    // Visibility culling
	if (!camera.isVisible(worldBounds))
        return;

    // FRM distance culling
	float sqDistance = (visualEngine->getCamera().getPosition() - worldBounds.center).squaredLength();

	FrameRateManager* frm = visualEngine->getFrameRateManager();

	if (sqDistance > frm->GetRenderCullSqDistance())
        return;

    frm->AddBlockQuota(/* blockCount= */ 5, sqDistance, /* isInSpatialHash= */ false);

	CoordinateFrame cframe(rotation * Matrix3::fromDiagonal(scale), translation);
    
    AdornMesh mesh = { material, &batch, 0, thickness, zIndex, alwaysOnTop, color, cframe };

	if (zIndex >= 0 && zIndex <= Adorn::maximumZIndex)
    {
        mesh.distanceKey = (visualEngine->getCamera().getPosition() - translation).squaredLength();
        meshesNoDepthTest[zIndex].push_back(mesh);
    }
    else if (color.a < 1 || material == Material_AALine)
	{
        mesh.distanceKey = (visualEngine->getCamera().getPosition() - translation).squaredLength();
		meshesTransparent.push_back(mesh);
	}
	else
	{
		meshesOpaque.push_back(mesh);
	}
}

void AdornRender::renderMeshes(DeviceContext* context, const std::vector<AdornMesh>& meshes, RenderPassStats& stats)
{
    Material currentMaterial = Material_Default;
    Color4 currentColor;

    for (size_t i = 0; i < meshes.size(); ++i)
	{
		const AdornMesh& mesh = meshes[i];

		if (mesh.zIndex >= 0)
			context->setDepthState(DepthState(mesh.alwaysOnTop ? DepthState::Function_Always : DepthState::Function_LessEqual, false));

		if (i == 0 || currentMaterial != mesh.material)
		{
			currentMaterial = mesh.material;
            currentColor = mesh.color;
			
			context->bindProgram(materials[currentMaterial].program.get());
			context->setConstant(materials[currentMaterial].colorHandle, &currentColor.r, 1);

            stats.passChanges++;
		}
		else if (currentColor != mesh.color)
		{
			currentColor = mesh.color;

			context->setConstant(materials[currentMaterial].colorHandle, &currentColor.r, 1);
		}

        if (mesh.lineThickness > 0)
        {
            Vector2 screenSize = Vector2(visualEngine->getViewWidth(), visualEngine->getViewHeight());
            float pixelScale = tanf(dataModel->getWorkspace()->getCamera()->getFieldOfView() * 0.5f) / screenSize.y;
            Vector4 pixelInfo = Vector4(pixelScale, screenSize.x, screenSize.y / screenSize.x, mesh.lineThickness);

            context->setConstant(materials[currentMaterial].pixelInfoHandle, &pixelInfo.x, 1);
        } 

		Matrix4 transform(mesh.cframe);

		context->setWorldTransforms4x3(transform[0], 1);
		context->draw(*mesh.batch);

		stats.batches++;
		stats.faces += mesh.batch->getCount() / 3;
		stats.vertices += mesh.batch->getIndexRangeEnd() - mesh.batch->getIndexRangeBegin();
	}
}

} 
}
