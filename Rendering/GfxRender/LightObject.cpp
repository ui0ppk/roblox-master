#include "stdafx.h"
#include "LightObject.h"

#include "v8datamodel/Light.h"
#include "v8datamodel/PartInstance.h"

#include "SceneUpdater.h"
#include "Util.h"
#include "VisualEngine.h"

namespace RBX
{
namespace Graphics
{

// http://www.gamedev.net/topic/418582-aabb-boundary-of-a-round-cone/
static Extents computeConeExtents(const Vector3& position, float radius, const Vector3& direction, float angle)
{
    static const Vector3 axes[6] =
    {
        Vector3(-1, 0, 0), Vector3(0, -1, 0), Vector3(0, 0, -1),
        Vector3(+1, 0, 0), Vector3(0, +1, 0), Vector3(0, 0, +1),
    };
    
    Vector3 E[6];
    
    Vector3 C = position;
    Vector3 A = direction;
    float r = radius;
    float s = G3D::toRadians(angle / 2);
    float coss = cosf(s), sins = sinf(s);

    for (int axis = 0; axis < 6; ++axis)
    {
        Vector3 D = axes[axis];

        // extreme point is on spherical cap: dot(D, A) >= cos(s)
        if (dot(D, A) >= coss)
        {
            E[axis] = C + r*D;
        }
        // extreme point is the apex: dot(D, A) <= cos(s+pi/2)
        else if (dot(D, A) <= -sins)
        {
            E[axis] = C;
        }
        // extreme point is on the circle
        else
        {
            Matrix3 AAT =
                Matrix3(A.x * A.x, A.x * A.y, A.x * A.z,
                        A.y * A.x, A.y * A.y, A.y * A.z,
                        A.z * A.x, A.z * A.y, A.z * A.z);

            E[axis] = C + (r * coss) * A + (r * sins) * normalize(D - AAT * D);
        }
    }

    return Extents(Vector3(E[0].x, E[1].y, E[2].z), Vector3(E[3].x, E[4].y, E[5].z));
}

static Extents computeLightExtents(const Vector3& position, const Vector3& direction, const Vector4& axisU, const Vector4& axisV, Light* light)
{
    if (PointLight* pointLight = light->fastDynamicCast<PointLight>())
    {
        return Extents::fromCenterRadius(position, pointLight->getRange());
    }
    else if (SurfaceLight* surfaceLight = light->fastDynamicCast<SurfaceLight>())
    {
        Vector3 u = axisU.xyz() * axisU.w;
        Vector3 v = axisV.xyz() * axisV.w;

        Extents e;

        for (int i = 0; i < 4; ++i)
        {
            Vector3 c = position + (i & 1 ? 1 : -1) * u + (i & 2 ? 1 : -1) * v;
            Extents ec = computeConeExtents(c, surfaceLight->getRange(), direction, surfaceLight->getAngle());

            e.expandToContain(ec);
        }

        return e;
    }
    else if (SpotLight* spotLight = light->fastDynamicCast<SpotLight>())
    {
        return computeConeExtents(position, spotLight->getRange(), direction, spotLight->getAngle());
    }
    else
    {
        RBXASSERT(false);
        return Extents();
    }
}

static float getLightRange(Light* light)
{
    if (PointLight* pointLight = light->fastDynamicCast<PointLight>())
    {
        return pointLight->getRange();
    }
    else if (SpotLight* spotLight = light->fastDynamicCast<SpotLight>())
    {
        return spotLight->getRange();
    }
    else if (SurfaceLight* surfaceLight = light->fastDynamicCast<SurfaceLight>())
    {
        return surfaceLight->getRange();
    }
    else
    {
        RBXASSERT(false);
        return 0;
    }
}

static void resizeShadowProjection(boost::scoped_array<unsigned char>& data, unsigned int oldSize, unsigned int newSize)
{
    RBXASSERT(newSize > 0);
    RBXASSERT(newSize % 2 == 1); // all resize requests are odd; this simplifies the copy logic below

    unsigned char* newData = new unsigned char[newSize * newSize];

    // during resizing, we assume that the center part of the shadow map is preserved
    // we need to copy that over and clear everything else
    memset(newData, 0, newSize * newSize);

    if (oldSize != 0)
    {
        unsigned int sharedSize = std::min(oldSize, newSize);
        RBXASSERT(sharedSize % 2 == 1);

        const unsigned char* oldData = data.get();

        // positive by construction
        unsigned int sharedStartOld = (oldSize - sharedSize) / 2;
        unsigned int sharedStartNew = (newSize - sharedSize) / 2;

        for (unsigned int y = 0; y < sharedSize; ++y)
        {
            memcpy(newData + (y + sharedStartNew) * newSize + sharedStartNew, oldData + (y + sharedStartOld) * oldSize + sharedStartOld, sharedSize);
        }
    }

    data.reset(newData);
}

LightObject::LightObject(VisualEngine* visualEngine)
    : Super(visualEngine, CullMode_SpatialHash, Flags_LightObject)
	, type(Type_None)
    , brightness(0)
    , range(0)
    , angle(0)
    , dirty(false)
{
}

LightObject::~LightObject()
{
    unbind();

    // notify scene updater about destruction so that the pointer to LightObject is no longer stored
    getVisualEngine()->getSceneUpdater()->notifyDestroyed(this);
}

void LightObject::onSleepingChangedEx(bool sleeping)
{
    if (sleeping)
    {
        getVisualEngine()->getSceneUpdater()->notifySleeping(this);
    }
    else
    {
        getVisualEngine()->getSceneUpdater()->notifyAwake(this);		
    }
}

void LightObject::onParentSizeChangedEx(const RBX::Reflection::PropertyDescriptor* pd)
{
    if (pd == &PartInstance::prop_Size)
    {
        invalidateEntity();
    }
}

static void getBasis(NormalId face, const Matrix3& rotation, const Vector3& halfSize, Vector4& axis, Vector4& axisU, Vector4& axisV)
{
    switch (face)
    {
    case NORM_X:
    case NORM_X_NEG:
		axis = Vector4(Math::getWorldNormal(face, rotation), halfSize.x);
        axisU = Vector4(Math::getColumn(rotation, 1), halfSize.y);
        axisV = Vector4(Math::getColumn(rotation, 2), halfSize.z);
        break;

    case NORM_Y:
    case NORM_Y_NEG:
		axis = Vector4(Math::getWorldNormal(face, rotation), halfSize.y);
        axisU = Vector4(Math::getColumn(rotation, 2), halfSize.z);
        axisV = Vector4(Math::getColumn(rotation, 0), halfSize.x);
        break;

    case NORM_Z:
    case NORM_Z_NEG:
		axis = Vector4(Math::getWorldNormal(face, rotation), halfSize.z);
        axisU = Vector4(Math::getColumn(rotation, 0), halfSize.x);
        axisV = Vector4(Math::getColumn(rotation, 1), halfSize.y);
        break;

    default:
        axisU = Vector4();
        axisV = Vector4();
    }
}

void LightObject::updateCoordinateFrame(bool recalcLocalBounds)
{
    if (!light) return;
    
    Extents oldWorldBB = getWorldBounds();

    CoordinateFrame frame = part ? part->calcRenderingCoordinateFrame() : CoordinateFrame();
    
    if (!dirty && transform.fuzzyEq(frame))
    {
        // Nothing to update
        return;
    }
    
    transform = frame;
    
    if (part)
    {
        // Update basis
		position = transform.translation;
        direction = Vector3();
        axisU = Vector4();
        axisV = Vector4();

        if (SurfaceLight* surfaceLight = light->fastDynamicCast<SurfaceLight>())
        {
            Vector4 axis;
            getBasis(surfaceLight->getFace(), transform.rotation, part->getPartSizeUi() * 0.5f, axis, axisU, axisV);

			position = transform.translation + axis.xyz() * axis.w;
			direction = axis.xyz();
        }
        else if (SpotLight* spotLight = light->fastDynamicCast<SpotLight>())
        {
			direction = Math::getWorldNormal(spotLight->getFace(), transform.rotation);
        }
        
        // Update world-space light extents
		Extents extents = computeLightExtents(position, direction, axisU, axisV, light.get());
    
        updateWorldBounds(extents);
    }
    else
    {
        // Reset world-space light extents
        updateWorldBounds(Extents());
        
        // Make sure the node is not in the spatial hash
        RBXASSERT(!IsInSpatialHash());
    }

    invalidateLighting(oldWorldBB);
    invalidateLighting(getWorldBounds());
}

void LightObject::onCombinedSignalEx(Instance::CombinedSignalType type, const Instance::ICombinedSignalData* data)
{
    switch (type)
    {
    case Instance::PROPERTY_CHANGED:
        onPropertyChangedEx(boost::polymorphic_downcast<const Instance::PropertyChangedSignalData*>(data)->propertyDescriptor);
        break;
    case Instance::ANCESTRY_CHANGED:
        onAncestorChangedEx();
        break;
    default:
        break;
    }
}

void LightObject::onPropertyChangedEx(const RBX::Reflection::PropertyDescriptor* descriptor)
{
    invalidateEntity();
}

void LightObject::onAncestorChangedEx()
{
    shared_ptr<Light> lightCopy = light;
    
    // Remove me from the scene if I am being removed from the Workspace
    if (!isInWorkspace(lightCopy.get()))
    {
        // will cause a delete on next updateEntity()
        zombify();
    }
    else
    {
        unbind();
        
        RBX::PartInstance* parent = RBX::Instance::fastDynamicCast<RBX::PartInstance>(lightCopy->getParent());
        shared_ptr<RBX::PartInstance> part = shared_from(parent);
            
        bind(part, lightCopy);
    }
}

void LightObject::bind(const shared_ptr<RBX::PartInstance>& part, const shared_ptr<RBX::Light>& light)
{
    RBXASSERT(!this->part && !this->light);
    RBXASSERT(light);
    
    this->part = part;
    this->light = light;

    connections.push_back(light->combinedSignal.connect(boost::bind(&LightObject::onCombinedSignalEx, this, _1, _2)));

    if (part)
    {
        connections.push_back(part->onDemandWrite()->sleepingChangedSignal.connect(boost::bind(&LightObject::onSleepingChangedEx, this, _1)));
        
        if (light->fastDynamicCast<SurfaceLight>())
            connections.push_back(part->propertyChangedSignal.connect( boost::bind(&LightObject::onParentSizeChangedEx, this, _1)));

        // we just connected, so sync up the state.
        onSleepingChangedEx(part->getSleeping());
    }
    
    invalidateEntity();
}

void LightObject::unbind()
{
    Super::unbind();
    
    part.reset();
    light.reset();
}

void LightObject::invalidateEntity()
{
    if (!dirty)
    {
        dirty = true;
        
        getVisualEngine()->getSceneUpdater()->queueInvalidatePart(this);
    }
}

void LightObject::updateEntity(bool assetsUpdated)
{
    if (connections.empty()) // zombified.
    {
        invalidateLighting(getWorldBounds());
        
		getVisualEngine()->getSceneUpdater()->destroyAttachment(this);
        return;
    }
    
    updateCoordinateFrame(true);

	if (light && light->getEnabled())
	{
		color = light->getColor();
		brightness = light->getBrightness();

        if (SurfaceLight* surfaceLight = light->fastDynamicCast<SurfaceLight>())
        {
			type = Type_Surface;
			range = surfaceLight->getRange();
			angle = surfaceLight->getAngle();
        }
        else if (SpotLight* spotLight = light->fastDynamicCast<SpotLight>())
        {
			type = Type_Spot;
			range = spotLight->getRange();
			angle = spotLight->getAngle();
        }
		else if (PointLight* pointLight = light->fastDynamicCast<PointLight>())
		{
			type = Type_Point;
			range = pointLight->getRange();
            angle = 0;
		}
		else
		{
			RBXASSERT(false);
			type = Type_None;
		}
	}
	else
	{
        type = Type_None;
	}

    if (light && light->getShadows())
    {
        // create an empty shadow map if necessary
        if (!shadowMap)
            shadowMap.reset(new LightShadowMap);

        // get new light size; note that we always use the full radius even for spot lights
        // this is necessary to minimize changes on light rotation
        // we need to ceil and add 1 to account for worst-case in terms of number of intersected voxels
        unsigned int lightSize = G3D::iCeil(getLightRange(light.get()) / 4.f) * 2 + 1;

        // clamp max size to handle FP issues
        if (lightSize > 31)
        {
            RBXASSERT(false);
            lightSize = 31;
        }

        // resize each projection of shadow map
        if (lightSize != shadowMap->size)
        {
            resizeShadowProjection(shadowMap->sliceX.data, shadowMap->size, lightSize);
            resizeShadowProjection(shadowMap->sliceY0.data, shadowMap->size, lightSize);
            resizeShadowProjection(shadowMap->sliceY1.data, shadowMap->size, lightSize);
            resizeShadowProjection(shadowMap->sliceZ.data, shadowMap->size, lightSize);

            shadowMap->size = lightSize;
        }
    }
    else
    {
        shadowMap.reset();
    }

    dirty = false;
}

void LightObject::invalidateLighting(const Extents& bbox)
{
    if (!bbox.isNull())
    {
        getVisualEngine()->getSceneUpdater()->lightingInvalidateLocal(bbox);
    }
}

const Extents& LightObject::getExtents() const
{
    return getWorldBounds();
}

}
}
