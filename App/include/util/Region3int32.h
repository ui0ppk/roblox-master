/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#pragma once

#include "Util/Vector3int32.h"

namespace RBX {

	class Region3int32 {
	private:
		Vector3int32 minPos;
		Vector3int32 maxPos;

	public:
		Region3int32();
		Region3int32(const Vector3int32& min, const Vector3int32& max);

		~Region3int32() {}

		Vector3int32 getMinPos() const;
		Vector3int32 getMaxPos() const;

		inline bool operator==(const Region3int32& other) const {
			return (minPos == other.minPos) && (maxPos == other.maxPos);
		}

		inline bool operator!=(const Region3int32& other) const {
			return !(*this == other);
		}
	};
}