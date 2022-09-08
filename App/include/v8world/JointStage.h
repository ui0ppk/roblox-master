/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8World/IWorldStage.h"
#include "Util/ConcurrencyValidator.h"
#include "Util/BiMultiMap.h"

namespace RBX {

	class Edge;
	class Joint;

	class JointStage : public IWorldStage {
	private:
		ConcurrencyValidator concurrencyValidator;
		class GroundStage*		getGroundStage();

		typedef RBX::BiMultiMap<Primitive*, Joint*> JointMap;	// find incomplete Joints by primitive
		JointMap				jointMap;						// is identical to the primitive fields stored in the fields
		std::set<Joint*>		incompleteJoints;
		std::set<Primitive*>	primitivesHere;
																// of all joints in the incompleteJoints list
		void moveEdgeToDownstream(Edge* e);
		void removeEdgeFromDownstream(Edge* e);

		void moveJointToDownstream(Joint* j);
		void removeJointFromDownstream(Joint* j);

		void putJointHere(Joint* j);
		void removeJointFromHere(Joint* j);

		bool edgeHasPrimitiveHere(Edge *e, Primitive* p);
		bool edgeHasPrimitivesHere(Edge *e);
		void visitAddedPrimitive(Primitive* p, Joint* j, std::vector<Joint*>& jointsToPush);

	public:
		///////////////////////////////////////////
		// IStage
		JointStage(IStage* upstream, World* world);

		~JointStage();

		/*override*/ IStage::StageType getStageType() const {return IStage::JOINT_STAGE;}

		/*override*/ void onEdgeAdded(Edge* e);
		/*override*/ void onEdgeRemoving(Edge* e);

		void onPrimitiveAdded(Primitive* p);
		void onPrimitiveRemoving(Primitive* p);
	};
} // namespace