#pragma once

#include "util/G3DCore.h"

#include "RenderNode.h"

namespace RBX
{
    class PartInstance;
    class Instance;
}

namespace RBX
{
namespace Graphics
{

class Emitter;

class ExplosionEmitter: public RenderNode
{
    typedef RenderNode Super;

public:
    ExplosionEmitter(VisualEngine* visualEngine);
    ~ExplosionEmitter();

    void bind(const shared_ptr<RBX::PartInstance>& part, const shared_ptr<RBX::Instance>& instance);
    
    // GfxBinding overrides
    virtual void invalidateEntity();
    virtual void updateEntity(bool assetsUpdated);
    virtual void unbind();

    // GfxPart overrides
    virtual void updateCoordinateFrame(bool recalcLocalBounds);

    virtual void updateRenderQueue(RenderQueue& queue, const RenderCamera& camera, RenderQueue::Pass pass);

private:
    void onCombinedSignalEx(Instance::CombinedSignalType type, const Instance::ICombinedSignalData* data);
    void onPropertyChangedEx(const RBX::Reflection::PropertyDescriptor* descriptor);
    void onAncestorChangedEx();
    void onSleepingChangedEx(bool sleeping);

    void applySettings();
    
    shared_ptr<RBX::PartInstance> part;
    shared_ptr<RBX::Instance> effect;
    CoordinateFrame transform;
    Extents localBox;
    bool dirty;
    bool enabled;

    enum { 
        kCore,
        kChunks,
        kShockwave,
        kSmoke,
        kImplosion,

        Num_Emitters,
    };
    scoped_ptr<Emitter> emitters[Num_Emitters];
};

}
}
