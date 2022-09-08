/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/SteppedInstance.h"
#include "Util/RunStateOwner.h"
#include "Util/G3DCore.h"
#include "GfxBase/IAdornable.h"
#include "V8DataModel/Effect.h"

namespace RBX {
	
	class Workspace;
	class Primitive;
	class PartInstance;
    class MegaClusterInstance;

	extern const char* const sExplosion;
	class Explosion : public DescribedCreatable<Explosion, Instance, sExplosion>
					, public IAdornable // this is only here for G3D
					, public IStepped
					, public Diagnostics::Countable<Explosion>
					, public Effect
	{
	private:
		typedef DescribedCreatable<Explosion, Instance, sExplosion> Super;
		Vector3 position;
		float blastRadius;
		float blastPressure;	// RBX Force / RBX^2 approximately
		float destroyJointRadiusPercent;

		float renderTime() const			{return 0.10f;}	// seconds

		float killRadius() const;
		float blastMaxObjectRadius() const;		// biggest object that can be blasted
		float killMaxObjectRadius() const;		// biggest object that can be killed

		void doKill();
		void doBlast(MegaClusterInstance* terrain, const std::vector<shared_ptr<PartInstance> >& parts);
		void signalBlast(const std::vector<shared_ptr<PartInstance> >& parts);

		// Instance
		/*override*/ bool askSetParent(const Instance* instance) const;
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider) {
			Super::onServiceProvider(oldProvider, newProvider);
			onServiceProviderIStepped(oldProvider, newProvider);
		}

		// IAdornable
		/*override*/ bool shouldRender3dAdorn() const {return true;}
		/*override*/ void render3dAdorn(Adorn* adorn);

		// IStepped
		/*override*/ void onStepped(const Stepped& event);

	public:
		enum ExplosionType
		{
			NO_EFFECT,
			NO_DEBRIS,
			WITH_DEBRIS
		};

		ExplosionType explosionType;

		ExplosionType getExplosionType() const { return explosionType; }
		void setExplosionType(ExplosionType value);

		float visualRadius() const;
		Explosion();
		virtual ~Explosion();

		rbx::signal<void(shared_ptr<Instance>, float)> hitSignal;

		static Reflection::BoundProp<Vector3> propPosition;
		static Reflection::BoundProp<float> propBlastPressure;

		// C++ only - use for internal tools
		void setVisualOnly() {
			propBlastPressure.setValue(this, 0.0f);
		}

		void setBlastRadius(float _blastRadius);
		float getBlastRadius() const		{return blastRadius;}
		const Vector3& getPosition() const { return position; }

		void setDestroyJoints(float _destroyJointRadiusPercent);
		float getDestroyJoints() const		{return destroyJointRadiusPercent;}

	};

}	// namespace RBX
