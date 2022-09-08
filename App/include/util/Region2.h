/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#pragma once

#include "Util/Rect.h"
#include "rbx/Debug.h"
#include "Util/Math.h"

namespace RBX {

	class Adorn;

	class Region2 {
	public:
		class WeightedPoint {
		public:
			Vector2 point;
			float radius;

			WeightedPoint()
				: point(Vector2::zero())
				, radius(0.0f)
			{}

			WeightedPoint(const Vector2& point, const float radius)
				: point(point)
				, radius(radius)
			{}
		};

	private:
		WeightedPoint owner;
		G3D::Array<WeightedPoint> others;

		bool findCloserOther(const Vector2& point, const float slop) const;

	public:
		void clearEmpty() {
			owner = WeightedPoint();
			others.fastClear();
		}

		bool isEmpty() const {
			return (owner.radius <= 0.0f);
		}

		Region2() 
		{}

		~Region2() {}

		void setOwner(const WeightedPoint& _owner) {
			owner = _owner;
		}

		void appendOther(const WeightedPoint& _other) {
			others.append(_other);
		}

		bool contains(const Vector2& pos2d, const float slop) const;

		static float getRelativeError(const Vector2& pos2d, const WeightedPoint& owner);		// go through all owner points - find best one

		static bool pointInRange(const Vector2& pos2d, const WeightedPoint& owner, const float slop);

		static bool closerToOtherPoint(	const Vector2& pos2d,
										const WeightedPoint& owner,
										const WeightedPoint& other,
										float slop);
	};
}