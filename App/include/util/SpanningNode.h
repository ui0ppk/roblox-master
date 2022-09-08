/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/IndexedMesh.h"

namespace RBX {

	class SpanningEdge;

	class SpanningNode : public IndexedMesh
	{
		friend class SpanningEdge;

	private:
		SpanningEdge*	edgeToParent;

	protected:
		virtual SpanningEdge* getFirstSpanningEdge() = 0;
		virtual SpanningEdge* getNextSpanningEdge(SpanningEdge* edge) = 0;

		void setEdgeToParent(SpanningEdge* edge);

	public:
		SpanningNode() : edgeToParent(NULL) {}

		~SpanningNode() {}

		SpanningNode* getParent()					{return getTypedParent<SpanningNode>();}
		const SpanningNode* getConstParent() const	{return getConstTypedParent<SpanningNode>();}

		SpanningNode* getChild(int i) {return getTypedChild<SpanningNode>(i);}
		
		SpanningEdge* getEdgeToParent()							{return edgeToParent;}
		const SpanningEdge* getConstEdgeToParent() const		{return edgeToParent;}

		static int getDepth(SpanningNode* node) {
			if (!node) {
				return 0;
			}
			else if (!node->getParent()) {
				return 1;
			}
			else {
				return getDepth(node->getParent()) + 1;
			}
		}

		bool lessThan(const IndexedTree* other) const;

		template<class Func>
		inline void visitEdges(Func func)
		{
			SpanningEdge* edge = getFirstSpanningEdge();
			while (edge) {
				func(this, edge);
				edge = getNextSpanningEdge(edge);
			}
		}
	};

} // namespace
