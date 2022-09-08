/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#pragma once

#include "Util/G3DCore.h"
#include "G3D/Rect2D.h"


namespace RBX {

	// TODO: Replace with G3D::Rect2D
	class Rect {
	public:
		typedef enum {TOP, BOTTOM, LEFT, RIGHT, CENTER, NONE} Location;

		static bool legalX(Location loc) {return ((loc != TOP) && (loc != BOTTOM));}
		static bool legalY(Location loc) {return ((loc != LEFT) && (loc != RIGHT));}

		Vector2		low;		// top left
		Vector2		high;		// bottom right

		Rect() : low(0,0), high(0,0) {}

		Rect(Rect2D r) : low(r.x0y0()), high(r.x1y1()) {}

		Rect2D toRect2D() const {return Rect2D::xyxy(low, high);}

		Rect(float left, float top, float right, float bottom) :
				low(left, top), high(right, bottom) {}

		Rect(const Vector2& _high) :
				low(Vector2::zero()), high(_high) {}

		Rect(const Vector2& _low, const Vector2& _high) :
				low(_low), high(_high) {}

		static Rect fromLowSize(const Vector2& _low, const Vector2& _size) {
			return Rect(_low, _low + _size);
		}

		static Rect xywh(float x, float y, float w, float h) {
			return Rect(x,y,x+w,y+h);
		}

		static Rect fromCenterSize(const Vector2& _center, const Vector2& _size) {
			Vector2 halfSize = _size * 0.5f;
			return Rect(_center - halfSize, _center + halfSize);
		}

		static Rect fromCenterSize(const Vector2& _center, float _size) {
			return fromCenterSize(_center, Vector2(_size, _size));
		}

		void unionWith(const Rect& other);

		void unionWith(const Vector2& point) {
			unionWith(Rect(point, point));
		}
	
		bool operator== (const Rect& other) const {
		    return ((low == other.low) && (high == other.high));
		}

		bool operator!= (const Rect& other) const {
		    return ((low != other.low) || (high != other.high));
		}
    
		bool contains(const Vector2& xz) const {
			return ((xz.x >= low.x) && (xz.x <= high.x) && (xz.y >= low.y) && (xz.y <= high.y));
		}

		bool pointInRect(int x, int y) const {
			return ((x >= low.x) && (x <= high.x) && (y >= low.y) && (y <= high.y));
		}
		bool pointInRect(Vector2int16 point) const {
			return pointInRect(point.x, point.y);
		}

		Vector2 size() const {
			return (high - low);
		}

		Vector2 center() const {
			return (low + high) * 0.5;
		}

		Location pointInBorder(const Vector2& point, float borderRatio);

		Vector2 positionPoint(Location xLoc, Location yLoc) const;

		Vector2 positionPoint(const Vector2& point, Location xLoc, Location yLoc) const;

		Rect positionChild(const Rect& child, Location xLoc, Location yLoc) const;

		Rect inset(int dx) {
			return Rect(low.x+dx, low.y+dx, high.x-dx, high.y-dx);
		}
		Rect inset(const Vector2int16& dd) {
			return Rect(low.x+dd.x, low.y+dd.y, high.x-dd.x, high.y-dd.y);
		}

		Vector2 clamp(const Vector2& point) {
			return point.clamp(low, high);
		}

		static const float BORDER_RATIO;
		static const float BORDER_RATIO_DRAG;
		static const float BORDER_RATIO_THIN;
	};
}