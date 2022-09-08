/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8World/IWorldStage.h"
#include "V8World/Assembly.h"
#include "Util/BiMultiMap.h"

namespace RBX {
	class Assembly;

	class EdgeBuffer : public IWorldStage {
	private:
		// DEBUG ONLY
		typedef RBX::BiMultiMap<Assembly*, Edge*> AssemblyEdgeMap;	// find incomplete Joints by primitive
		AssemblyEdgeMap assemblyEdges;

		bool debugPushEdgeToDownstream(Edge* e);
		bool debugRemoveEdgeFromDownstream(Edge* e);
		bool debugAddAssembly(Assembly* a);
		bool debugRemoveAssembly(Assembly* a);

		bool assemblyIsHere(Assembly* a);

		void assemblyPrimitiveAdded(Primitive* p);
		void assemblyPrimitiveRemoved(Primitive* p);

		void pushEdgeIfOk(Edge* e);
		bool pushSpringOk(Edge* e);
		bool pushKinematicOk(Edge* e);
		void removeEdgeIfDownstream(Edge* e);

	protected:
		void afterAssemblyAdded(Assembly* a);
		void beforeAssemblyRemoving(Assembly* a);

		EdgeBuffer(IStage* upstream, IStage* downstream, World* world) 
			: IWorldStage(upstream, downstream, world)
		{}

		virtual ~EdgeBuffer();

		/*override*/ void onEdgeAdded(Edge* e);
		/*override*/ void onEdgeRemoving(Edge* e);
	};
} // namespace
