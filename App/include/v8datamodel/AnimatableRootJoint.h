#pragma once

#include "V8DataModel/PartInstance.h"
#include "V8DataModel/IAnimatableJoint.h"

namespace RBX
{
	class AnimatableRootJoint : public IAnimatableJoint
	{
		bool isAnimating;
		shared_ptr<PartInstance> part;
		CoordinateFrame lastCFrame;
	public:
		AnimatableRootJoint(const shared_ptr<PartInstance>& part);
		PartInstance* getPart() const { return part.get(); }

		/*override*/ void setAnimating(bool value);
		/*override*/ const std::string& getParentName();
		/*override*/ const std::string& getPartName();
		/*override*/ void applyPose(const CachedPose& pose);
	};
}