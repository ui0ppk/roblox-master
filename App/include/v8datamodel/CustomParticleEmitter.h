#pragma once


#include "V8Tree/Instance.h"
#include "V8DataModel/Effect.h"
#include "V8DataModel/PartInstance.h"
#include "util/TextureId.h"

#include "v8dataModel/NumberSequence.h"
#include "v8datamodel/ColorSequence.h"
#include "v8datamodel/NumberRange.h"


namespace RBX
{
    extern const char* const sParticleEmitter;
    class CustomParticleEmitter : public DescribedCreatable<CustomParticleEmitter, Instance, sParticleEmitter>, public Effect
    {
        typedef DescribedCreatable<CustomParticleEmitter, Instance, sParticleEmitter> Base;
    public:
        CustomParticleEmitter();
        virtual ~CustomParticleEmitter();

        
        
        static Reflection::PropDescriptor<CustomParticleEmitter, TextureId>      prop_texture;
        static Reflection::PropDescriptor<CustomParticleEmitter, ColorSequence>  prop_color;
        static Reflection::PropDescriptor<CustomParticleEmitter, NumberSequence> prop_transp;
        static Reflection::PropDescriptor<CustomParticleEmitter, NumberSequence> prop_size;

        static Reflection::PropDescriptor<CustomParticleEmitter, bool> prop_enabled;
        static Reflection::PropDescriptor<CustomParticleEmitter, float> prop_lightEmission;
        static Reflection::PropDescriptor<CustomParticleEmitter, float> prop_rate;
        static Reflection::PropDescriptor<CustomParticleEmitter, NumberRange> prop_speed;
        static Reflection::PropDescriptor<CustomParticleEmitter, float> prop_spread;
        static Reflection::PropDescriptor<CustomParticleEmitter, NumberRange> prop_rotation;
        static Reflection::PropDescriptor<CustomParticleEmitter, NumberRange> prop_rotSpeed;
        static Reflection::PropDescriptor<CustomParticleEmitter, NumberRange> prop_lifetime;
        static Reflection::PropDescriptor<CustomParticleEmitter, Vector3> prop_accel;
        static Reflection::PropDescriptor<CustomParticleEmitter, float> prop_zOffset;
		static Reflection::PropDescriptor<CustomParticleEmitter, float> prop_velocityInheritance;
		static Reflection::PropDescriptor<CustomParticleEmitter, float> prop_dampening;
		static Reflection::PropDescriptor<CustomParticleEmitter, bool> prop_lockedToLocalSpace;
		static Reflection::EnumPropDescriptor<CustomParticleEmitter, NormalId> prop_emissionDirection;

		static Reflection::RemoteEventDesc<CustomParticleEmitter, void(int)> event_onEmitRequested;

		static Reflection::BoundFuncDesc<CustomParticleEmitter, void(int)> desc_burst;

        bool  getEnabled() const { return enabled; }
        float getLightEmission() const { return lightEmission; }
        float getRate() const { return rate; }
        const NumberRange& getSpeed() const { return speed; }
        float getSpread() const { return spread; }
        const NumberRange& getRotation() const { return rotation; }
        const NumberRange& getRotSpeed() const { return rotSpeed; }
        const NumberRange& getLifetime() const { return lifetime; }
        const Vector3&     getAccel() const { return accel; }
        float getZOffset() const { return zOffset; }

        void setEnabled(bool v);
        void setLightEmission(float v);
        void setRate(float v);
        void setSpeed(const NumberRange& v);
        void setSpread(float v);
        void setRotation(const NumberRange& v);
        void setRotSpeed(const NumberRange& v);
        void setLifetime(const NumberRange& v);
        void setAccel(const Vector3& v);
        void setZOffset(float v);

		rbx::remote_signal<void(int)> onEmitRequested;

        const TextureId& getTexture() const;
        void setTexture(const TextureId& id);

        const NumberSequence& getTransparency() const;
        void setTransparency( const NumberSequence& v );

        const ColorSequence& getColor() const;
        void setColor(const ColorSequence& val);

        const NumberSequence& getSize() const;
        void setSize(const NumberSequence& val);

		float getVelocityInheritance() const { return velocityInheritance; }
		void setVelocityInheritance(float value);

		float getDampening() const { return dampening; }
		void setDampening(float value);

		bool getLockedToLocalSpace() const { return lockedToLocalSpace; }
		void setLockedToLocalSpace(bool value);

		void requestBurst(int value);

		NormalId getEmissionDirection() const { return emissionDirection; }
		void setEmissionDirection(NormalId value);

    private:

        TextureId       texture;
        ColorSequence   color;
        NumberSequence  transparency;
        NumberSequence  size;

        bool            enabled;
        float           lightEmission;
        float           rate;
        NumberRange     speed;
        float           spread;
        NumberRange     rotation;
        NumberRange     rotSpeed;
        NumberRange     lifetime;
        Vector3         accel;
        float           zOffset;
		float			velocityInheritance;
		float			dampening;
		bool			lockedToLocalSpace;

		NormalId		emissionDirection;

        virtual bool askSetParent(const Instance* parent) const {return Instance::fastDynamicCast<PartInstance>(parent) != NULL;}
        virtual bool askAddChild(const Instance* instance) const {return false;}
        virtual void onAncestorChanged(const AncestorChanged& event);
    };

}