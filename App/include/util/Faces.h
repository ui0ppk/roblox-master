/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/NormalId.h"

namespace RBX {
	
	//A utility class for holding a set of "Faces" associated with an object (top, bottom, left, right, front, back)
	class Faces
	{
	public:
		Faces(int normalIdMask = 0);
		void clear() { normalIdMask = NORM_NONE_MASK; }
		void setNormalId(NormalId normalId, bool value);
		bool getNormalId(NormalId normalId) const;


		bool operator==(const Faces& other) const {
			return normalIdMask == other.normalIdMask;
		}
		bool operator!=(const Faces& other) const {
			return normalIdMask != other.normalIdMask;
		}


		int normalIdMask;
	};
}
