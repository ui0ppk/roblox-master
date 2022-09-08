/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Humanoid/Jumping.h"
#include "Humanoid/Humanoid.h"

#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Accoutrement.h"
#include "V8World/Primitive.h"
#include "V8World/World.h"
#include "V8World/ContactManager.h"
#include "V8Kernel/Body.h"
#include "V8Kernel/Kernel.h"

DYNAMIC_FASTINTVARIABLE(PGSJumpForceAdjustment, 520)

namespace RBX {
namespace HUMAN {

const char* const sJumping = "Jumping";


Jumping::Jumping(Humanoid* humanoid, StateType priorState)
	:Named<Flying, sJumping>(humanoid, priorState)
	,jumpDir(0.0f, 1.0f, 0.0f)
{
	if (priorState == SWIMMING)
	{
		setOutOfWater();
	}
	else if (priorState == CLIMBING) 
	{
		Vector3 lookDir = humanoid->getTorsoSlow()->getCoordinateFrame().lookVector();
		lookDir.y = 0.0f;
		lookDir = lookDir.unit();
		jumpDir = jumpDir - (lookDir);
		jumpDir = jumpDir.unit();
	}
	setTimer(0.5);	
}


void Jumping::onComputeForceImpl()
{
	Super::onComputeForceImpl();

 	if (!getFinished())
	{
		Humanoid* humanoid = getHumanoid();
		Body* root = humanoid->getRootBodyFast();

		float jumpVelocity = root->getVelocity().linear.dot(jumpDir);

		float targetVelocity = humanoid->getJumpPower();

		const float yAccelDesired = root ? kJumpP() * (targetVelocity - jumpVelocity) : 0.0f;
		//StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "%f", root->getVelocity().linear.y);

		if (yAccelDesired<=0.0) {		// We've acheived the desired velocity
			setFinished(true);
		}
		else if (findCeiling()) {
			setFinished(true);
		}
		else if (root ) {
			Primitive* floor = getFloorPrimitive();
			if (floor) {
				Body* floorRoot = floor->getBody()->getRoot();
				float mass;
				if (getHumanoid()->getWorld()->getUsingPGSSolver())
				{
					mass = root->getBranchMass();
				} else {
					mass = std::min(root->getBranchMass(), floorRoot->getBranchMass());
				}
				const float newForceY = mass * yAccelDesired;
				const float& currentForceYRef = root->getBranchForce().y;

				if (newForceY > currentForceYRef) {
					const float diff = newForceY - currentForceYRef;

					if (getHumanoid()->getWorld()->getUsingPGSSolver())  // For PGS kernel bodies we apply a fraction of the force
					{
                        float factor = (float) DFInt::PGSJumpForceAdjustment / 1000.0f;
						root->accumulateForceAtBranchCofm(jumpDir * newForceY * factor);
						floorRoot->accumulateForce(Vector3(0.0, -diff * factor, 0.0), getFloorTouchInWorld());
					}
					else
					{
						// TODO: Clamp this?
						root->accumulateForceAtBranchCofm(jumpDir * newForceY);
						floorRoot->accumulateForce(Vector3(0.0, -diff, 0.0), getFloorTouchInWorld());
					}
				}
			} else 
			{ // always apply - if we are not near the floor, the jumping state will end
				const float newForceY = root->getBranchMass() * yAccelDesired;
				if (getHumanoid()->getWorld()->getUsingPGSSolver())
					root->accumulateForceAtBranchCofm(jumpDir * newForceY * 0.1);
				else
					root->accumulateForceAtBranchCofm(jumpDir * newForceY);
			}
		}
	}
}

bool Jumping::findCeiling()
{
	Humanoid* humanoid = getHumanoid();

	if (Primitive* torsoPrim = humanoid->getTorsoPrimitiveSlow())
	{
		const CoordinateFrame& torsoC = torsoPrim->getCoordinateFrame();

		Vector3 halfSize = 0.4f * torsoPrim->getSize();

		Vector3 pointOnTorso = torsoC.pointToWorldSpace(Vector3(0, -halfSize.y, 0));

		RbxRay tryRay = RbxRay::fromOriginAndDirection(pointOnTorso, Vector3::unitY());

		float maxDistance;
		Primitive* headPrim = humanoid->getHeadPrimitiveSlow();
		
		if (headPrim)
			maxDistance = headPrim->getSize().y + torsoPrim->getSize().y * 1.5f;
		else
			maxDistance = torsoPrim->getSize().y * 1.5f;

		shared_ptr<PartInstance> ceilingPart = tryCeiling(tryRay, maxDistance, torsoPrim->getAssembly());

		if (ceilingPart)
			return true;
		else
		{
			for (int x = -1; x <= 1; x+=2) {
				for (int z = -1; z <= 1; z+=2) 
				{
					Vector3 offset (x * halfSize.x, -halfSize.y, z * halfSize.z);

					pointOnTorso = torsoC.pointToWorldSpace(offset);				

					tryRay = RbxRay::fromOriginAndDirection(pointOnTorso, Vector3::unitY());

					if ((ceilingPart = tryCeiling(tryRay, maxDistance, torsoPrim->getAssembly())))
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

shared_ptr<PartInstance> Jumping::tryCeiling(const RbxRay& ray, float maxDistance, Assembly* humanoidAssembly)
{
	Humanoid* humanoid = getHumanoid();
	float distance;
	RBXASSERT(humanoidAssembly);

	filteringAssembly = humanoidAssembly;
	Vector3 hitPoint;

	Primitive* floorPrim = humanoid->getWorld()->getContactManager()->getHitLegacy(	ray, 
																					NULL, 
																					this, 
																					hitPoint, 
																					distance, 
																					maxDistance,
																					true);
	shared_ptr<PartInstance> answer;
	if (floorPrim && (floorPrim->getAssembly() != humanoidAssembly)) 
	{
		answer = shared_from<PartInstance>(PartInstance::fromPrimitive(floorPrim));
	}
	return answer;
}

HitTestFilter::Result Jumping::filterResult(const Primitive* testMe) const
{
	RBXASSERT(testMe);

	HitTestFilter::Result newResult = HitTestFilter::INCLUDE_PRIM;

	if (	(!testMe->getCanCollide())
		||	(filteringAssembly && (filteringAssembly == testMe->getConstAssembly()))
		|| (Instance::fastDynamicCast<Accoutrement>(PartInstance::fromConstPrimitive(testMe))))
	{
		newResult = HitTestFilter::IGNORE_PRIM;
	}

	return newResult;
}

} // namespace
} // namespace