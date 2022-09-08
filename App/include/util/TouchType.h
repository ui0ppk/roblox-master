/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once


namespace RBX {

	class PVInstance;

	class TouchType {
	private:
		friend class PVInstance;
		typedef enum {NOTHING, TOUCH_ONLY, INTERSECT} Status;
		Status	worldExtents;
		Status	objects;

	public:
		TouchType()	: 
			worldExtents(NOTHING),
			objects(NOTHING)
			{}

		bool intersectingWorldExtents() {
			return (worldExtents == INTERSECT);
		}
		bool intersectingObject() {
			return (objects == INTERSECT);
		}
		bool intersecting() {
			return (intersectingWorldExtents() || intersectingObject());
		}
		bool touchingWorldExtents() {
			return (worldExtents == TOUCH_ONLY);
		}
		bool touchingObject() {
			return (objects == TOUCH_ONLY);
		}
		bool touching() {
			return (touchingWorldExtents() || touchingObject());
		}
		bool touchingOnlyWorldExtents() {
			return (touchingWorldExtents() && !touchingObject());
		}
		bool nothingFound() {
			return ((worldExtents == NOTHING) && (objects == NOTHING));
		}
		bool touchingNotIntersecting() {
			return (touching() && (!intersecting()));
		}
		bool somethingFound() {
			return (!nothingFound());
		}
		bool intersectsOtherOnly() {
			return ((objects == INTERSECT) && (worldExtents == NOTHING));
		}
	};
} // namespace