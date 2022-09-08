/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/BasicPartInstance.h"
#include "V8DataModel/ActionStation.h"
#include "Network/Players.h"

namespace RBX {

class Humanoid;
class Motor6D;

template<class Base>
class PlatformImpl : public ActionStation<Base>
{
	private:
		typedef ActionStation<Base> Super;
		// "Backend" connections
		rbx::signals::scoped_connection platformTouched;
		rbx::signals::scoped_connection humanoidDonePlatformStanding;		

		void onEvent_platformTouched(shared_ptr<Instance> other) {
			if (Humanoid* h = Humanoid::humanoidFromBodyPart(other.get())) {
				if (PartInstance* torso = h->getTorsoSlow()) {
					if (this->sleepTimeUp() 
						&&	!findPlatformMotor6D()
						&&  !h->getPlatformStanding()
						&&  !h->getDead()
						&&	!torso->computeNetworkOwnerIsSomeoneElse()
						&&	Workspace::contextInWorkspace(torso)
						&&	Workspace::contextInWorkspace(this)) 
					{
						RBXASSERT(h->getTorsoSlow()->getPartPrimitive()->getAssembly() != this->getPartPrimitive()->getAssembly());
						if(this->getCoordinateFrame().rotation.column(1).y > 0.7f)			// must be within 45 degrees of upright to mount
						{
							createPlatformMotor6D(h);
						}
						else
						{
							// this doesn't cut it. disable and do in lua for now.
							/*// kick it, up and away from torso.
							Vector3 torsopos = torso->getCoordinateFrame().translation;
							Vector3 ppos = getCoordinateFrame().translation;
							Vector3 dirv = ppos - torsopos;
							dirv.y = 0; // neutralize up.
							dirv.unitize();
							static float away = 3.0f;
							static float up = 30.0f;
							Vector3 upimpulse = Vector3(0, up, 0);
							applySpecificImpulse(upimpulse + dirv * away, torso->getCoordinateFrame().translation);
							*/
						}
					}
				}
			}
		}

		// Destroy platform - on event no longer platformed from humanoid state (simulator)
		void onEvent_humanoidDonePlatformStanding() {
			findAndDestroyPlatformMotor6D();
		}
		
		void createPlatformMotor6DInternal(Humanoid *h)
		{
			if (!h->getTorsoSlow())
				return;

			PartInstance *pTorsoPart = NULL; 
			pTorsoPart = h->getVisibleTorsoSlow();
			RBXASSERT(pTorsoPart); // we just checked for this

			CoordinateFrame originalPlatformCoord = this->getCoordinateFrame();
			RBXASSERT(this->getCoordinateFrame() == this->getPartPrimitive()->getCoordinateFrame());

			CoordinateFrame platformCoord;
			platformCoord.lookAt(Vector3(0,-1,0), Vector3(0,0,-1));
			platformCoord.translation = Vector3(0, this->getPartSizeXml().y *0.5f, 0);				// at the surface.

			CoordinateFrame torsoCoord;
			torsoCoord.lookAt(Vector3(0,-1,0), Vector3(0,0,-1));
			torsoCoord.translation = Vector3(0, -3.0, 0);

			Primitive* torso = pTorsoPart->getPartPrimitive();
			Assembly* torsoAssembly = torso->getAssembly();
			if (!torsoAssembly)
				return;

			RBXASSERT(torsoAssembly != this->getPartPrimitive()->getAssembly());
			Primitive* root = torsoAssembly->getAssemblyPrimitive();

			root->setVelocity(Velocity::zero());

			shared_ptr<Motor6D> tempMotor6D = Creatable<Instance>::create<Motor6D>();

			tempMotor6D->setName("PlatformMotor6D");
			tempMotor6D->setPart0(this);
			tempMotor6D->setPart1(pTorsoPart);

			tempMotor6D->setC0(platformCoord);
			tempMotor6D->setC1(torsoCoord);

			Instance::propArchivable.setValue(tempMotor6D.get(), false); 
			tempMotor6D->setParent(this);
		}

		void onChildRemoved(Instance* child)
		{
			if (Motor6D* childMotor6D = isChildPlatformMotor6D(child)) 
			{
				humanoidDonePlatformStanding.disconnect();
				ActionStation<Base>::sleepTime = Time::now<Time::Fast>();
				onPlatformStandingChanged(false, humanoidFromMotor6D(childMotor6D));
			}
		}

