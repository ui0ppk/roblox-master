/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Kernel/KernelIndex.h"
#include "V8Kernel/Cofm.h"
#include "V8Kernel/SimBody.h"
#include "V8Kernel/Link.h"
#include "Util/IndexArray.h"
#include "Util/IndexedTree.h"
#include "Util/Memory.h"
#include "rbx/threadsafe.h"

class btCollisionObject;

namespace RBX {

	/*
			Body class.  Handles rigid joints and kinematic/dynamic joints.  Automatically calculates
			mass properties.  Automatically updates state.

			StateRoot:	Body or Joint that contains the latest state id.
			RigidRoot:	Body that I am clumped with
			COFM:		Calculated for every body in a rigid group - chain is broken for linked bodies
			SIMBody:	Only Free Bodies have this

			Whenever adjusting a joint, must increment the stateRoot of the assembly

			World Body				// Special Body - no COFM
				|	|
				|	-- Body			// anchored body - rigid join to the world body - when moving, adjust state index
				|
				-- Body				// anchored body - rigid join to the world body 
					|
					-- Body
					|
					-- Body
						0			// this is a link
						-- Body		// this is the StateRoot for the chain below it
							|
							Body

				Body (free)			// this is the StateRoot for the free body and chain
					|				// this body has group COFM for the top three bodies + SimBody
					-- Body
					|
					-- Body
						0
						-- Body		// this body has a group COFM for the bodies below it
							|
							Body

			COFM:	body, kinematic, assembly (assumes a floating object)

	*/

	class BodyPvSetter;

