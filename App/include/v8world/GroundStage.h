/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8World/IWorldStage.h"

namespace RBX {

	class Primitive;
	class Joint;
	class KernelJoint;
	class RigidJoint;

	class GroundStage : public IWorldStage {
	private:
		typedef IWorldStage Super;
		class EdgeStage* getEdgeStage();

		bool kernelJointHere(Primitive* p);

		void addGroundJoint(Primitive* p, bool grounded);
		void removeGroundJoint(Primitive* p, bool grounded);

		void onKernelJointAdded(KernelJoint* k);
		void onKernelJointRemoving(KernelJoint* k);

		void checkForFreeGroundJoint(RigidJoint* r);
		void rebuildFreeGround(Primitive* p);
		void rebuildOthers(Primitive* changedP);

		RigidJoint* heaviestRigidToGround(Primitive* p);

	public:
		///////////////////////////////////////////
		// IStage
		GroundStage(IStage* upstream, World* world);
		~GroundStage();

		/*override*/ IStage::StageType getStageType() const {return IStage::GROUND_STAGE;}

		void onPrimitiveAdded(Primitive* p);
		void onPrimitiveRemoving(Primitive* p);

		void onPrimitiveFixedChanging(Primitive* p);
		void onPrimitiveFixedChanged(Primitive* p);

		/*override*/ void onEdgeAdded(Edge* e);
		/*override*/ void onEdgeRemoving(Edge* e);
	};


} // namespace

