 /* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8World/IPipelined.h"
#include "V8World/Enum.h"
#include <list>

namespace RBX {
namespace Graphics {
class CullableSceneNode;
} }

namespace RBX {

	class RBXBaseClass Edge : public IPipelined
	{
	private:
		Sim::EdgeState		edgeState;
		Sim::ThrottleType	throttleType;

		Primitive*		prim0;
		Primitive*		prim1;

		int				index0;		// linked lists for primitive 0 and 1
		int				index1;

	protected:
		// protected - no one should set it here
		virtual void setPrimitive(int i, Primitive* p);

	public:
		typedef enum {JOINT, CONTACT} EdgeType;		// purely to eliminate dynamic casts

		Edge(Primitive* prim0, Primitive* prim1);

		virtual ~Edge() {
			RBXASSERT(index0 == -1);
			RBXASSERT(index1 == -1);
			RBXASSERT(prim0 == NULL);
			RBXASSERT(prim1 == NULL);
			index0 = -1;
			index1 = -1;
			prim0 = static_cast<Primitive*>(Debugable::badMemory());
			prim1 = static_cast<Primitive*>(Debugable::badMemory());
		}

		// poor man's way of no dynamic casts
		virtual EdgeType getEdgeType() const = 0;

        virtual void generateDataForMovingAssemblyStage(void) {}

		Sim::EdgeState getEdgeState() const {return edgeState;}
		void setEdgeState(Sim::EdgeState value) {edgeState = value;}

		Sim::ThrottleType getThrottleType() const {return throttleType;}
		void setThrottleType(Sim::ThrottleType value) {throttleType = value;}

		template<class Type>
		Type* fastCast(EdgeType edgeType) {
			bool match = (this->getEdgeType() == edgeType);
			RBXASSERT_VERY_FAST(match == (dynamic_cast<Type*>(this) != NULL));
			return match ? static_cast<Type*>(this) : NULL;
		}

		Primitive* getPrimitive(int i) {
			RBXASSERT_VERY_FAST((i == 0) || (i == 1));
			return (&prim0)[i];
		}

		const Primitive* getConstPrimitive(int i) const {
			RBXASSERT_VERY_FAST((i == 0) || (i == 1));
			return (&prim0)[i];
		}

		Primitive* otherPrimitive(const Primitive* p) {return (p == prim0) ? prim1 : prim0;}
		RBX::Graphics::CullableSceneNode* otherPrimitive(const RBX::Graphics::CullableSceneNode* p) { RBXASSERT(NULL); return NULL;}
		
		const Primitive* otherConstPrimitive(const Primitive* p) const {return (p == prim0) ? prim1 : prim0;}

		Primitive* otherPrimitive(int i) {
			RBXASSERT_VERY_FAST((i == 0) || (i == 1));
			return (&prim0)[(i + 1) % 2];
		}

		const Primitive* otherConstPrimitive(int i) const {
			RBXASSERT_VERY_FAST((i == 0) || (i == 1));
			return (&prim0)[(i + 1) % 2];
		}

		int getPrimitiveId(const Primitive* p) const {
			RBXASSERT_VERY_FAST(links(p));
			return (p == prim0) ? 0 : 1;
		}

		int getIndex(const Primitive* p) const {
			RBXASSERT_VERY_FAST(this->links(p));
			return (p == prim0) ? index0 : index1;
		}
		
		void setIndex(Primitive* p, int index) {
			RBXASSERT_VERY_FAST(this->links(p));
			if (p == prim0) {
				index0 = index;
			}
			else {
				index1 = index;
			}
		}

		bool links(const Primitive* p) const {
			return ((p == prim0) || (p == prim1));
		}

		bool links(Primitive* p0, Primitive* p1) const {
			return (	((p0 == prim0) && (p1 == prim1))
					||	((p0 == prim1) && (p1 == prim0))	
					);
		}
	};

}	// namespace
