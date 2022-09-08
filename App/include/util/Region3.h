/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#pragma once

#include "G3D/Vector3.h"
#include "G3D/CoordinateFrame.h"

namespace RBX {
	class Extents;

	class Region3 {
	private:
		G3D::CoordinateFrame cframe;
		Vector3 size;
		void init(const Extents &extents);

	public:
		Region3();
		Region3(const Vector3& min, const Vector3& max);
		explicit Region3(const Extents &extents);

		~Region3() {}

		const G3D::CoordinateFrame& getCFrame() const { return cframe; }
		const Vector3& getSize() const { return size; }

		Vector3 minPos() const;
		Vector3 maxPos() const;

		inline bool operator==(const Region3& other) const {
			return (size == other.size) && (cframe == other.cframe);
		}

		inline bool operator!=(const Region3& other) const {
			return !(*this == other);
		}
	};
}