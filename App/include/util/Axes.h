/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/NormalId.h"

namespace RBX {
	
	//A utility class for holding a set of "Faces" associated with an object (top, bottom, left, right, front, back)
	class Axes
	{
	public:
		static int axisToMask(Vector3::Axis axis);
		static Vector3::Axis normalIdToAxis(NormalId normalId);
		static NormalId		axisToNormalId(Vector3::Axis axis);

	public:
		Axes(int axisMask = 0);
		void clear() { axisMask = 0; }
		void setAxisByNormalId(NormalId normalId, bool value);
		bool getAxisByNormalId(NormalId normalId) const;
		void setAxis(Vector3::Axis axis, bool value);
		bool getAxis(Vector3::Axis axis) const;



		bool operator==(const Axes& other) const {
			return axisMask == other.axisMask;
		}
		bool operator!=(const Axes& other) const {
			return axisMask != other.axisMask;
		}


		int axisMask;
	};
}
