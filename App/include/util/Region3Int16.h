/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#pragma once

#include "G3D/Vector3int16.h"
#include "G3DCore.h"

namespace RBX {

	class Region3int16 {
	private:
		Vector3int16 minPos;
		Vector3int16 maxPos;

	public:
		Region3int16()
        {
        }

		Region3int16(const Vector3int16& min, const Vector3int16& max)
            : minPos(min)
            , maxPos(max)
        {
        }

		const Vector3int16& getMinPos() const
        {
            return minPos;
        }

		const Vector3int16& getMaxPos() const
        {
            return maxPos;
        }

		bool operator==(const Region3int16& other) const
        {
			return (minPos == other.minPos) && (maxPos == other.maxPos);
		}

		bool operator!=(const Region3int16& other) const
        {
			return !(*this == other);
		}

        bool contains(const Vector3int16& p) const
        {
            return
                static_cast<unsigned int>(p.x - minPos.x) <= static_cast<unsigned int>(maxPos.x - minPos.x) &&
                static_cast<unsigned int>(p.y - minPos.y) <= static_cast<unsigned int>(maxPos.y - minPos.y) &&
                static_cast<unsigned int>(p.z - minPos.z) <= static_cast<unsigned int>(maxPos.z - minPos.z);
        }

        bool empty() const
        {
            return minPos.x > maxPos.x || minPos.y > maxPos.y || minPos.z > maxPos.z;
        }
	};
}