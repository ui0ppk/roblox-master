/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"


#include "V8DataModel/Animator.h"
#include "V8DataModel/Animation.h"
#include "V8DataModel/AnimationTrack.h"
#include "V8DataModel/AnimationTrackState.h"
#include "V8DataModel/JointInstance.h"
#include "V8DataModel/Workspace.h"
#include "V8World/SendPhysics.h"
#include "V8World/World.h"
#include "V8World/Primitive.h"
#include "V8World/Assembly.h"
#include "Network/Players.h"
#include "Network/NetworkOwner.h"

#include "humanoid/Humanoid.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/AnimatableRootJoint.h"
#include "v8datamodel/AnimationController.h"

#include "rbx/Profiler.h"

DYNAMIC_FASTFLAGVARIABLE(ErrorOnFailedToLoadAnim, false)

namespace RBX {

const char* const sAnimator = "Animator";

REFLECTION_BEGIN();
//Keep this interface in sync with the proxy one on Humanoid.
//Returns an AnimationTrack
static Reflection::BoundFuncDesc<Animator, shared_ptr<Instance>(shared_ptr<Instance>)> desc_LoadAnimation(&Animator::loadAnimation, "LoadAnimation","animation", Security::None);

// animation state replication
static Reflection::RemoteEventDesc<Animator, void(ContentId, float,float,float)> event_OnPlay(&Animator::onPlaySignal, "OnPlay", "animation", "fadeTime", "weight", "speed", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::BROADCAST);
static Reflection::RemoteEventDesc<Animator, void(ContentId, float)> event_OnStop(&Animator::onStopSignal, "OnStop", "animation", "fadeTime", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::BROADCAST);
static Reflection::RemoteEventDesc<Animator, void(ContentId, float)> event_OnAdjustSpeed(&Animator::onAdjustSpeedSignal, "OnAdjustSpeed", "animation", "speed", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::BROADCAST);
static Reflection::RemoteEventDesc<Animator, void(ContentId, float)> event_OnSetTimePosition(&Animator::onSetTimePositionSignal, "OnSetTimePosition", "animation", "timePosition", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::BROADCAST);
REFLECTION_END();

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// FRONTEND AND BACKEND

Animator::Animator() 
: DescribedCreatable<Animator, Instance, sAnimator>()
{
	setName(sAnimator);

	FASTLOG1(FLog::ISteppedLifetime, "Animator created - %p", this);

	onPlaySignal.connect(boost::bind(&Animator::onPlay, this, _1, _2, _3, _4));
	onStopSignal.connect(boost::bind(&Animator::onStop, this, _1, _2));
	onAdjustSpeedSignal.connect(boost::bind(&Animator::onAdjustSpeed, this, _1, _2));
	onSetTimePositionSignal.connect(boost::bind(&Animator::onSetTimePosition, this, _1, _2));
}

Animator::Animator(Instance* replicatingContainer)
: DescribedCreatable<Animator, Instance, sAnimator>()
{
    shared_ptr<Instance> container = shared_from(replicatingContainer);

	setName(sAnimator);

	FASTLOG1(FLog::ISteppedLifetime, "Animator created - %p", this);

	onPlaySignal.connect(boost::bind(&Animator::onPlay, this, _1, _2, _3, _4));
	onStopSignal.connect(boost::bind(&Animator::onStop, this, _1, _2));
	onAdjustSpeedSignal.connect(boost::bind(&Animator::onAdjustSpeed, this, _1, _2));
	onSetTimePositionSignal.connect(boost::bind(&Animator::onSetTimePosition, this, _1, _2));
}

Animator::~Animator()
{
	FASTLOG1(FLog::ISteppedLifetime, "Animator destroyed - %p", this);
}

Instance* Animator::getRootInstance()
{
	if (getParent())
	{
		return getParent()->getParent();
	}

	return NULL;
}

void Animator::setupClumpChangedListener(Instance* rootInstance)
{
	if (PartInstance* p = rootInstance->findFirstDescendantOfType<PartInstance>())
	{
		if (shared_ptr<PartInstance> rootPart = Instance::fastSharedDynamicCast<PartInstance>(p->getRootPart()))
			clumpChangedConnection = rootPart->onDemandWrite()->clumpChangedSignal.connect(boost::bind(&Animator::onEvent_ClumpChanged, this, _1));
	}
}

void Animator::reloadAnimation(shared_ptr<AnimationTrackState> animationTrackState)
{
	if (animationTrackState->getKeyframeSequence()) 
	{
		if(std::find(activeAnimations.begin(), activeAnimations.end(), animationTrackState) == activeAnimations.end()){
			activeAnimations.push_back(animationTrackState);
		}
	}
}

float Animator::getGameTime() const 
{
    RunService* runservice = NULL;
	runservice = ServiceProvider::find<RunService>(getParent());

    if(runservice)
    {
        return (float)runservice->gameTime();
    }
	return 0.0f;
}

shared_ptr<Instance> Animator::loadAnimation(shared_ptr<Instance> instance)
{
	shared_ptr<Animation> animation = Instance::fastSharedDynamicCast<Animation>(instance);

	if(animation)
	{
		shared_ptr<const KeyframeSequence> kfs = animation->getKeyframeSequence(getParent());
		if (!kfs)
		{
			StandardOut::singleton()->printf(MESSAGE_WARNING, "Object must be in Workspace before loading animation.");
		}

		shared_ptr<Animator> sharedThis = shared_from(this);
		shared_ptr<AnimationTrackState> trackState(Creatable<Instance>::create<AnimationTrackState>(kfs, sharedThis));
		shared_ptr<AnimationTrack> track(Creatable<Instance>::create<AnimationTrack>(trackState, sharedThis, animation));
		trackState->setAnimationTrack(track);
		trackState->setName(instance->getName());
		track->setName(instance->getName());
		return track;
	}
	else
	{
		throw RBX::runtime_error("Argument error: must be an Animation object");
	}
}

void Animator::onTrackStepped(shared_ptr<AnimationTrackState> track, double time, KeyframeSequence::Priority priority, 
								std::vector<PoseAccumulator>* poses
								)
{
	if(track->getPriority() == priority)
	{
		if(poses->size() == 0) // delay creation
		{
			poses->reserve(animatableJoints.size());
			for(size_t i = 0; i < animatableJoints.size(); ++i)
			{
				poses->push_back(PoseAccumulator(animatableJoints[i]));
			}
		}

		track->step(*poses, time);
	}
}


void Animator::appendAnimatableJointsRec(shared_ptr<Instance> instance, shared_ptr<Instance> exclude)
{
	if (instance == exclude)
		return;

	Motor* joint = Instance::fastDynamicCast<Motor>(instance.get());
	if(joint)
	{
		joint->setIsAnimatedJoint(true);
		animatableJoints.push_back(JointPair(instance, joint));
	}
}

void resetJointFlag(shared_ptr<Instance> instance)
{
    Motor* joint = Instance::fastDynamicCast<Motor>(instance.get());
    if(joint)
    {
        joint->setIsAnimatedJoint(false);
    }
}

void Animator::calcAnimatableJoints(Instance* rootInstance, shared_ptr<Instance> exclude)
{
	animatableJoints.clear();

	if(rootInstance)
	{  
		rootInstance->visitDescendants(boost::bind(&resetJointFlag, _1));
		rootInstance->visitDescendants(boost::bind(&Animator::appendAnimatableJointsRec, this, _1, exclude));
	}
}

void Animator::onStepped(const Stepped& event)
{
    RBXPROFILER_SCOPE("Physics", "Animator::onStepped");

	calcAnimatableJoints(getRootInstance());

	if (animatableJoints.size() == 0 || activeAnimations.size() == 0)
		return;

	std::vector<PoseAccumulator> core;
	std::vector<PoseAccumulator> idle;
	std::vector<PoseAccumulator> movement;
	std::vector<PoseAccumulator> action;

	float gameTime = getGameTime();
	for(std::list<shared_ptr<AnimationTrackState> >::iterator iter = activeAnimations.begin(); iter != activeAnimations.end();) {
		if(!(*iter)->getKeyframeSequence() || (*iter)->isStopped(gameTime)) {
			activeAnimations.erase(iter++);
			continue;
		}
		++iter;
	}

	// checking to see if the server should take control of this object - NPC humanoid
	PartInstance* part = testForServerLockPart.get();
	if (part != NULL && activeAnimations.size() > 0 && Time::now<Time::Fast>() > serverLockTimer) {
		Primitive* prim = part->getPartPrimitive();
		Assembly* a = prim->getAssembly();
		if (a != NULL) {
			prim = a->getAssemblyPrimitive();
			part = PartInstance::fromPrimitive(prim);
		}
		part->resetNetworkOwnerTime(3.0f);								// wait 

		part->setNetworkOwnerAndNotify(Network::NetworkOwner::Server());

		serverLockTimer = Time::now<Time::Fast>() + Time::Interval(2.5f);
	}

	std::for_each(activeAnimations.begin(), activeAnimations.end(), boost::bind(&Animator::onTrackStepped, this, _1, event.gameTime, KeyframeSequence::CORE, &core));
	std::for_each(activeAnimations.begin(), activeAnimations.end(), boost::bind(&Animator::onTrackStepped, this, _1, event.gameTime, KeyframeSequence::IDLE, &idle));
	std::for_each(activeAnimations.begin(), activeAnimations.end(), boost::bind(&Animator::onTrackStepped, this, _1, event.gameTime, KeyframeSequence::MOVEMENT, &movement));
	std::for_each(activeAnimations.begin(), activeAnimations.end(), boost::bind(&Animator::onTrackStepped, this, _1, event.gameTime, KeyframeSequence::ACTION, &action));

	//fold all poses into idle
	if(idle.size() == 0)
		idle = core;
	else
	{
		for(size_t i = 0; i < core.size(); ++i)
		{
			core[i].pose.weight *= idle[i].pose.maskWeight; // here is where we use the blendweight. make the blend is that.
			idle[i].pose = CachedPose::blendPoses(core[i].pose, idle[i].pose);
		}
	}


	if(idle.size() == 0)
		idle = movement;
	else
	{
		for(size_t i = 0; i < movement.size(); ++i)
		{
			idle[i].pose.weight *= movement[i].pose.maskWeight; // here is where we use the blendweight. make the blend is that.
			idle[i].pose = CachedPose::blendPoses(idle[i].pose, movement[i].pose);
		}
	}

	if(idle.size() == 0)
		idle = action;
	else
	{
		for(size_t i = 0; i < action.size(); ++i)
		{
			idle[i].pose.weight *= action[i].pose.maskWeight; // here is where we use the blendweight. make the blend is that.
			idle[i].pose = CachedPose::blendPoses(idle[i].pose, action[i].pose);
		}
	}
	
	// now, apply poses to joints;
	if(idle.size() != 0)
	{
		// need to call applyPose even on untouched animation, to allow legacy animaitons to show.
		// if you won't want this, make smarter uses of setAnimating()
		for(size_t i = 0; i < idle.size(); ++i)
		{
			shared_ptr<Instance> pInst = idle[i].joint.first.lock();
			if (pInst != NULL) 
			{
				idle[i].joint.second->applyPose(idle[i].pose);
			}
		}
	}
	else
	{
		for(size_t i = 0; i < animatableJoints.size(); ++i)
		{
			if (animatableJoints[i].first.lock() != NULL) 
			{
				animatableJoints[i].second->applyPose(CachedPose());
			}
		}
	}
}

/*override*/ void Animator::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider) 
{
	Super::onServiceProvider(oldProvider, newProvider);
	onServiceProviderIStepped(oldProvider, newProvider);		// hooks up stepped

	if (!newProvider)
		return;
}

