/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#pragma once

#include "V8Tree/Instance.h"
#include "Util/AnimationId.h"

namespace RBX {
	extern const char *const sAnimation;

	class KeyframeSequence;
	class Animation
		: public DescribedCreatable<Animation, Instance, sAnimation>
	{
	private:
		typedef DescribedCreatable<Animation, Instance, sAnimation> Super;
		ContentId assetId;
	public:
		Animation();

		shared_ptr<const KeyframeSequence> getKeyframeSequence() const;

		shared_ptr<const KeyframeSequence> getKeyframeSequence(const Instance* context) const;

		AnimationId getAssetId() const { return assetId; } 
		void setAssetId(AnimationId value);

		bool isEmbeddedAsset() const;

		/*override*/ bool askSetParent(const Instance* instance) const { return true; }
		/*override*/ int getPersistentDataCost() const 
		{
			return Super::getPersistentDataCost() + Instance::computeStringCost(getAssetId().toString());
		}
	};

} // namespace
