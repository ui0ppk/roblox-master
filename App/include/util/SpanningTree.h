/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "G3D/Array.h"
#include <set>

namespace RBX {

	class SpanningNode;
	class SpanningEdge;

	class SpanningTree
	{
	private:
		G3D::Array<SpanningEdge*> tempEdges;
		int size;

		static SpanningNode* lightParent(int testSide, SpanningNode* child, SpanningEdge*& answer, int& lightSide);
		static SpanningNode* testEdgeToParent(int testSide, SpanningNode* child, SpanningEdge*& answer, int& lightSide);
		static void findLightestUpstream(SpanningNode* n0, SpanningNode* n1, int d0, int d1, SpanningEdge*& answer, int& lightSide);
		static void buildDownstreamTree(SpanningNode* root, std::set<SpanningNode*>& tree);

		void removeEdge(SpanningEdge* edge);
		void addEdge(SpanningEdge* edge, SpanningNode* newParent);

		void findAndDeactivateEdges(SpanningNode* child, SpanningEdge* deactivate, G3D::Array<SpanningEdge*>& toActivate);
		void activateEdges(SpanningNode* child, const G3D::Array<SpanningEdge*>& toActivate);

		static void findLightestUpstream(SpanningEdge* e, SpanningEdge*& answer, int& lightSide);
		static SpanningEdge* findHeaviestDownstream(SpanningNode* node, SpanningNode*& newParent);

		void swapTree(SpanningEdge* deactivate, SpanningEdge* activate, SpanningNode* newParent);
		void swap(SpanningEdge* deactivate, SpanningEdge* activate, SpanningNode* newParent);


	protected:
		/*implement*/ virtual void onSpanningEdgeAdding(SpanningEdge* edge, SpanningNode* child) {}
		/*implement*/ virtual void onSpanningEdgeAdded(SpanningEdge* edge) {}
		/*implement*/ virtual void onSpanningEdgeRemoving(SpanningEdge* edge) {}
		/*implement*/ virtual void onSpanningEdgeRemoved(SpanningEdge* edge, SpanningNode* child) {}
		/*implement*/ virtual bool validateTree(SpanningNode* root) {return true;}

	public:
		void insertSpanningTreeEdge(SpanningEdge* insertEdge);
		void removeSpanningTreeEdge(SpanningEdge* removeEdge);

		SpanningTree();
		~SpanningTree();
	};

} // namespace