/*override*/ void Animator::verifySetParent(const Instance* instance) const
{
    if (instance == NULL || fastDynamicCast<Humanoid>(instance) || fastDynamicCast<AnimationController>(instance))
    {
        Super::verifySetParent(instance);
    }
    else
    {
        throw RBX::runtime_error("Animator has to be placed under Humanoid or AnimationController!");
    }
}

/*override*/ bool Animator::askAddChild(const Instance* instance) const 
{ 
	return Instance::fastDynamicCast<AnimationTrackState>(instance)  != NULL; 
}

void Animator::onAncestorChanged(const AncestorChanged& event)
{
	Super::onAncestorChanged(event);
}

void Animator::onEvent_DescendantAdded(shared_ptr<Instance> descendant)
{
	IAnimatableJoint* joint = dynamic_cast<IAnimatableJoint*>(descendant.get());
	if(joint)
	{
		Instance* figure = getRootInstance();
		calcAnimatableJoints(figure);

		if (animatableJoints.size() > 0)
			setupClumpChangedListener(figure);
	}
}

void Animator::onEvent_DescendantRemoving(shared_ptr<Instance> descendant)
{
	IAnimatableJoint* joint = dynamic_cast<IAnimatableJoint*>(descendant.get());
	if(joint)
	{
		calcAnimatableJoints(getRootInstance(), descendant);

		if (animatableJoints.size() == 0)
			clumpChangedConnection.disconnect();
	}
}

