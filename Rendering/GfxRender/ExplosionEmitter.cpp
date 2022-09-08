#include "stdafx.h"

#include "ExplosionEmitter.h"

#include "V8DataModel/PartInstance.h"
#include "v8datamodel/Explosion.h"

#include "SceneUpdater.h"
#include "Util.h"

#include "Emitter.h"
#include "TextureManager.h"

#include "VisualEngine.h"

FASTFLAG(RenderNewParticles2Enable);

namespace RBX
{
namespace Graphics
{

ExplosionEmitter::ExplosionEmitter(VisualEngine* visualEngine)
    : Super(visualEngine, CullMode_SpatialHash)
    , dirty(false)
    , enabled(false)
    , localBox(Vector3::zero(), Vector3::zero())
{
}

ExplosionEmitter::~ExplosionEmitter()
{
    unbind();

    // notify scene updater about destruction so that the pointer to ExplosionEmitter is no longer stored
    getVisualEngine()->getSceneUpdater()->notifyDestroyed(this);
}

void ExplosionEmitter::onSleepingChangedEx(bool sleeping)
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

void ExplosionEmitter::updateCoordinateFrame(bool recalcLocalBounds)
{
    CoordinateFrame frame = part ? part->calcRenderingCoordinateFrame() : CoordinateFrame();

    if (RBX::Explosion* expl = Instance::fastDynamicCast<RBX::Explosion>(effect.get()))
    {
        frame.rotation = Matrix3::identity();
        frame.translation = expl->getPosition();
    }
    
    if (!dirty && transform.fuzzyEq(frame))
    {
        // Nothing to update
        return;
    }
    
    transform = frame;

    for (int j=0; j<Num_Emitters; ++j)
    {
        if (emitters[j])
            emitters[j]->transform() = transform;
    }

    // Update world-space extents
    updateWorldBounds(localBox.toWorldSpace(transform));
}

void ExplosionEmitter::onCombinedSignalEx(Instance::CombinedSignalType type, const Instance::ICombinedSignalData* data)
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

void ExplosionEmitter::onPropertyChangedEx(const RBX::Reflection::PropertyDescriptor* descriptor)
{
    invalidateEntity();
}

void ExplosionEmitter::onAncestorChangedEx()
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

void ExplosionEmitter::bind(const shared_ptr<RBX::PartInstance>& part, const shared_ptr<RBX::Instance>& instance)
{
    RBXASSERT(!this->part && !this->effect);
    RBXASSERT(instance);
    
    this->part = part;
    this->effect = instance;

    connections.push_back(instance->combinedSignal.connect(boost::bind(&ExplosionEmitter::onCombinedSignalEx, this, _1, _2)));

    if (part)
    {
        connections.push_back(part->onDemandWrite()->sleepingChangedSignal.connect(boost::bind(&ExplosionEmitter::onSleepingChangedEx, this, _1)));
        
        // we just connected, so sync up the state.
        onSleepingChangedEx(part->getSleeping());
    }
    
    invalidateEntity();
}

void ExplosionEmitter::unbind()
{
    Super::unbind();

    part.reset();
    effect.reset();
}

void ExplosionEmitter::invalidateEntity()
{
    if (!dirty)
    {
        dirty = true;
        
        getVisualEngine()->getSceneUpdater()->queueInvalidatePart(this);
    }
}

void ExplosionEmitter::updateEntity(bool assetsUpdated)
{
    if (connections.empty()) // zombified.
    {
        getVisualEngine()->getSceneUpdater()->destroyAttachment(this);
        return;
    }
    
    applySettings();
    updateCoordinateFrame(true);

    dirty = false;
}

void ExplosionEmitter::updateRenderQueue(RenderQueue& queue, const RenderCamera& camera, RenderQueue::Pass pass)
{
    for (int j=0; j<Num_Emitters; ++j)
    {
        if (emitters[j])
            emitters[j]->draw(queue);
    }

    // Render bounding box
    if (getVisualEngine()->getSettings()->getDebugShowBoundingBoxes())
        debugRenderBoundingBox();
}

void ExplosionEmitter::applySettings()
{
    if (!effect) return;

    static const Vector2 zz(0,0);
    static const Vector3 zzz(0,0,0);
    static const Vector4 zzzz(0,0,0,0);

    static const Vector2 cc(1,1);
    static const Vector3 ccc(1,1,1);
    static const Vector4 cccc(1,1,1,1);
    
    static const Vector2 nc(-1,1);
    
    CoordinateFrame initCframe = static_cast<RBX::Explosion*>(effect.get())->getPosition();

    enabled = true;
    
    if (1) // core
    {
        scoped_ptr<Emitter>& emitter = emitters[kCore];
        if (enabled && !emitter)
            emitter.reset(new Emitter(getVisualEngine()));

        Emitter::Appearance a = {};
        a.shader = Emitter::Shader_Crazy;
        a.blendCode = Emitter::Blend_PremultipliedAlpha;
        a.alphaStripTexture = "rbxasset://textures/particles/explosion01_core_alpha.png";
        a.colorStripTexture = "rbxasset://textures/particles/explosion_color.dds";
        a.mainTexture       = "rbxasset://textures/particles/explosion01_core_main.dds";
        a.colorStripBaseline = -1;

        emitter->setAppearance( a );
        emitter->setBlendRatio(0.2f);
        emitter->setModulateColor(cccc);

        emitter->setDampening( 4 );
        emitter->setGlobalForce( zzz );
        emitter->setGrowth( 2.0f * cc );
        emitter->setLocalForce( zzz );
        emitter->setRotation( -400/57.3f * nc );
        emitter->setSizeX( 5.0f * cc );
        emitter->setSizeY( 5.0f * cc );
        emitter->setSpeed( 20.0f * cc );
        emitter->setSpin( -90/57.3f * nc );
        emitter->setSpread( 3.1415926f * cc );

        emitter->setEmissionRate(0);
        emitter->setEmitterShape(0, Box( -ccc, ccc) );
        emitter->setLife(Vector2(0.5f,0.7f));
        
        emitter->transform() = initCframe;
        emitter->emit(100);
    }

    if (1) // chunks
    {
        scoped_ptr<Emitter>& emitter = emitters[kChunks];
        if (enabled && !emitter)
            emitter.reset(new Emitter(getVisualEngine()));

        Emitter::Appearance a = {};
        a.shader = Emitter::Shader_Crazy;
        a.blendCode = Emitter::Blend_PremultipliedAlpha;
        a.alphaStripTexture = "rbxasset://textures/particles/explosion01_core_alpha.png";
        a.colorStripTexture = "rbxasset://textures/particles/explosion_color.dds";
        a.mainTexture       = "rbxasset://textures/particles/explosion01_core_main.dds";
        a.colorStripBaseline = -1;

        emitter->setAppearance( a );
        emitter->setBlendRatio(0.15f);
        emitter->setModulateColor(cccc);
        emitter->setZOffset(0.01f);

        emitter->setDampening( 2 );
        emitter->setGlobalForce( zzz );
        emitter->setGrowth( -2.0f * cc );
        emitter->setLocalForce( Vector3(0,23,0) );
        emitter->setRotation( -100/57.3f * nc );
        emitter->setSizeX( 1.3f * cc );
        emitter->setSizeY( 1.3f * cc );
        emitter->setSpeed( Vector2(60,80) );
        emitter->setSpin( zz );
        emitter->setSpread( 3.1415926f * cc );

        emitter->setEmissionRate(0);
        emitter->setEmitterShape(0, Box( -ccc, ccc) );
        emitter->setLife(Vector2(1,1));

        emitter->transform() = initCframe;
        emitter->emit(50);
    }

    if (1) // shockwave
    {
        scoped_ptr<Emitter>& emitter = emitters[kShockwave];
        if (enabled && !emitter)
            emitter.reset(new Emitter(getVisualEngine()));

        Emitter::Appearance a = {};
        a.shader = Emitter::Shader_Add;
        a.blendCode = Emitter::Blend_AlphaOne;
        a.alphaStripTexture = "rbxasset://textures/particles/explosion01_core_alpha.png";
        a.colorStripTexture = "rbxasset://textures/particles/smoke_color.dds";
        a.mainTexture       = "rbxasset://textures/particles/explosion01_shockwave_main.dds";
        a.colorStripBaseline = 0;

        emitter->setAppearance( a );
        emitter->setBlendRatio(0);
        emitter->setModulateColor(cccc);

        emitter->setDampening( 0 );
        emitter->setGlobalForce( zzz );
        emitter->setGrowth( 200.0f * cc );
        emitter->setLocalForce( zzz );
        emitter->setRotation( -100/57.3f * nc );
        emitter->setSizeX( 0.0f * cc );
        emitter->setSizeY( 0.0f * cc );
        emitter->setSpeed( zz );
        emitter->setSpin( zz );
        emitter->setSpread( zz );

        emitter->setEmissionRate(0);
        emitter->setEmitterShape(0, Box( -zzz, zzz) );
        emitter->setLife(Vector2(0.5f,0.7f));

        emitter->transform() = initCframe;
        emitter->emit(3);
    }

    if (1) // smoke
    {
        scoped_ptr<Emitter>& emitter = emitters[kSmoke];
        if (enabled && !emitter)
            emitter.reset(new Emitter(getVisualEngine()));

        Emitter::Appearance a = {};
        a.shader = Emitter::Shader_Modulate;
        a.blendCode = Emitter::Blend_AlphaBlend;
        a.alphaStripTexture = "rbxasset://textures/particles/explosion01_smoke_alpha.dds";
        a.colorStripTexture = "rbxasset://textures/particles/explosion01_smoke_color_new.dds";
        a.mainTexture       = "rbxasset://textures/particles/explosion01_core_main.dds";
        a.colorStripBaseline = 0;

        emitter->setAppearance( a );
        emitter->setBlendRatio(0);
        emitter->setModulateColor(cccc);
        emitter->setZOffset( 0.02f);

        emitter->setDampening( 2 );
        emitter->setGlobalForce( zzz );
        emitter->setGrowth( 1.0f * cc );
        emitter->setLocalForce( Vector3(0,10,0) );
        emitter->setRotation( -200/57.3f * nc );
        emitter->setSizeX( 6.0f * cc );
        emitter->setSizeY( 6.0f * cc );
        emitter->setSpeed( Vector2(5,20) );
        emitter->setSpin( 30.0f / 57.3f * nc );
        emitter->setSpread( 3.1415926f * cc );

        emitter->setEmissionRate(0);
        emitter->setEmitterShape(0, Box( -zzz, zzz) );
        emitter->setLife(Vector2(1,2));

        emitter->transform() = initCframe;
        emitter->emit(60);
    }

    if (1) // implosion/flash
    {
        scoped_ptr<Emitter>& emitter = emitters[kImplosion];
        if (enabled && !emitter)
            emitter.reset(new Emitter(getVisualEngine()));

        Emitter::Appearance a = {};
        a.shader = Emitter::Shader_Add;
        a.blendCode = Emitter::Blend_Additive;
        a.alphaStripTexture = "rbxasset://textures/particles/explosion01_core_alpha.png";
        a.colorStripTexture = "rbxasset://textures/particles/explosion01_implosion_color.png";
        a.mainTexture       = "rbxasset://textures/particles/explosion01_implosion_main.dds";
        a.colorStripBaseline = 0;

        emitter->setAppearance( a );
        emitter->setBlendRatio(0);
        emitter->setModulateColor(cccc);
        emitter->setZOffset(-0.02f);

        emitter->setDampening( 0 );
        emitter->setGlobalForce( zzz );
        emitter->setGrowth( 14.0f * cc );
        emitter->setLocalForce( zzz );
        emitter->setRotation( -300/57.3f * nc );
        emitter->setSizeX( 20.0f * cc );
        emitter->setSizeY( 20.0f * cc );
        emitter->setSpeed( Vector2(20,20) );
        emitter->setSpin( zz );
        emitter->setSpread( zz );

        emitter->setEmissionRate(0);
        emitter->setEmitterShape(0, Box( -zzz, zzz) );
        emitter->setLife(Vector2(0.1f,0.3f));

        emitter->transform() = initCframe;
        emitter->emit(2);
    }

    localBox = Extents();

    for (int j=0; j<Num_Emitters; ++j)
        if (emitters[j])
            localBox.expandToContain(emitters[j]->computeBBox());

}

}
}
