#pragma once

#include "Util/ContentId.h"

namespace RBX {

	class AnimationId : public ContentId
	{
	public:
		AnimationId(const ContentId& id):ContentId(id) {}
		AnimationId(const char* id):ContentId(id) {}
		AnimationId(const std::string& id):ContentId(id) {}
		AnimationId() {}

		bool isActive() const { return toString().substr(0, 9)=="active://"; }

		static AnimationId nullAnimation() {
			static AnimationId t;				// note - the name in the contentId will get a boost call_once
			return t;
		}
	};
}