void Animator::onEvent_ClumpChanged(shared_ptr<Instance> instance)
{
	RBXASSERT(animatableJoints.size() > 0);

	PartInstance* part = Instance::fastDynamicCast<PartInstance>(instance.get());
	if (Primitive* primitive = part->getPartPrimitive())
	{
		if (Assembly* assembly = primitive->getAssembly())
		{
			// check for new root assembly part	
			PartInstance* rootPart = PartInstance::fromAssembly(assembly);
			if ((rootPart != part) && rootPart->isDescendantOf(getRootInstance()))
				clumpChangedConnection = rootPart->onDemandWrite()->clumpChangedSignal.connect(boost::bind(&Animator::onEvent_ClumpChanged, this, _1));

			assembly->setAnimationControlled(true);
		}
	}
}

void Animator::onEvent_AncestorModified()
{
	descentdantAdded = NULL;
	descentdantRemoved = NULL;
	ancestorChanged = NULL;
	clumpChangedConnection = NULL;

    Instance* figure = getRootInstance();

    if(figure)
    {
        descentdantAdded = figure->onDemandWrite()->descendantAddedSignal.connect(boost::bind(&Animator::onEvent_DescendantAdded, this, _1));
        descentdantRemoved = figure->onDemandWrite()->descendantRemovingSignal.connect(boost::bind(&Animator::onEvent_DescendantRemoving, this, _1));
        ancestorChanged =  figure->ancestryChangedSignal.connect(boost::bind(&Animator::onEvent_AncestorModified, this));
    } 
	calcAnimatableJoints(figure);

	// listen to assembly change only if we have animatable joints, so we can mark it as animation controlled
	if (animatableJoints.size() > 0)
		setupClumpChangedListener(figure);
}

