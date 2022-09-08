/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8World/IWorldStage.h"
#include <map>

namespace RBX {

	class Joint;
	class Primitive;
	/*	
		Sits between the world and the JointStage (for now, the engine);

		Receives: 
			1)	Primitives
			2)	Edges (Joints and Contacts)

		Passes Downstream: 
			1)	Primitives
			2)	Edges 
				a) between two primitives, both different, both non-null
		*/

	class CleanStage : public IWorldStage {
	private:
		class JointStage*		getJointStage();

		bool primitivesAreOk(Edge* e);

	public:
		///////////////////////////////////////////
		// IStage
		CleanStage(IStage* upstream, World* world);

		~CleanStage() {}

		/*override*/ IStage::StageType getStageType() const {return IStage::CLEAN_STAGE;}

		/*override*/ void onEdgeAdded(Edge* e);
		/*override*/ void onEdgeRemoving(Edge* e);

		void onPrimitiveAdded(Primitive* p);
		void onPrimitiveRemoving(Primitive* p);

		void onJointPrimitiveNulling(Joint* j, Primitive* nulling);
		void onJointPrimitiveSet(Joint* j, Primitive* p);
	};
} // namespace