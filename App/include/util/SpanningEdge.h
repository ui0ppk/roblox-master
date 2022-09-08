/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/IndexedTree.h"

namespace RBX {

	class SpanningNode;
	class SpanningTree;

	class SpanningEdge
	{
		friend class SpanningTree;

	private:
		void removeFromSpanningTree();
		void addToSpanningTree(SpanningNode* newParent);

	protected:

	public:
		SpanningEdge() {}

		virtual ~SpanningEdge() {}

		bool isLighterThan(const SpanningEdge* other) const {
			return other->isHeavierThan(this);
		}

		bool inSpanningTree() const;

		SpanningNode* getChildSpanningNode();
		SpanningNode* getParentSpanningNode();

		const SpanningNode* getConstChildSpanningNode() const;
		const SpanningNode* getConstParentSpanningNode() const;

		//////////////////////////////////////////////////////////
		//
		virtual bool isHeavierThan(const SpanningEdge* other) const = 0;

		virtual SpanningNode* otherNode(SpanningNode* n) = 0;
		virtual const SpanningNode* otherConstNode(const SpanningNode* n) const = 0;

		virtual SpanningNode* getNode(int i) = 0;
		virtual const SpanningNode* getConstNode(int i) const = 0;

		SpanningNode* otherNode(int i) {
			RBXASSERT_VERY_FAST((i == 0) || (i == 1));
			return getNode((i + 1) % 2);
		}

		//////////////////////////////////////////////////////////
		//
		static bool heavierEdge(const SpanningEdge* test, const SpanningEdge* other) {
			return test->isHeavierThan(other);
		}
	};

} // namespace