void Animator::onPlay(ContentId animation, float fadeTime, float weight, float speed)
{
    std::map<ContentId, shared_ptr<AnimationTrack> >::iterator node = animationTrackMap.find(animation);
    if (node == animationTrackMap.end())
    {
        passiveLoadAnimation(animation);
        node = animationTrackMap.find(animation);
    }
    if (node != animationTrackMap.end())
    {
        if (Workspace::showActiveAnimationAsset)
        {
            StandardOut::singleton()->printf(MESSAGE_INFO, "Play animation: %s", node->first.c_str());
        }
        node->second->localPlay(fadeTime, weight, speed);
        if (Workspace::showActiveAnimationAsset)
        {
            activeAnimation = node->second->getAnimationName();
            activeAnimation.append(" (");
            activeAnimation.append(node->first.c_str());
            activeAnimation.append(")");
        }
    }

	tellParentAnimationPlayed(node->second);
}

shared_ptr<const Reflection::ValueArray> Animator::getPlayingAnimationTracks()
{
	shared_ptr<Reflection::ValueArray> playingAnimationTracks(rbx::make_shared<Reflection::ValueArray>());
	std::list<shared_ptr<AnimationTrackState> >::iterator node;
	for (node = activeAnimations.begin(); node != activeAnimations.end(); ++node)
	{
		shared_ptr<Instance> animationTrack(node->get()->getAnimationTrack());
		playingAnimationTracks->push_back(animationTrack);
	}
	return playingAnimationTracks;
}

