/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/KeyframeSequence.h"
#include "V8DataModel/AnimationTrackState.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/JointInstance.h"
#include "V8DataModel/Workspace.h"

DYNAMIC_FASTFLAGVARIABLE(AnimationEasingStylesEnabled, false)
DYNAMIC_FASTFLAGVARIABLE(CachedPoseInitialized, false)

namespace RBX {

const char* const sKeyframeSequence = "KeyframeSequence";

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<KeyframeSequence, shared_ptr<const Instances>()> func_getKeyframes(&KeyframeSequence::getKeyframes, "GetKeyframes", Security::None);
static Reflection::BoundFuncDesc<KeyframeSequence, void(shared_ptr<Instance>)> func_addKeyframe(&KeyframeSequence::addKeyframe, "AddKeyframe", "keyframe", Security::None);
static Reflection::BoundFuncDesc<KeyframeSequence, void(shared_ptr<Instance>)> func_removeKeyframe(&KeyframeSequence::removeKeyframe, "RemoveKeyframe", "keyframe", Security::None);

const Reflection::PropDescriptor<KeyframeSequence, bool> prop_Loop("Loop", category_Data, &KeyframeSequence::getLoop, &KeyframeSequence::setLoop);
const Reflection::EnumPropDescriptor<KeyframeSequence, KeyframeSequence::Priority> prop_Priority("Priority", category_Data, &KeyframeSequence::getPriority, &KeyframeSequence::setPriority);
REFLECTION_END();

const std::string IAnimatableJoint::sNULL = std::string();
const std::string IAnimatableJoint::sROOT = std::string("__Root");

namespace Reflection {
template<>
EnumDesc<RBX::KeyframeSequence::Priority>::EnumDesc()
:EnumDescriptor("AnimationPriority")
{
	addPair(KeyframeSequence::IDLE, "Idle");
	addPair(KeyframeSequence::MOVEMENT, "Movement");
	addPair(KeyframeSequence::ACTION, "Action");
	addPair(KeyframeSequence::CORE, "Core");
}
}//namespace Reflection


////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// FRONTEND AND BACKEND

KeyframeSequence::KeyframeSequence() 
: DescribedCreatable<KeyframeSequence, Instance, sKeyframeSequence>()
, loop(true)
, priority(KeyframeSequence::ACTION)
{
	setName(sKeyframeSequence);
}

shared_ptr<const Instances> KeyframeSequence::getKeyframes()
{
	return getChildren2();
}
void KeyframeSequence::addKeyframe(shared_ptr<Instance> keyframe)
{
	if (keyframe != NULL) {
		keyframe->setParent(this);
	}
}
void KeyframeSequence::removeKeyframe(shared_ptr<Instance> keyframe)
{
	if (keyframe != NULL) {
		if(keyframe->getParent() == this){
			keyframe->setParent(NULL);
		}
	}
}

static void CopyChild(boost::shared_ptr<Instance> instance, Instance* newParent)
{
    instance->luaClone()->setParent(newParent);
}

void KeyframeSequence::copyKeyframeSequence(KeyframeSequence* other)
{
	setLoop(other->getLoop());
	setPriority(other->getPriority());
	other->visitChildren(boost::bind(&CopyChild, _1, this));
}
void KeyframeSequence::setLoop(bool value)
{
	if(loop != value){
		loop = value;
		raisePropertyChanged(prop_Loop);
	}
}

void KeyframeSequence::setPriority(Priority value)
{
	if(priority != value){
		priority = value;
		raisePropertyChanged(prop_Priority);
	}
}

const KeyframeSequence::Cache& KeyframeSequence::getCachedData() const
{
	if(!cache.isValid)
	{
		cacheData();
	}
	return cache;
}

float KeyframeSequence::getDuration() const
{
	return getCachedData().duration;
}

void KeyframeSequence::apply(std::vector<PoseAccumulator>& jointposes, double lastkeyframetime, double keyframetime, float trackweight) const
{
	if(trackweight <= 0) return;

	// find two keyframes to lerp.
	CachedKeyframe* before = NULL;
	CachedKeyframe* after = NULL;

	getCachedData(); // validates cache.

	if(cache.keyframes.size() == 0)
	{
		return; // no pose.
	}

	if( (loop && cache.duration > 0.0) && (keyframetime > cache.duration || keyframetime < 0) )
	{
		float duration = cache.duration;
		if (keyframetime < 0)
		{
			int durationsOff = (int)(fabs(keyframetime) / duration) + 1;
			keyframetime += durationsOff * duration;
		}
		else if (keyframetime > duration)
		{
			int durationsOff = (int)(keyframetime / duration);
			keyframetime -= durationsOff * duration;
		}
	}

	//todo:  cache the translation step somewhere in the AnimationTrack /AnimationTrackState

	// now we have all the joints, and the two keyframes. lets' go!

	for(size_t i = 0; i < cache.animatedJoints.size(); ++i)
	{
		std::string parentName = cache.namedParts[cache.animatedJoints[i].first];
		std::string childName = cache.namedParts[cache.animatedJoints[i].second];

		for(size_t j = 0; j < jointposes.size(); ++j)
		{			
			if (jointposes[j].joint.first.lock() != NULL) 
			{
				IAnimatableJoint* joint = jointposes[j].joint.second;

				if(joint != NULL &&
					joint->getParentName() == parentName &&
					joint->getPartName() == childName)
				{

					// find the before/after keyframes.
					before = &cache.keyframes[0];
					after = &cache.keyframes[cache.keyframes.size()-1];
					for(size_t k = 0; k < cache.keyframes.size(); ++k)
					{
						if(cache.keyframes[k].time <= keyframetime && cache.keyframes[k].poses[i] != NULL)
						{
							before = &cache.keyframes[k];
						}
						else if (cache.keyframes[k].poses[i] != NULL)
						{
							after = &cache.keyframes[k];
							break;
						}
					}

					if (after->poses[i] == NULL)
					{
						after = before;
					}

					// found pose;
					if(!before->poses[i] || !after->poses[i])
						continue; // joint masked out of these two keyframes. 

					double afterw = (keyframetime - before->time);
					double beforew = (after->time - keyframetime);
					if(afterw + beforew <= 0)
					{
						afterw = 1.0f; // just take the after pose.
					}

					CachedPose ipose = CachedPose::interpolatePoses(*before->poses[i], *after->poses[i], (float)beforew, (float)afterw);
					if (DFFlag::CachedPoseInitialized)
						ipose.initialized = true;

					// re-weight based on trackweight. (for fade-in and fade-out)
					ipose.weight =		G3D::lerp(0.0f, ipose.weight, trackweight);
					ipose.maskWeight =	G3D::lerp(1.0f, ipose.maskWeight, trackweight);

					if (DFFlag::CachedPoseInitialized && !jointposes[j].pose.initialized)
					{
						jointposes[j].pose = ipose;
					} else {
						jointposes[j].pose = CachedPose::blendPoses(jointposes[j].pose, ipose);
					}
					if (DFFlag::CachedPoseInitialized)
						jointposes[j].pose.initialized = true;
					continue;
				}
			}
		}
	}


}


void KeyframeSequence::onChildAdded(Instance* child) 
{
	Super::onChildAdded(child);
	invalidateCache();
}
void KeyframeSequence::onChildRemoved(Instance* child) 
{
	Super::onChildRemoved(child);
	invalidateCache();
}

void KeyframeSequence::invalidateCache()
{
	cache.isValid = false;
}

template<class V>
size_t findOrAdd(std::vector<V>& v, const V& s)
{
	size_t i;
	for(i = 0; i < v.size() && v[i] != s; ++i);
	if(i == v.size())
		v.push_back(s);
	return i;
}


void KeyframeSequence::AppendPosePass0(const shared_ptr<Instance>& child) const
{
	Pose* pose = Instance::fastDynamicCast<Pose>(child.get());
	if(pose)
	{
		CachedPose cpose;

		// here we are pre-populating cache.animatedJoints, because we need to know how many total
		// *unique* animatable joints are in this animation.
		bool isRoot = Instance::fastDynamicCast<Pose>(pose->getParent()) == 0;
		std::string parentName = isRoot ? IAnimatableJoint::sROOT : pose->getParent()->getName();
		std::string childName = pose->getName();
		size_t parentPart = findOrAdd(cache.namedParts, parentName);
		size_t childPart = findOrAdd(cache.namedParts, childName);
		findOrAdd(cache.animatedJoints, std::make_pair(parentPart, childPart));

		// here we keep track of the pose count, so we can pre-allocate the whole block.
		// (which will allow us to take pointers into the vector, as opposed to indices)
		cache.poseCount++;
	}

}
void KeyframeSequence::AppendPosePass1(const shared_ptr<Instance>& child, std::vector<CachedPose*>* poses) const
{
	Pose* pose = Instance::fastDynamicCast<Pose>(child.get());
	if(pose)
	{
		CachedPose cpose;

		bool isRoot = Instance::fastDynamicCast<Pose>(pose->getParent()) == 0;
		
		std::string parentName = isRoot ? "__Root" : pose->getParent()->getName();
		std::string childName = pose->getName();
		size_t parentPart = findOrAdd(cache.namedParts, parentName);
		size_t childPart = findOrAdd(cache.namedParts, childName);

		size_t jointindex = findOrAdd(cache.animatedJoints, std::make_pair(parentPart, childPart));

		cpose.weight = pose->getWeight();
		cpose.setCFrame(pose->getCoordinateFrame());
		cpose.maskWeight = pose->getMaskWeight();
		if (DFFlag::AnimationEasingStylesEnabled)
		{
			cpose.easingStyle = pose->getEasingStyle();
			cpose.easingDirection = pose->getEasingDirection();
		}

		cache.allPoses.push_back(cpose);
		(*poses)[jointindex] = &cache.allPoses.back();

	}

}

KeyframeSequence::CachedKeyframe KeyframeSequence::makeKeyframe(Keyframe* kf) const
{
	CachedKeyframe ckf;
	ckf.time = kf->getTime();
	ckf.poses.resize(cache.animatedJoints.size()); // all initialized to null

	kf->visitDescendants(boost::bind(&KeyframeSequence::AppendPosePass1, this, _1, &ckf.poses));

	return ckf;
}

void KeyframeSequence::cacheKeyframePass0(const shared_ptr<Instance>& child) const
{
	Keyframe* kf = Instance::fastDynamicCast<Keyframe>(child.get());
	if(kf)
	{
		cache.duration = std::max(cache.duration,kf->getTime());
		
		kf->visitDescendants(boost::bind(&KeyframeSequence::AppendPosePass0, this, _1));
	}
}
void KeyframeSequence::cacheKeyframePass1(const shared_ptr<Instance>& child) const
{
	Keyframe* kf = Instance::fastDynamicCast<Keyframe>(child.get());
	if(kf)
	{
		cache.keyframes.push_back(makeKeyframe(kf));
		
	}
}

void KeyframeSequence::cacheData() const
{
	cache.allPoses.clear();
	cache.animatedJoints.clear();
	cache.namedParts.clear();
	cache.keyframes.clear();
	cache.duration = 0.0f;
	cache.poseCount = 0;
	
	visitChildren(boost::bind(&KeyframeSequence::cacheKeyframePass0, this, _1));
	cache.allPoses.reserve(cache.poseCount);

	visitChildren(boost::bind(&KeyframeSequence::cacheKeyframePass1, this, _1));
	std::sort(cache.keyframes.begin(), cache.keyframes.end());

	cache.isValid = true;
}

CoordinateFrame CachedPose::getCFrame() const
{
	Vector3 axis = rotaxisangle;
	float angle = axis.unitize();
	return CoordinateFrame(Matrix3::fromAxisAngleFast(axis, angle), translation);
}
void CachedPose::setCFrame(const CoordinateFrame& cframe)
{
	translation = cframe.translation;
	Vector3 axis;
	float angle;
	cframe.rotation.toAxisAngle(axis, angle);
	rotaxisangle = axis * angle;
	initialized = true;
}


// converts one axisAngle into the corresponding vector in the oposite direction, but that
// corresponds to the same rotation. Give it the length or the vector.
inline Vector3 flipAxisAngle(const Vector3& r, float lr)
{
	return r * ((lr - Math::twoPif())/lr);
}

// assuming length of vector < pi, this lerps the shortest rotation distance.
Vector3 lerpAxisAngle(const Vector3& r0, const Vector3& r1, float w0, float w1)
{
	float dot = r0.dot(r1);

	if(dot < 0) // vectors oposite. shortest path _possibly_ not the linear lerp.
	{
		float l0 = r0.length();
		float l1 = r1.length();
		RBXASSERT(l0 <= Math::pif()+0.01 && l1 <= Math::pif()+0.01);
		if (!(l0 < l1))
		{
			float lenofr1onr0 = - dot / l0;
			if(l0 + lenofr1onr0 > Math::pif()) 
			{
				// flip r0.
				Vector3 r = flipAxisAngle(r0, l0) * w0 + r1 * w1;
				// normalize result in the 0..pi range.
				float lr_sq = r.squaredLength();
				if(lr_sq > Math::pif() * Math::pif())
				{
					return flipAxisAngle(r, sqrt(lr_sq));
				}
				else
				{
					return r;
				}
			}
		}
		else // l1 >= l0
		{
			if(G3D::fuzzyEq(l1, 0)) 
			{
				return Vector3::zero();
			}
			else
			{
				// i'm lazy. should probably expand this so we can re-use length calculation done above.
				return lerpAxisAngle(r1, r0, w1, w0);
			}
		}
	}
	return r0 * w0 + r1 * w1;
}

float bounceEasingStyle(float t)
{

	if (t < 0.36363636)
	{
		return 7.5625 * t * t;
	}
	else if(t < 0.72727272)
	{
		t -= 0.54545454;
		return 7.5625 * t * t + 0.75;
	}
	else if(t < 0.90909090)
	{
		t -= 0.81818181;
		return 7.5625 * t * t + 0.9375;
	}
	else
	{
		t -= 0.95454545;
		return 7.5625 * t * t + 0.984375;
	}
}

CachedPose CachedPose::interpolatePoses(const CachedPose& p0, const CachedPose& p1, float w0, float w1)
{
	if(w0 <= 0 || (DFFlag::CachedPoseInitialized && !p0.initialized))
		return p1;
	if(w1 <= 0 || (DFFlag::CachedPoseInitialized && !p1.initialized))
		return p0;

	float nw0 = w0 / (w0 + w1);
	float nw1 = w1 / (w0 + w1);


	if (DFFlag::AnimationEasingStylesEnabled){

		switch(p0.easingStyle)
		{
		case Pose::POSE_EASING_STYLE_LINEAR:
		default:
			//these cases don't affect the calculated weights
			break;

		case Pose::POSE_EASING_STYLE_CONSTANT:
			switch(p0.easingDirection)
			{
			default:
			case Pose::POSE_EASING_DIRECTION_OUT:
				nw0 = 1;
				nw1 = 0;
				break;
			case Pose::POSE_EASING_DIRECTION_IN_OUT:
				if (nw0 > 0.5f)
					nw0 = 1.0f;
				else
					nw0 = 0;
				nw1 = 1 - nw0;
				break;
			case Pose::POSE_EASING_DIRECTION_IN:
				nw0 = 0;
				nw1 = 1;
				break;
			}
			break;

		case Pose::POSE_EASING_STYLE_ELASTIC:
			//elastic function: p = overshoot factor
			//(p * x) - ( (x ^ 3) * (p - 1) )

			switch(p0.easingDirection)
			{
			default:
			case Pose::POSE_EASING_DIRECTION_OUT:
				{
					float totalTime = 1.0f;
					float p = totalTime*.3;
					float t = nw1;
					float s = p/4;
					nw1 = 1 + pow(2,-10*t) * sin( (t*totalTime-s)*(Math::twoPi())/p );
					nw0 = 1 - nw1;
					break;
				}
			case Pose::POSE_EASING_DIRECTION_IN:
				{
					float totalTime = 1.0f;
					float p = totalTime*.3;
					float t = nw0;
					float s = p/4;
					nw0 = 1 + pow(2,-10*t) * sin( (t*totalTime-s)*(Math::twoPi())/p );
					nw1 = 1 - nw0;
					break;
				}
			case Pose::POSE_EASING_DIRECTION_IN_OUT:
				{
					float t = nw0 / (0.5f);
					float p = (.3*1.5);
					float s = p/4;

					if (t < 1) {
						t -= 1;
						nw0 = -.5 * pow(2,10*t) * sin( (t-s)*(Math::twoPi())/p );
					}
					else {
						t -= 1;
						nw0 = 1 + 0.5 * pow(2,-10*t) * sin( (t-s)*(Math::twoPi())/p );
					}
					nw1 = 1 - nw0;
					break;
				}
			}
			break;

		case Pose::POSE_EASING_STYLE_CUBIC:
			switch(p0.easingDirection)
			{
			default:
			case Pose::POSE_EASING_DIRECTION_OUT:
				nw0 = 1 - pow((1 - nw0), 3);
				nw1 = 1 - nw0;
				break;
			case Pose::POSE_EASING_DIRECTION_IN_OUT:
				if (nw0 < 0.5f)
				{
					nw0 = pow(2*nw0, 3) * 0.5f;
				}
				else
				{
					nw0 = (1 - pow((2 - 2 * nw0),3)) * 0.5f + 0.5f;
				}
				nw1 = 1 - nw0;
				break;
			case Pose::POSE_EASING_DIRECTION_IN:
				nw0 = pow(nw0, 3);
				nw1 = 1 - nw0;
			}
			break;

		case Pose::POSE_EASING_STYLE_BOUNCE:
			switch(p0.easingDirection)
			{
			default:
			case Pose::POSE_EASING_DIRECTION_IN:
				{
					nw0 = bounceEasingStyle(nw0);
					nw1 = 1 - nw0;
					break;
				}
			case Pose::POSE_EASING_DIRECTION_OUT:
				{
					nw0 = 1 - bounceEasingStyle(1 - nw0);
					nw1 = 1 - nw0;
					break;
				}
			case Pose::POSE_EASING_DIRECTION_IN_OUT:
				{
					nw0 = nw0 * 2;
					if (nw0 < 1)
					{
						nw0 = bounceEasingStyle(nw0) * 0.5f;
					}
					else
					{
						nw0 = 0.5f + bounceEasingStyle(nw0 - 1) * 0.5f;
					}
					nw1 = 1 - nw0;
					break;
				}
			}
			break;
		}
	}

	CachedPose r;
	r.weight = nw0 * p0.weight + nw1 * p1.weight;
	r.maskWeight = nw0 * p0.maskWeight + nw1 * p1.maskWeight; 
										 // keep maskWeights normalized, seems like the right thing. confirm.
	r.translation = p0.translation * nw0 + p1.translation * nw1;
	r.rotaxisangle = lerpAxisAngle(p0.rotaxisangle, p1.rotaxisangle, nw0, nw1);
	return r;
}

// applies p1's mask to p0.
CachedPose CachedPose::blendPoses(const CachedPose& p0, const CachedPose& p1)
{
	CachedPose r;

	float maskedw0 = std::min(p0.weight, p1.maskWeight); // don't want to double fade-out a faded animation. used min instead of *.
	r.weight = maskedw0 +p1.weight;
	r.maskWeight = std::min(p0.maskWeight, p1.maskWeight); // nb: this value will now only be used at the top level priority collapse.
	r.translation = maskedw0 * p0.translation + p1.weight * p1.translation;
	r.rotaxisangle = lerpAxisAngle(p0.rotaxisangle, p1.rotaxisangle, maskedw0, p1.weight);
	if (DFFlag::CachedPoseInitialized)
		r.initialized = true;

	return r;
}

void KeyframeSequence::verifySetAncestor(const Instance* const newParent, const Instance* const instanceGettingNewParent) const
{
	Super::verifySetAncestor(newParent, instanceGettingNewParent);
}

} // RBX
