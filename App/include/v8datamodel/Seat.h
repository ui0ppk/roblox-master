/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/BasicPartInstance.h"
#include "V8DataModel/ActionStation.h"
#include "Network/Players.h"

DYNAMIC_FASTFLAG(FixAnchoredSeatingPosition)
DYNAMIC_FASTFLAG(FixSeatingWhileSitting)

namespace RBX {

class Humanoid;
class Weld;

template<class Base>
class SeatImpl : public ActionStation<Base>
{
	private:
		typedef ActionStation<Base> Super;
		// "Backend" connections
		rbx::signals::scoped_connection seatTouched;
		rbx::signals::scoped_connection humanoidDoneSitting;

		bool disabled;

		void onEvent_seatTouched(shared_ptr<Instance> other) {
			if (Humanoid* h = Humanoid::humanoidFromBodyPart(other.get())) {
				if (PartInstance* torso = h->getTorsoSlow()) {
					if (this->sleepTimeUp() 
						&&	!findSeatWeld()
						&&  !h->getSit()
						&&  !h->getDead()
						&&  !disabled
						&&	!torso->computeNetworkOwnerIsSomeoneElse()
						&&	Workspace::contextInWorkspace(torso)
						&&	Workspace::contextInWorkspace(this)) {
						RBXASSERT(h->getTorsoSlow()->getPartPrimitive()->getAssembly() != this->getPartPrimitive()->getAssembly());
						createSeatWeld(h);
					}
				}
			}
		}

		void createSeatWeldInternal(Humanoid *h)
		{
			if (!h || !h->getTorsoSlow() || (DFFlag::FixSeatingWhileSitting && h->getSit()))
				return;

			RBXASSERT(h->getTorsoSlow()); // we just checked for this
			CoordinateFrame originalSeatCoord = this->getCoordinateFrame();
			RBXASSERT(this->getCoordinateFrame() == this->getPartPrimitive()->getCoordinateFrame());

			PartInstance* torsoSlow = h->getTorsoSlow();
			Primitive* torso = torsoSlow->getPartPrimitive();
			Assembly* torsoAssembly = torso->getAssembly();

			if (!torsoAssembly)
				return;

			RBXASSERT(torsoAssembly != this->getPartPrimitive()->getAssembly());

			Primitive* root = torsoAssembly->getAssemblyPrimitive();

			float seatOffset = this->getPartSizeXml().y*0.5f;
			float torsoOffset = root->getSize().y*0.5f;

			CoordinateFrame seatCoord;
			seatCoord.lookAt(Vector3(0,-1,0), Vector3(0,0,-1));
			seatCoord.translation = Vector3(0, seatOffset, 0);	// at the surface.

			CoordinateFrame torsoCoord;
			torsoCoord.lookAt(Vector3(0,-1,0), Vector3(0,0,-1));

			//Note that we hover the torso 0.5 studs higher than the seat, for the "leg room"
			if (DFFlag::FixAnchoredSeatingPosition){
				torsoCoord.translation = Vector3(0, -(torsoOffset + 0.5f), 0);
			}else{
				torsoCoord.translation = Vector3(0, -1.5, 0);
			}

			RBXASSERT(root == torso);
			root->setVelocity(Velocity::zero());

			if (DFFlag::FixAnchoredSeatingPosition){
				//If the seat is anchored the weld will not move the torso. To fix this, we move the torso before welding. (Again, note 0.5 for leg room)
				//root->setCoordinateFrame(this->getPartPrimitive()->getCoordinateFrame() * CoordinateFrame(Vector3(0, seatOffset + torsoOffset + 0.5f, 0)));
				PartInstance* part = PartInstance::fromPrimitive(root);
				part->setCoordinateFrame(this->getPartPrimitive()->getCoordinateFrame() * CoordinateFrame(Vector3(0, seatOffset + torsoOffset + 0.5f, 0)));
			}

			shared_ptr<Weld> tempWeld = Creatable<Instance>::create<Weld>();

			tempWeld->setName("SeatWeld");
			tempWeld->setPart0(this);
			tempWeld->setPart1(torsoSlow);

			tempWeld->setC0(seatCoord);
			tempWeld->setC1(torsoCoord);

			Instance::propArchivable.setValue(tempWeld.get(), false); 
			tempWeld->setParent(this);
		}

		// Destroy seat - on event no longer seated from humanoid state (simulator)
		void onEvent_humanoidDoneSitting() {
			findAndDestroySeatWeld();
		}