void Animator::tellParentAnimationPlayed(shared_ptr<Instance> animationTrack) {
	Instance* parent = getParent();
	if (parent)
	{
		AnimationController* animationController = Instance::fastDynamicCast<AnimationController>(parent);
		if (animationController)
		{
			animationController->animationPlayedSignal(animationTrack);
		}
		else
		{
			Humanoid* humanoid = Instance::fastDynamicCast<Humanoid>(parent);
			if (humanoid)
			{
				humanoid->animationPlayedSignal(animationTrack);
			}
		}
	}
}

void Animator::onStop(ContentId animation, float fadeTime)
{
    std::map<ContentId, shared_ptr<AnimationTrack> >::iterator node = animationTrackMap.find(animation);
    if (node != animationTrackMap.end())
    {
        if (Workspace::showActiveAnimationAsset)
        {
            StandardOut::singleton()->printf(MESSAGE_INFO, "Stop animation: %s", node->first.c_str());
        }
        node->second->localStop(fadeTime);
    }
}

void Animator::onAdjustSpeed(ContentId animation, float speed)
{
	std::map<ContentId, shared_ptr<AnimationTrack> >::iterator node = animationTrackMap.find(animation);
	if (node != animationTrackMap.end())
	{
		node->second->localAdjustSpeed(speed);
	}
}

void Animator::onSetTimePosition(ContentId animation, float timePosition)
{
	std::map<ContentId, shared_ptr<AnimationTrack> >::iterator node = animationTrackMap.find(animation);
	if (node != animationTrackMap.end())
	{
		node->second->localSetTimePosition(timePosition);
	}
}

void Animator::passiveLoadAnimation(ContentId animationId)
{
    if (animationTrackMap.find(animationId) == animationTrackMap.end())
    {
        if (Workspace::showActiveAnimationAsset)
        {
            StandardOut::singleton()->printf(MESSAGE_INFO, "Add animation: %s", animationId.toString().c_str());
        }
        shared_ptr<Animation> sharedAnimation(Creatable<Instance>::create<Animation>());
        sharedAnimation->setAssetId(animationId);
        shared_ptr<Instance> animationTrackInstance = loadAnimation(sharedAnimation);
        if (shared_ptr<AnimationTrack> animationTrack = boost::dynamic_pointer_cast<AnimationTrack>(animationTrackInstance))
        {
            animationTrackMap.insert(std::pair<ContentId, shared_ptr<AnimationTrack> >(animationId, animationTrack));
        }
    }
}

void Animator::replicateAnimationPlay(ContentId animation, float fadeTime, float weight, float speed, shared_ptr<Instance> track)
{
	//replicate the event
    event_OnPlay.replicateEvent(this, animation, fadeTime, weight, speed);

	//call it locally with the track
	tellParentAnimationPlayed(track);
}

void Animator::replicateAnimationStop(ContentId animation, float fadeTime)
{
    event_OnStop.replicateEvent(this, animation, fadeTime);
}

void Animator::replicateAnimationSpeed(ContentId animation, float speed)
{
	event_OnAdjustSpeed.replicateEvent(this, animation, speed);
}

void Animator::replicateAnimationTimePosition(ContentId animation, float timePosition)
{
	event_OnSetTimePosition.replicateEvent(this, animation, timePosition);
}

}
