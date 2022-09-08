/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8Kernel/Cofm.h"
#include "V8Kernel/Body.h"

#include "V8Kernel/Constants.h"
#include "Util/Units.h"
#include "Util/Math.h"
#include "rbx/Debug.h"

namespace RBX {

Cofm::Cofm(Body* body) 
: body(body)
, dirty(true)
{}

void Cofm::updateIfDirty() 
{
	if (dirty) {
		mass = body->getMass();

 		if (body->numChildren() == 0) {
 			cofmInBody = body->getCofmOffset();
 			moment = body->getIBodyAtPoint(cofmInBody);
			
			RBXASSERT(Math::fuzzyEq(moment,
						Math::momentToObjectSpace(body->getIWorldAtPoint(body->getCoordinateFrame().pointToWorldSpace(cofmInBody)),
							body->getCoordinateFrame().rotation),
						moment.l1Norm() * 0.01));
 		}
 		else {
			Vector3 bodyCofmOffset = body->getCofmOffset();
			Vector3 bodyCofmInWorld = body->getCoordinateFrame().pointToWorldSpace(bodyCofmOffset);		// toWorldSpace(bodyCofmOffset);

			Vector3 cofmWorld = bodyCofmInWorld * body->getMass();  // Needs to transform and add offset.
			for (int i = 0; i < body->numChildren(); ++i) {
				Body* b = body->getChild(i);
				mass += b->getBranchMass();
				cofmWorld += b->getBranchCofmPos() * b->getBranchMass();
			}
			cofmWorld = cofmWorld / mass;
			cofmInBody = body->getCoordinateFrame().pointToObjectSpace(cofmWorld);

			Matrix3 iWorldSum = body->getIWorldAtPoint(cofmWorld);	
			for (int i = 0; i < body->numChildren(); ++i) {
				iWorldSum = iWorldSum + body->getChild(i)->getBranchIWorldAtPoint(cofmWorld);
			}
			moment = Math::momentToObjectSpace(iWorldSum, body->getCoordinateFrame().rotation);
		}

		RBXASSERT(dirty);		// concurrency issues?
		dirty = false;
	}
}



} // namespace
