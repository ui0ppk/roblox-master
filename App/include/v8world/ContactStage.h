/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8World/IWorldStage.h"

namespace RBX {

	class Primitive;

	class ContactStage : public IWorldStage {
	private:
		class TreeStage* getTreeStage();

	public:
		///////////////////////////////////////////
		// IStage
		ContactStage(IStage* upstream, World* world);

		~ContactStage() {}

		/*override*/ IStage::StageType getStageType() const {return IStage::CONTACT_STAGE;}

		/*override*/ void onEdgeAdded(Edge* e);
		/*override*/ void onEdgeRemoving(Edge* e);

		void onPrimitiveAdded(Primitive* p);
		void onPrimitiveRemoving(Primitive* p);
	};
} // namespace