#include "stdafx.h"

#include "CustomEmitter.h"

#include "V8DataModel/PartInstance.h"
#include "v8datamodel/CustomParticleEmitter.h"

#include "SceneUpdater.h"
#include "Util.h"

#include "Emitter.h"
#include "TextureManager.h"

#include "VisualEngine.h"

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

namespace RBX
{
namespace Graphics
{

CustomEmitter::CustomEmitter(VisualEngine* visualEngine)
    : Super(visualEngine, CullMode_SpatialHash)
    , dirtyFlags(0)
    , enabled(false)
    , localBox(Vector3::zero(), Vector3::zero())
	, requestedEmitCount(0)
{
}

CustomEmitter::~CustomEmitter()
{
    unbind();

    // notify scene updater about destruction so that the pointer to CustomEmitter is no longer stored
    getVisualEngine()->getSceneUpdater()->notifyDestroyed(this);
}

void CustomEmitter::onSleepingChangedEx(bool sleeping)
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

void CustomEmitter::updateCoordinateFrame(bool recalcLocalBounds)
{
    CoordinateFrame frame = part ? part->calcRenderingCoordinateFrame() : CoordinateFrame();

    if ( (dirtyFlags & kDirty_Transform) == 0 && transform.fuzzyEq(frame))
    {
        return; // Nothing to update
    }
    
    transform = frame;
    if (emitter)
    {
        emitter->transform() = transform;
        localBox = emitter->computeBBox();
        updateWorldBounds(localBox.toWorldSpace(transform));

		if (part)
			emitter->setVelocity(part->getVelocity());
		else
			emitter->setVelocity(Velocity());
    }
}

void CustomEmitter::onCombinedSignalEx(Instance::CombinedSignalType type, const Instance::ICombinedSignalData* data)
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

void CustomEmitter::onAncestorChangedEx()
{
    shared_ptr<Instance> effectCopy = effect;
    
    // Remove me from the scene if I am being removed from the Workspace
    if (!isInWorkspace(effectCopy.get()))
    {
        // will cause a delete on next updateEntity()
        zombify();
    }
    else
    {
        unbind();
        
        RBX::PartInstance* parent = RBX::Instance::fastDynamicCast<RBX::PartInstance>(effectCopy->getParent());
        shared_ptr<RBX::PartInstance> part = shared_from(parent);
            
        bind(part, effectCopy);
    }
}

void CustomEmitter::bind(const shared_ptr<RBX::PartInstance>& part, const shared_ptr<RBX::Instance>& instance)
{
    RBXASSERT(!this->part && !this->effect);
    this->part = part;
    this->effect = shared_from(instance->fastDynamicCast<RBX::CustomParticleEmitter>());

    //

    if (part)
    {
        connections.push_back(part->onDemandWrite()->sleepingChangedSignal.connect(boost::bind(&CustomEmitter::onSleepingChangedEx, this, _1)));
        
        // we just connected, so sync up the state.
        onSleepingChangedEx(part->getSleeping());
    }

    if (effect)
    {
		connections.push_back(this->effect->onEmitRequested.connect(boost::bind(&CustomEmitter::requestEmit, this, _1)));
        connections.push_back(instance->combinedSignal.connect(boost::bind(&CustomEmitter::onCombinedSignalEx, this, _1, _2)));
        
        if(part)
        {
             connections.push_back( part->propertyChangedSignal.connect( boost::bind(&CustomEmitter::onParentSize, this, _1)) );
		}
    }

    dirtyFlags = kDirty_All;
    invalidateEntity();
}

void CustomEmitter::unbind()
{
    Super::unbind();
    part.reset();
    effect.reset();
}

void CustomEmitter::requestEmit(int particleCount)
{
	requestedEmitCount += particleCount;

	dirtyFlags |= kDirty_Fast;
	invalidateEntity();
}

void CustomEmitter::invalidateEntity()
{
    if ((dirtyFlags & kDirty_Invalidate) == 0)
    {
        dirtyFlags |= kDirty_Invalidate; // prevent repeated submissions
        getVisualEngine()->getSceneUpdater()->queueInvalidatePart(this);
    }
}

void CustomEmitter::updateEntity(bool assetsUpdated)
{
    if (connections.empty()) // zombified.
    {
        getVisualEngine()->getSceneUpdater()->destroyAttachment(this);
        return;
    }
    
    applyAllSettings();
    updateCoordinateFrame(true);

    dirtyFlags = kDirty_None;
}

void CustomEmitter::updateRenderQueue(RenderQueue& queue, const RenderCamera& camera, RenderQueue::Pass pass)
{
    if (!enabled && emitter && !emitter->pcount()) 
        emitter.reset();

    if (emitter)
	{
        emitter->draw(queue);
	}

    // Render bounding box
    if (getVisualEngine()->getSettings()->getDebugShowBoundingBoxes())
        debugRenderBoundingBox();
}

static inline Vector2 v2(const NumberRange& r) { return Vector2(r.min, r.max); }

void CustomEmitter::applyAllSettings()
{
    if (!effect) return;

    if( !emitter )
    {
        emitter.reset( new Emitter(getVisualEngine(), true, effect->getFullName() + ".Texture") );
        dirtyFlags = kDirty_All;
    }

    static const Vector2 zz(0,0);
    static const Vector3 zzz(0,0,0);
    static const Vector4 zzzz(0,0,0,0);
    static const Vector2 cc(1,1);

    if (dirtyFlags & kDirty_Fast)
    {
        Vector3 boxsize(1,1,1);

        if( part )
        {
            boxsize = 0.5f * part->getRenderSize();
        }

        enabled = effect->getEnabled();

        emitter->setBlendRatio(1.0f - effect->getLightEmission());
        emitter->setEmissionRate( effect->getRate() * effect->getEnabled() );
        emitter->setSpeed( v2(effect->getSpeed()) );
        emitter->setSpread( cc * effect->getSpread() / 180 * M_PI );
        emitter->setRotation( v2(effect->getRotation()) / 180 * M_PI );
        emitter->setSpin( v2(effect->getRotSpeed()) / 180 * M_PI );
        emitter->setZOffset(effect->getZOffset());
        emitter->setLife( v2(effect->getLifetime()) );
        emitter->setGlobalForce(effect->getAccel());

        emitter->setEmitterShape(0, Box(-boxsize, boxsize) );

		emitter->setDampening(effect->getDampening());
		emitter->setLockedToLocalSpace(effect->getLockedToLocalSpace());

		emitter->setVelocityInheritance(effect->getVelocityInheritance());

        localBox = emitter->computeBBox();
        updateWorldBounds(localBox.toWorldSpace(transform));

		//calculate the spherical direction
		Vector2 sphericalDirection;
		switch(effect->getEmissionDirection())
		{
		case RBX::NORM_Y:
			sphericalDirection = Vector2(M_PI / 2, M_PI / 2);
			break;
		case RBX::NORM_Y_NEG:
			sphericalDirection = Vector2(-M_PI / 2, M_PI / 2);
			break;
		case RBX::NORM_X:
			sphericalDirection = Vector2(0, M_PI / 2);
			break;
		case RBX::NORM_X_NEG:
			sphericalDirection = Vector2(0, -M_PI / 2);
			break;
		case RBX::NORM_Z_NEG:
			sphericalDirection = Vector2(0, M_PI);
			break;
		case RBX::NORM_Z:
			sphericalDirection = Vector2(0, 0);
			break;
		default:
			break;
		}
		emitter->setSphericalDirection(sphericalDirection);

		//emit!
		if (requestedEmitCount > 0)
		{
			updateCoordinateFrame(true);
			emitter->emit(requestedEmitCount);
			requestedEmitCount = 0;
		}
    }

    if (dirtyFlags & kDirty_Curves)
    {
        emitter->setColorCurve( &effect->getColor() );
        emitter->setAlphaCurve( &effect->getTransparency() );
        emitter->setSizeCurve( &effect->getSize() );
        localBox = emitter->computeBBox();
        updateWorldBounds(localBox.toWorldSpace(transform));
    }

    if (dirtyFlags & kDirty_Slow)
    {
        Emitter::Appearance a = {};
        a.alphaStripTexture = "";
        a.colorStripTexture = "";
        a.mainTexture       = effect->getTexture().c_str();
        a.colorStripBaseline = -1;
        a.shader = Emitter::Shader_Custom;
        a.blendCode = Emitter::Blend_PremultipliedAlpha;
        emitter->setAppearance( a, effect->getFullName() + ".Texture" );
    }
}

void CustomEmitter::onPropertyChangedEx(const RBX::Reflection::PropertyDescriptor* p)
{
    if (p == &CustomParticleEmitter::prop_texture)
    {
        dirtyFlags |= kDirty_Slow;
        invalidateEntity();
        return;
    }

    if (p == &CustomParticleEmitter::prop_color)
    {
        dirtyFlags |= kDirty_Curves;
        invalidateEntity();
        return;
    }

    if (p == &CustomParticleEmitter::prop_transp)
    {
        dirtyFlags |= kDirty_Curves;
        invalidateEntity();
        return;
    }

    if (p == &CustomParticleEmitter::prop_size)
    {
        dirtyFlags |= kDirty_Curves;
        invalidateEntity();
        return;
    }

    dirtyFlags |= kDirty_Fast;
    invalidateEntity();
}

void CustomEmitter::onParentSize(pd p)
{
    if (p == &PartInstance::prop_Size)
    {
        dirtyFlags |= kDirty_Fast;
        invalidateEntity();
    }
}

}
}
