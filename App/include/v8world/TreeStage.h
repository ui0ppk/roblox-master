/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8World/IWorldStage.h"
#include "V8World/Enum.h"
#include "Util/Utilities.h"
#include "Util/SpanningTree.h"

namespace RBX {

	class Primitive;
	class Joint;
	class Mechanism;
	class Clump;
	class Edge;

	class JointSort {
	public:
		static bool heavierJoint(const Joint* j0, const Joint* j1);
	};

	class TreeStage : public IWorldStage
					, public SpanningTree
	{
	private:
		typedef SpanningTree Super;		
		int									maxTreeDepth;

		///////////////////////////////////////////////////////
		///////////////////////////////////////////////////////
		// 
		// Data storage
		std::set<Mechanism*>				dirtyMechanisms;	
		std::set<Mechanism*>				downstreamMechanisms;	

		///////////////////////////////////////////////////////
		// Traverse Utilities
		//
		void removeSpanningTreeJoint(Joint* j);
		void swapTree(Joint* deactivate, Joint* activate, Primitive* newParent);

		///////////////////////////////////////////////
		// Spanning Tree
		//
		/*override*/ void onSpanningEdgeAdding(SpanningEdge* edge, SpanningNode* child);
		/*override*/ void onSpanningEdgeAdded(SpanningEdge* edge);
		/*override*/ void onSpanningEdgeRemoving(SpanningEdge* edge);
		/*override*/ void onSpanningEdgeRemoved(SpanningEdge* edge, SpanningNode* child);
		/*override*/ bool validateTree(SpanningNode* root);

		void removeFromPipeline(Mechanism* m);
		void dirtyMechanism(Mechanism* m);
		void cleanMechanism(Mechanism* m);	// true if moved downstream

		void destroyClump(Primitive* p);
		void destroyAssembly(Primitive* p);
		void destroyMechanism(Primitive* p);

	public:
		///////////////////////////////////////////
		// IStage
		TreeStage(IStage* upstream, World* world);

		~TreeStage();

		/*override*/ IStage::StageType getStageType() const {return IStage::TREE_STAGE;}

		/*override*/ void onEdgeAdded(Edge* e);
		/*override*/ void onEdgeRemoving(Edge* e);

		/*override*/ int getMetric(IWorldStage::MetricType metricType);

		/////////////////////////////////////////////
		// From the Joint Stage
		//
		void onPrimitiveAdded(Primitive* p);
		void onPrimitiveRemoving(Primitive* p);

		/////////////////////////////////////////////
		// From the World
		//

		void assemble();				// update everything;

		bool isAssembled() const		{return dirtyMechanisms.empty();}

		// from internal and world
		void sendClumpChangedMessage(Primitive* childPrim);
	};

} // namespace


