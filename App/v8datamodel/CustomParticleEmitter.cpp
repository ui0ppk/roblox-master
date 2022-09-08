#include "stdafx.h"
#include "v8datamodel/CustomParticleEmitter.h"
#include "Util/RobloxGoogleAnalytics.h"

#define CAT_VIS category_Appearance
#define CAT_EMISSION "Emission"
#define CAT_MOTION "Motion"
#define CAT_PARTICLE_BEHAVIOR "Particles"

DYNAMIC_FASTFLAGVARIABLE(CustomEmitterInstanceEnabled, false)
DYNAMIC_FASTFLAGVARIABLE(EnableParticleDrag, false)

namespace RBX
{

    const char* const sParticleEmitter = "ParticleEmitter";

    CustomParticleEmitter::CustomParticleEmitter()
    : DescribedCreatable<CustomParticleEmitter, Instance, sParticleEmitter>("ParticleEmitter")
    , texture("rbxasset://textures/particles/sparkles_main.dds")
    , color( Color3(1,1,1) )
    , transparency(0)
    , size( 1 )
    , lightEmission(0)
    , enabled(true)
    , rate( 20 )
    , speed( 5, 5 )
    , spread( 0 )
    , rotation(0, 0)
    , rotSpeed(0, 0)
    , lifetime(5, 10)
    , accel(0, 0, 0)
    , zOffset(0)
	, lockedToLocalSpace(false)
	, dampening(0)
	, velocityInheritance(0)
	, emissionDirection(RBX::NORM_Y)
    {
    }

    CustomParticleEmitter::~CustomParticleEmitter()
    {
    }


    REFLECTION_BEGIN();
    //static Reflection::BoundProp<> prop_( "", CAT_VIS, &ParticleEmitter::, &ParticleEmitter:: );

    Reflection::PropDescriptor<CustomParticleEmitter, TextureId>     CustomParticleEmitter::prop_texture( "Texture", CAT_VIS, &CustomParticleEmitter::getTexture, &CustomParticleEmitter::setTexture );
    Reflection::PropDescriptor<CustomParticleEmitter, ColorSequence> CustomParticleEmitter::prop_color( "Color", CAT_VIS, &CustomParticleEmitter::getColor, &CustomParticleEmitter::setColor );
    Reflection::PropDescriptor<CustomParticleEmitter, NumberSequence> CustomParticleEmitter::prop_transp( "Transparency", CAT_VIS, &CustomParticleEmitter::getTransparency, &CustomParticleEmitter::setTransparency );
    Reflection::PropDescriptor<CustomParticleEmitter, NumberSequence> CustomParticleEmitter::prop_size( "Size", CAT_VIS, &CustomParticleEmitter::getSize, &CustomParticleEmitter::setSize );

    Reflection::PropDescriptor<CustomParticleEmitter, bool>          CustomParticleEmitter::prop_enabled("Enabled", CAT_EMISSION, &CustomParticleEmitter::getEnabled, &CustomParticleEmitter::setEnabled);
    Reflection::PropDescriptor<CustomParticleEmitter, float>         CustomParticleEmitter::prop_lightEmission("LightEmission", CAT_VIS, &CustomParticleEmitter::getLightEmission, &CustomParticleEmitter::setLightEmission);
    Reflection::PropDescriptor<CustomParticleEmitter, float>         CustomParticleEmitter::prop_rate("Rate", CAT_EMISSION, &CustomParticleEmitter::getRate, &CustomParticleEmitter::setRate);
    Reflection::PropDescriptor<CustomParticleEmitter, NumberRange>   CustomParticleEmitter::prop_speed("Speed", CAT_EMISSION, &CustomParticleEmitter::getSpeed, &CustomParticleEmitter::setSpeed);
    Reflection::PropDescriptor<CustomParticleEmitter, float>         CustomParticleEmitter::prop_spread("VelocitySpread", CAT_EMISSION, &CustomParticleEmitter::getSpread, &CustomParticleEmitter::setSpread);
    Reflection::PropDescriptor<CustomParticleEmitter, NumberRange>   CustomParticleEmitter::prop_rotation("Rotation", CAT_EMISSION, &CustomParticleEmitter::getRotation, &CustomParticleEmitter::setRotation);
    Reflection::PropDescriptor<CustomParticleEmitter, NumberRange>   CustomParticleEmitter::prop_rotSpeed("RotSpeed", CAT_EMISSION, &CustomParticleEmitter::getRotSpeed, &CustomParticleEmitter::setRotSpeed);
    Reflection::PropDescriptor<CustomParticleEmitter, NumberRange>   CustomParticleEmitter::prop_lifetime("Lifetime", CAT_EMISSION, &CustomParticleEmitter::getLifetime, &CustomParticleEmitter::setLifetime);
    Reflection::PropDescriptor<CustomParticleEmitter, Vector3>       CustomParticleEmitter::prop_accel("Acceleration", CAT_MOTION, &CustomParticleEmitter::getAccel, &CustomParticleEmitter::setAccel);
    Reflection::PropDescriptor<CustomParticleEmitter, float>         CustomParticleEmitter::prop_zOffset("ZOffset", CAT_VIS, &CustomParticleEmitter::getZOffset, &CustomParticleEmitter::setZOffset);
	Reflection::PropDescriptor<CustomParticleEmitter, float>		 CustomParticleEmitter::prop_velocityInheritance("VelocityInheritance", CAT_PARTICLE_BEHAVIOR, &CustomParticleEmitter::getVelocityInheritance, &CustomParticleEmitter::setVelocityInheritance);
	Reflection::PropDescriptor<CustomParticleEmitter, float>		 CustomParticleEmitter::prop_dampening("Drag", CAT_PARTICLE_BEHAVIOR, &CustomParticleEmitter::getDampening, &CustomParticleEmitter::setDampening);
	Reflection::PropDescriptor<CustomParticleEmitter, bool>			 CustomParticleEmitter::prop_lockedToLocalSpace("LockedToPart", CAT_PARTICLE_BEHAVIOR, &CustomParticleEmitter::getLockedToLocalSpace, &CustomParticleEmitter::setLockedToLocalSpace);
	Reflection::EnumPropDescriptor<CustomParticleEmitter, NormalId>  CustomParticleEmitter::prop_emissionDirection("EmissionDirection", CAT_EMISSION, &CustomParticleEmitter::getEmissionDirection, &CustomParticleEmitter::setEmissionDirection);