	class Body	: public IndexedTree 
				, public Allocator<Body>
	{
	public:
		friend class KernelData;
		friend class Kernel;
		friend class SimBody;

	private:
		rbx::spin_mutex		mutex;		// for safe calls that require update

        // Unique identifier set by the World
        boost::uint64_t     uid;
        int                 guidIndex;

		int					leafBodyIndex;
		
		int&				getLeafBodyIndex()		{return leafBodyIndex;}

		int					connectorUseCount;	// how many connectors connect this body
		static Body*		worldBody;
		static void initStaticData();			// inits the world body

		Body*				root;				// top body - either anchored or 6 dof

		Cofm*				cofm;		
		Cofm*				getCofm()				{return cofm;}		// use these to insure const correctness;

		SimBody*			simBody;			// Only present for parent && in kernel
		SimBody*			getSimBody()			{return simBody;}
		const SimBody*		getConstSimBody() const	{return simBody;}

		void				refreshCofm();

		Link*				link;				// if link != NULL, use for "getMeInParent()"

		// defining variables
		bool				canThrottle;		// this body can throttle - i.e., slow down
		CoordinateFrame		meInParent;			// used if no link, i.e. - rigid connection
		Matrix3				moment;
		float				mass;
		Vector3				cofmOffset;

		// resulting variables
		unsigned int					stateIndex;
		PV					pv;				

		void				resetRoot(Body* newRoot);

		bool				validateParentCofmDirty();

		const CoordinateFrame& getMeInParent() {
			RBXASSERT(getParent());
			return getLink() ? getLink()->getChildInParent() : meInParent;
		}
		const CoordinateFrame& getConstMeInParent() const {
			RBXASSERT(getConstParent());
			RBXASSERT(!getConstLink());				// fails if linked (i.e. only works in same clump);
			return meInParent;
		}

        void updatePV();  // Does not inline anyhow.

		bool pvIsUpToDate() const {
			if (!getConstParent()) {
				return true;
			}
			else {
				if (stateIndex != getRoot()->getStateIndexNoUpdate()) {
					return false;
				}
				else {
					return getConstParent()->pvIsUpToDate();
				}
			}
		}

		void onChildAdded(Body* newChild);
		void onChildRemoved(Body* newChild);

		Body* calcRoot()					{return getParent() ? getParent()->calcRoot() : this;}
		const Body* calcRootConst() const	{return getConstParent() ? getConstParent()->calcRootConst() : this;}

		///////////////////////////////////////////////
		// Indexed Tree
		/*override*/ void onParentChanging();
		/*override*/ void onParentChanged(IndexedTree* oldParent);
		/*override*/ void onChildAdding(IndexedTree* child);
		/*override*/ void onChildAdded(IndexedTree* child);
		/*override*/ void onChildRemoved(IndexedTree* child);

	public:
		Body();

		~Body();

		static unsigned int getNextStateIndex();

		// Static world body
		static Body* getWorldBody();

		SimBody*			getRootSimBody()			{return getRoot()->getSimBody();}
		const SimBody*		getConstRootSimBody() const	{return getRoot()->getConstSimBody();}

        void setUID( boost::uint64_t _uid );
        boost::uint64_t getUID() const { return uid; }

        // Used only by the solver inspector to id the objects between different clients
        void setGuidIndex( int _guidIndex ) { guidIndex = _guidIndex; }
        int getGuidIndex() const { return guidIndex; }

		//////////////////////////////////////////////////////
		// From Cofm, SimBody
		bool cofmIsClean() {return getCofm() ? !getCofm()->getIsDirty() : true;}

		//////////////////////////////////////////////////////
		// From Child
		void makeCofmDirty();

		void advanceStateIndex();

		void makeStateDirty() {
			getRoot()->advanceStateIndex();
		}

		unsigned int getStateIndex() {
			updatePV();
			return stateIndex;
		}

		unsigned int getStateIndexNoUpdate() const {
			return stateIndex;
		}

		//////////////////////////////////////////////////////
		// Base vs. Branch

		Body* getChild(int i)						{return getTypedChild<Body>(i);}
		const Body* getConstChild(int i) const		{return getConstTypedChild<Body>(i);}

		Body* getParent()							{return getTypedParent<Body>();}
		const Body* getConstParent() const			{return getConstTypedParent<Body>();}

		Link* getLink()								{return link;}
		const Link*	getConstLink() const			{return link;}

		const Body* getRoot() const {
			RBXASSERT_SLOW(root == calcRootConst());
			return root;
		}

		Body* getRoot() {
			RBXASSERT_SLOW(root == calcRootConst());
			return root;
		}

		const Vector3& getCofmOffset() {
			return cofmOffset;
		}

		const Vector3& getBranchCofmOffset();

		// Only works on bodies in same clump - otherwise not const for the link and will assert
		CoordinateFrame getMeInAncestor(const Body* ancestor) const {
			if (ancestor == this) {
				return CoordinateFrame();
			}
			else if (ancestor == getConstParent()) {
				return getConstMeInParent();
			}
			else {
				return getConstParent()->getMeInAncestor(ancestor) * getConstMeInParent();
			}
		}

		inline float getMass() const						{return mass;}
		inline Matrix3 getIBody() const					{return moment;}
		inline Vector3 getIBodyV3() const					{return Math::toDiagonal(getIBody());}
		Matrix3 getIBodyAtPoint(const Vector3& point);
		inline Matrix3 getMoment() const            {return getIBody();}
		inline Vector3 getPrincipalMoment() const   {return getIBodyV3();}
		Matrix3 getIWorld()							{return Math::momentToWorldSpace(getIBody(), getCoordinateFrame().rotation);}
		Matrix3 getIWorldAtPoint(const Vector3& point);

		// Branch refers to everything below me....
		float getBranchMass()						{return getCofm() ? getCofm()->getMass() : mass;}					
		Matrix3 getBranchIBody()					{return getCofm() ? getCofm()->getMoment() : moment;}
		Vector3 getBranchIBodyV3()					{return Math::toDiagonal(getBranchIBody());}
		Matrix3 getBranchIWorld()					{return Math::momentToWorldSpace(getBranchIBody(), getCoordinateFrame().rotation);}
		Matrix3 getBranchIWorldAtPoint(const Vector3& point);
		Vector3 getBranchCofmPos();
		CoordinateFrame getBranchCofmCoordinateFrame();

		// 

		const PV& getPvFast() const {
			RBXASSERT_FISHING(pvIsUpToDate());
			return pv;
		}

		// Current Job should hold the Data Model write lock or somehow have locked the Body::mutex.
		const PV& getPvUnsafe() {
			updatePV();
			return pv;
		}

		const PV& getPV_Spin_Lock() {
			rbx::spin_mutex::scoped_lock lock(mutex);
			updatePV();
			return pv;
		}

		const PV& getPvSafe() const {
			Body* thisNotConst = const_cast<Body*>(this);			
			return thisNotConst->getPV_Spin_Lock();
		}

		const Vector3& getPosFast() const {
			RBXASSERT_FISHING(pvIsUpToDate());
			return pv.position.translation;
		}

		const Vector3& getPos() {
			updatePV();
			return pv.position.translation;
		}
	
		const CoordinateFrame& getCoordinateFrameFast() const {
			RBXASSERT_FISHING(pvIsUpToDate());
			return pv.position;
		}

		const CoordinateFrame& getCoordinateFrame() {
			updatePV();
			return pv.position;
		}

		const Velocity& getVelocity() {
			updatePV();
			return pv.velocity;
		}

		bool getCanThrottle() const {
			return canThrottle;
		}

		void accumulateImpulseAtBranchCofm(const Vector3& impulse) {
			if (SimBody* s = getRootSimBody()) {
				s->accumulateImpulseAtBranchCofm(impulse);
			}
		}

		void accumulateLinearImpulse(const Vector3& impulse, const Vector3& worldPos) {
			if (SimBody* s = getRootSimBody()) {
				s->accumulateImpulse(impulse, worldPos);
			}
		}

		void accumulateRotationalImpulse(const Vector3& impulse) {
			if (SimBody* s = getRootSimBody()) {
				s->accumulateRotationalImpulse(impulse);
			}
		}

		void accumulateForceAtBranchCofm(const Vector3& force) {
			RBXASSERT(getRoot() == this);			// should only be called on Root objects
			if (SimBody* s = getRootSimBody()) {
				s->accumulateForceCofm(force);
			}
		}

		void accumulateForce(const Vector3& force, const Vector3& worldPos) {
			if (SimBody* s = getRootSimBody()) {
				s->accumulateForce(force, worldPos);
			}
		}

		void accumulateTorque(const Vector3& torque) {
			if (SimBody* s = getRootSimBody()) {
				s->accumulateTorque(torque);
			}
		}

		void resetForceAccumulators() {
			if (SimBody* s = getRootSimBody()) {
				s->resetForceAccumulators();
			}
		}

		void resetImpulseAccumulators() {
			if (SimBody* s = getRootSimBody()) {
				s->resetImpulseAccumulators();
			}
		}

		const Vector3& getBranchForce() const {
			RBXASSERT(getRoot() == this);			// should only be called on Root objects
			const SimBody* s = getConstRootSimBody();
			return s ? s->getForce() : Vector3::zero();
		}

		const Vector3& getBranchTorque() const {
			RBXASSERT(getRoot() == this);			// should only be called on Root objects
			const SimBody* s = getConstRootSimBody();
			return s ? s->getTorque() : Vector3::zero();
		}

		const Velocity& getBranchVelocity() {		// velocity at the COFM of the assembly
			RBXASSERT(getRoot() == this);			// should only be called on Root objects
			const SimBody* s = getRootSimBody();
			return s ? s->getPV().velocity : Velocity::zero();
		}

		/////////////////////////////////////////////////////
		// setting properties
		//
		void setParent(Body* parent) {setIndexedTreeParent(parent);}

		void setMeInParent(const CoordinateFrame& _meInParent);

		void setMeInParent(Link* _link);

		void setMass(float _mass);

		void setMoment(const Matrix3& _momentInBody);

		void setCofmOffset(const Vector3& _centerOfMassInBody);


		// Only Primitive can set these - make sure we keep byproducts updated - fuzzy extents, etc.
		// ToDo:  Hack - is there a better way to limit access to only primitives, while not including "friend class Primitive"???

		void setPv(const PV& _pv, const BodyPvSetter& bpv);

		void setCoordinateFrame(const CoordinateFrame& worldCoord, const BodyPvSetter& bpv);

		void setVelocity(const Velocity& worldVelocity, const BodyPvSetter& bpv);

		void setCanThrottle(bool value, const BodyPvSetter& bpv);

        void updateBulletCollisionObject(btCollisionObject* object);

	public:

		/////////////////////////////////////////////////////////
		// Debugging / reporting functions / complex stuff
		//

		inline bool isLeafBody()		const	{return leafBodyIndex >= 0;}

		float kineticEnergy();
		float potentialEnergy();

	};

} // namespace

