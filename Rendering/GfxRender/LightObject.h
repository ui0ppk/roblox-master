#pragma once

#include "util/G3DCore.h"

#include "CullableSceneNode.h"

namespace RBX
{
	class PartInstance;
	class Light;
}

namespace RBX
{
namespace Graphics
{

struct LightShadowSlice
{
    boost::scoped_array<unsigned char> data;
};

struct LightShadowMap
{
    unsigned int size;

    LightShadowSlice sliceX;
    LightShadowSlice sliceY0;
    LightShadowSlice sliceY1;
    LightShadowSlice sliceZ;

    LightShadowMap(): size(0)
    {
    }
};

class LightObject: public CullableSceneNode
{
    typedef CullableSceneNode Super;

public:
    enum Type
	{
        Type_None,
        Type_Point,
        Type_Spot,
        Type_Surface
	};

    LightObject(VisualEngine* visualEngine);
    ~LightObject();

    void bind(const shared_ptr<RBX::PartInstance>& part, const shared_ptr<RBX::Light>& light);
    
    LightShadowMap* getShadowMap() const { return shadowMap.get(); }

	Type getType() const { return type; }
	const Color3& getColor() const { return color; }
	float getBrightness() const { return brightness; }
	float getRange() const { return range; }
	float getAngle() const { return angle; }

	const Vector3& getPosition() const { return position; }
	const Vector3& getDirection() const { return direction; }

	const Vector4& getAxisU() const { return axisU; }
	const Vector4& getAxisV() const { return axisV; }

    const Extents& getExtents() const;

    // GfxBinding overrides
    virtual void invalidateEntity();
    virtual void updateEntity(bool assetsUpdated);
    virtual void unbind();

    // GfxPart overrides
    virtual void updateCoordinateFrame(bool recalcLocalBounds);

private:
    void onCombinedSignalEx(Instance::CombinedSignalType type, const Instance::ICombinedSignalData* data);
    void onPropertyChangedEx(const RBX::Reflection::PropertyDescriptor* descriptor);
    void onAncestorChangedEx();
    void onSleepingChangedEx(bool sleeping);
    void onParentSizeChangedEx(const RBX::Reflection::PropertyDescriptor* pd);

    void invalidateLighting(const Extents& bbox);
    
    shared_ptr<RBX::PartInstance> part;
    shared_ptr<RBX::Light> light;

    scoped_ptr<LightShadowMap> shadowMap;
    CoordinateFrame transform;
    Vector3 position;
    Vector3 direction;
    Vector4 axisU;
    Vector4 axisV;

    Type type;

    Color3 color;
    float brightness;
    float range;
    float angle;

    bool dirty;
};

}
}