	Reflection::BoundFuncDesc<CustomParticleEmitter, void(int)>		 CustomParticleEmitter::desc_burst(&CustomParticleEmitter::requestBurst, "Emit", "particleCount", 16, Security::None);

	Reflection::RemoteEventDesc<CustomParticleEmitter, void(int)> CustomParticleEmitter::event_onEmitRequested(&CustomParticleEmitter::onEmitRequested, "OnEmitRequested", "particleCount", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::BROADCAST);
    REFLECTION_END();


    void CustomParticleEmitter::setEnabled(bool v)
    {
        if (enabled == v) return;
        enabled = v;
        raisePropertyChanged(prop_enabled);
    }

    void CustomParticleEmitter::setLightEmission(float v)
    {
        if (lightEmission == v) return;
        lightEmission = v;
        raisePropertyChanged(prop_lightEmission);
    }

    void CustomParticleEmitter::setRate(float v)
    {
        if (rate == v) return;
        rate = v;
        raisePropertyChanged(prop_rate);
    }

    void CustomParticleEmitter::setSpeed(const NumberRange& v)
    {
        if (speed == v) return;
        speed = v;
        raisePropertyChanged(prop_speed);
    }

    void CustomParticleEmitter::setSpread(float v)
    {
        if (spread == v) return;
        spread = v;
        raisePropertyChanged(prop_spread);
    }

    void CustomParticleEmitter::setRotation(const NumberRange& v)
    {
        if (rotation == v) return;
        rotation = v;
        raisePropertyChanged(prop_rotation);
    }

    void CustomParticleEmitter::setRotSpeed(const NumberRange& v)
    {
        if (rotSpeed == v) return;
        rotSpeed = v;
        raisePropertyChanged(prop_rotSpeed);
    }

    void CustomParticleEmitter::setLifetime(const NumberRange& v)
    {
        if (lifetime == v) return;
        lifetime = v;
        raisePropertyChanged(prop_lifetime);
    }

    void CustomParticleEmitter::setAccel(const Vector3& v)
    {
        if (accel == v) return;
        accel = v;
        raisePropertyChanged(prop_accel);
    }

    void CustomParticleEmitter::setZOffset(float v)
    {
        if (zOffset == v) return;
        zOffset = v;
        raisePropertyChanged(prop_zOffset);
    }

    const TextureId& CustomParticleEmitter::getTexture() const { return texture; }

    void CustomParticleEmitter::setTexture(const TextureId& id)
    {
        if (texture == id) return;
        texture = id;
        raisePropertyChanged( prop_texture );
    }

    const ColorSequence& CustomParticleEmitter::getColor() const { return color; }

    void CustomParticleEmitter::setColor(const ColorSequence& val)
    {
        if (color == val) return;
        color = val;
        raisePropertyChanged( prop_color );
    }

    const NumberSequence& CustomParticleEmitter::getTransparency() const { return transparency; }

    void CustomParticleEmitter::setTransparency(const NumberSequence& v)
    {
        if (transparency == v) return;
        transparency = v;
        raisePropertyChanged( prop_transp );
    }

    const NumberSequence& CustomParticleEmitter::getSize() const { return size; }

    void CustomParticleEmitter::setSize(const NumberSequence& val) 
    {
        if (size == val) return;
        size = val;
        raisePropertyChanged(prop_size);
    }

	void CustomParticleEmitter::setVelocityInheritance(float value)
	{
		if (value != velocityInheritance)
		{
			velocityInheritance = value;
			raisePropertyChanged(prop_velocityInheritance);
		}
	}

	void CustomParticleEmitter::setDampening(float value)
	{
		if (!DFFlag::EnableParticleDrag)
		{
			raisePropertyChanged(prop_dampening);
			return;
		}

		if (value != dampening)
		{
			dampening = value;
			raisePropertyChanged(prop_dampening);
		}
	}

	void CustomParticleEmitter::setLockedToLocalSpace(bool value)
	{
		if (value != lockedToLocalSpace)
		{
			lockedToLocalSpace = value;
			raisePropertyChanged(prop_lockedToLocalSpace);
		}
	}

	void CustomParticleEmitter::setEmissionDirection(NormalId value)
	{
		if (value != emissionDirection)
		{
			emissionDirection = value;
			raisePropertyChanged(prop_emissionDirection);
		}
	}

	void CustomParticleEmitter::requestBurst(int value)
	{
			event_onEmitRequested.fireAndReplicateEvent(this, value);
		}


    static void trackCreation()
    {
        RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "CustomParticleEmitter");
    }

    void CustomParticleEmitter::onAncestorChanged(const AncestorChanged& ev)
    {
        Base::onAncestorChanged(ev);
        static boost::once_flag flag = BOOST_ONCE_INIT;
        boost::call_once(&trackCreation, flag );
    }
}
