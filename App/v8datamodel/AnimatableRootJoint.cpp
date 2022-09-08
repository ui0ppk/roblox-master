#include "stdafx.h"

#include "V8DataModel/AnimatableRootJoint.h"

using namespace RBX;

AnimatableRootJoint::AnimatableRootJoint(const shared_ptr<PartInstance>& part)
: IAnimatableJoint(), part(part)
  ,isAnimating(false)
{
}

void AnimatableRootJoint::setAnimating(bool value)
{
	if(isAnimating  != value && value)
	{
		// reset.
		lastCFrame = CoordinateFrame();
	}
	isAnimating = value;
}

const std::string& AnimatableRootJoint::getParentName()
{
	return IAnimatableJoint::sROOT;
}

const std::string& AnimatableRootJoint::getPartName()
{
	return part->getName();
}

void AnimatableRootJoint::applyPose(const CachedPose& pose)
{
	//nb: we completely ignore blend weights unless they are zero
	//nb: we ignore maskWeight and pretend it is 1. (since we don't prevent physics from acting)
	CoordinateFrame poseCFrame = pose.getCFrame();
	if(pose.weight > 0)
	{
		// to let physics act on the part, we extract the change from last frame.
		// then just apply the change.
		// toObjectSpace is this->inverse() * c;
		// delta = last^-1 * pose
		// newcframe = currentcframe * delta
		// newcframe = currentcframe * last^-1 * pose
		// so if currentcframe == last (eg: part was anchored), then
		// newcframe = pose 
		CoordinateFrame delta = lastCFrame.toObjectSpace(poseCFrame);
		
		part->setCoordinateFrame(part->getCoordinateFrame() * delta);
	}

	lastCFrame = poseCFrame;
}