		void onChildRemoved(Instance* child) 
		{
			if (Weld* childWeld = isChildSeatWeld(child)) 
			{
				setOccupant(NULL);
				humanoidDoneSitting.disconnect();
				ActionStation<Base>::sleepTime = Time::now<Time::Fast>();
				onSeatedChanged(false, humanoidFromWeld(childWeld));
			}
		}

		void onChildAdded(Instance* child) 
		{
			if (Weld* childWeld = isChildSeatWeld(child)) 
			{
				destroyOtherWelds(childWeld);

				if (Humanoid* h = humanoidFromWeld(childWeld)) {
					setOccupant(h);
					h->setSeatPart(this);
					h->setSit(true);		// will fire this event
					humanoidDoneSitting = h->doneSittingSignal.connect(boost::bind(&SeatImpl::onEvent_humanoidDoneSitting, this));
					onSeatedChanged(true, h);
				}
			}
		}

		Humanoid* humanoidFromWeld(Weld* weld) {
			PartInstance* torso = weld ? weld->getPart1() : NULL;
			return Humanoid::humanoidFromBodyPart(torso);
		}

		Weld* findSeatWeld() {
			for (size_t i = 0; i < this->numChildren(); ++i) {
				if (Weld* answer = isChildSeatWeld(this->getChild(i))) {
					return answer;
				}
			}
			return NULL;
		}


		Weld* isChildSeatWeld(Instance* child) {
			if (child->getName() == "SeatWeld") {
				if (Weld* childWeld = child->fastDynamicCast<Weld>()) {
					return childWeld;
				}
			}
			return NULL;
		}

		void destroyOtherWeld(shared_ptr<Instance> child, Weld* childWeld) {
			if ((child.get() != childWeld) && isChildSeatWeld(child.get())) {
				child->setParent(NULL);
			}
		}

		void destroyOtherWelds(Weld* childWeld) {
			this->visitChildren(boost::bind(&SeatImpl::destroyOtherWeld, this, _1, childWeld));
		}
	
	protected:
		shared_ptr<Humanoid> occupant;

		void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider) {
			if(oldProvider) {
				seatTouched.disconnect(); // just to be safe
				onSeatedChanged(false, humanoidFromWeld(findSeatWeld()));		// seat is destroyed before weld in this case
			}

			Super::onServiceProvider(oldProvider, newProvider);

			if(newProvider) {
				seatTouched = ActionStation<Base>::onDemandWrite()->localSimulationTouchedSignal.connect(boost::bind(&SeatImpl::onEvent_seatTouched, this, _1));
			}
		}

		virtual void createSeatWeld(Humanoid *h) {
			createSeatWeldInternal(h);
		}

		virtual void findAndDestroySeatWeld() {
			findAndDestroySeatWeldInternal();
		}

		void createSeatWeldInternal(shared_ptr<Instance> h) {
			createSeatWeldInternal(Instance::fastSharedDynamicCast<Humanoid>(h).get());
		}

		void findAndDestroySeatWeldInternal()
		{
			while (Weld* weld = findSeatWeld()) {
				weld->setParent(NULL);
			}
		}

		/*implement*/ virtual void onSeatedChanged(bool seated, Humanoid* humanoid) {}
		/*implement*/ virtual void setOccupant(Humanoid* value) {}

	public:
		//		SeatImpl() {}
		const bool& getDisabled() const { return disabled; }
		void setDisabled(const bool& _value) { 
			if (!disabled && _value){
				// If the disabled property is changed to being disabled, we want to also stop the character from sitting
				while (Weld* weld = findSeatWeld()) {
					if (Humanoid* h = humanoidFromWeld(weld)){
						h->setSit(false);
					}
					weld->setParent(NULL);
				}
			}
			disabled = _value;
		}

		Humanoid* getOccupant() const {return occupant.get();}
};


extern const char* const sSeat;

class Seat : public DescribedCreatable<Seat, SeatImpl<BasicPartInstance>, sSeat>
{
	typedef SeatImpl<BasicPartInstance> Super;
public:

	rbx::remote_signal<void(shared_ptr<Instance>)> createSeatWeldSignal;
	rbx::remote_signal<void()> destroySeatWeldSignal;

	Seat();
	~Seat();

	/*override*/ void createSeatWeld(Humanoid *h);
	/*override*/ void findAndDestroySeatWeld();
	/*override*/ void onSeatedChanged(bool seated, Humanoid* humanoid);
	/*override*/ void setOccupant(Humanoid* value);
};


} // namespace