/* Copyright 2003-2013 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/SteppedInstance.h"
#include "Util/RunStateOwner.h"

namespace RBX {
	
//	class Workspace;
//	class Primitive;
//	class PartInstance;
	class Animator;

	extern const char* const sAnimationController;
	class AnimationController : public DescribedCreatable<AnimationController, Instance, sAnimationController>
					, public IStepped
	{
	private:
		typedef DescribedCreatable<AnimationController, Instance, sAnimationController> Super;

		shared_ptr<Animator> animator;
		Animator* getAnimator();

		// Instance
		/*override*/ bool askSetParent(const Instance* instance) const;
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

		// IStepped
		/*override*/ void onStepped(const Stepped& event);

	public:
		AnimationController();
		virtual ~AnimationController();

		shared_ptr<Instance> loadAnimation(shared_ptr<Instance> animation);
		shared_ptr<const Reflection::ValueArray> getPlayingAnimationTracks();

		rbx::signal<void(shared_ptr<Instance>)> animationPlayedSignal;
	};

}	// namespace RBX