		void onChildAdded(Instance* child) 
		{
			if (Motor6D* childMotor6D = isChildPlatformMotor6D(child)) 
			{
				destroyOtherMotor6Ds(childMotor6D);

				if (Humanoid* h = humanoidFromMotor6D(childMotor6D)) {
					h->setPlatformStanding(true);		// will fire this event
					humanoidDonePlatformStanding = h->donePlatformStandingSignal.connect(boost::bind(&PlatformImpl::onEvent_humanoidDonePlatformStanding, this));
					onPlatformStandingChanged(true, h);
				}
			}
		}

	protected:

		virtual void createPlatformMotor6D(Humanoid *h) {
			createPlatformMotor6DInternal(h);
		}

		virtual void findAndDestroyPlatformMotor6D() {
			findAndDestroyPlatformMotor6DInternal();
		}

		void createPlatformMotor6DInternal(shared_ptr<Instance> instance) {
			createPlatformMotor6DInternal(Instance::fastSharedDynamicCast<Humanoid>(instance).get());
		}

		void findAndDestroyPlatformMotor6DInternal()
		{
			while (Motor6D* motor6D = findPlatformMotor6D()) {
				motor6D->setParent(NULL);
			}
		}

		Humanoid* humanoidFromMotor6D(Motor6D* motor6D) {
			PartInstance* torso = motor6D ? motor6D->getPart1() : NULL;
			return Humanoid::humanoidFromBodyPart(torso);
		}

		Motor6D* findPlatformMotor6D() {
			for (size_t i = 0; i < this->numChildren(); ++i) {
				if (Motor6D* answer = isChildPlatformMotor6D(this->getChild(i))) {
					return answer;
				}
			}
			return NULL;
		}

		Motor6D* isChildPlatformMotor6D(Instance* child) {
			if (child->getName() == "PlatformMotor6D") {
				if (Motor6D* childMotor6D = child->fastDynamicCast<Motor6D>()) {
					return childMotor6D;
				}
			}
			return NULL;
		}

		void destroyOtherMotor6D(shared_ptr<Instance> child, Motor6D* childMotor6D) {
			if ((child.get() != childMotor6D) && isChildPlatformMotor6D(child.get())) {
				child->setParent(NULL);
			}
		}

		void destroyOtherMotor6Ds(Motor6D* childMotor6D) {
			this->visitChildren(boost::bind(&PlatformImpl::destroyOtherMotor6D, this, _1, childMotor6D));
		}
	
		void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider) {
			if(oldProvider) {
				platformTouched.disconnect(); // just to be safe
				onPlatformStandingChanged(false, humanoidFromMotor6D(findPlatformMotor6D()));		// platform is destroyed before motor6D in this case
			}

			Super::onServiceProvider(oldProvider, newProvider);

			if(newProvider) {
				platformTouched = ActionStation<Base>::onDemandWrite()->localSimulationTouchedSignal.connect(boost::bind(&PlatformImpl::onEvent_platformTouched, this, _1));
			}
		}

		/*implement*/ virtual void onPlatformStandingChanged(bool platformed, Humanoid* humanoid) {}

		// implement this if platform is "kickable"
		virtual void applySpecificImpulse(Vector3 impulseWorld, Vector3 worldpos) {};

	public:
};






extern const char* const sPlatform;
// changed to described, we don't ship right now.
// need to fix dismount issue.
class Platform : public Reflection::Described<Platform, sPlatform, PlatformImpl<BasicPartInstance> >
{
	typedef PlatformImpl<BasicPartInstance> Super;
public:
	rbx::remote_signal<void(shared_ptr<Instance>)> createPlatformMotor6DSignal;
	rbx::remote_signal<void()> destroyPlatformMotor6DSignal;

	///////////////////////////////////////////////////////////////////////////
	// PlatformImpl
	/*override*/ void createPlatformMotor6D(Humanoid *h);
	/*override*/ void findAndDestroyPlatformMotor6D();

	Platform() {
		setName(sPlatform);

		createPlatformMotor6DSignal.connect(boost::bind(static_cast<void (Platform::*)(shared_ptr<Instance>)>(&Platform::createPlatformMotor6DInternal), this, _1));
		destroyPlatformMotor6DSignal.connect(boost::bind(&Platform::findAndDestroyPlatformMotor6DInternal, this));
	}
};



} // namespace