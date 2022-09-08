/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8World/IWorldStage.h"


namespace RBX {

	class Primitive;

	class EdgeStage : public IWorldStage {
	private:
		typedef IWorldStage Super;
		class ContactStage* getContactStage();

	public:
		///////////////////////////////////////////
		// IStage
		EdgeStage(IStage* upstream, World* world);

		~EdgeStage() {}

		/*override*/ IStage::StageType getStageType() const {return IStage::EDGE_STAGE;}

		/*override*/ void onEdgeAdded(Edge* e);
		/*override*/ void onEdgeRemoving(Edge* e);

		void onPrimitiveAdded(Primitive* p);
		void onPrimitiveRemoving(Primitive* p);
	};
} // namespace