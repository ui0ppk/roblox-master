/**
 @file Line.h
 
 Line class
 
 @maintainer Morgan McGuire, http://graphics.cs.williams.edu
 
 @created 2001-06-02
 @edited  2006-02-28
 */

#ifndef G3D_LINE_H
#define G3D_LINE_H

#include "G3D/platform.h"
#include "G3D/Vector3.h"

namespace G3D {

class Plane;

/**
 An infinite 3D line.
 */
class Line {
protected:

    Vector3 _point;
    Vector3 _direction;

    Line(const Vector3& point, const Vector3& direction) {
        _point     = point;
        _direction = direction.direction();
    }

public:

    /** Undefined (provided for creating Array<Line> only) */
    inline Line() {}

    virtual ~Line() {}

    /**
      Constructs a line from two (not equal) points.
     */
    static Line fromTwoPoints(const Vector3 &point1, const Vector3 &point2) {
        // ROBLOX
		//return Line(point1, point2 - point1);
		return Line(point1, (point2 - point1).direction());
		// =====

	}

    /**
      Creates a line from a point and a (nonzero) direction.
     */
    static Line fromPointAndDirection(const Vector3& point, const Vector3& direction) {
        //ROBLOX
		//return Line(point, direction);
		  return Line(point, direction.direction());
		// =====
    }
	
	// ROBLOX
	static Line fromPointAndUnitDirection(const Vector3& point, const Vector3& unitDirection) {
        return Line(point, unitDirection);
    }
	//=====

    /**
      Returns the closest point on the line to point.
     */
    Vector3 closestPoint(const Vector3& pt) const;

    /**
      Returns the distance between point and the line
     */
    double distance(const Vector3& point) const {
        return (closestPoint(point) - point).magnitude();
    }

    /** Returns a point on the line */
    Vector3 point() const;

    /** Returns the direction (or negative direction) of the line */
    Vector3 direction() const;

    /**
     Returns the point where the line and plane intersect.  If there
     is no intersection, returns a point at infinity.
     */
    Vector3 intersection(const Plane &plane) const;


    /** Finds the closest point to the two lines.
        
        @param minDist Returns the minimum distance between the lines.

        @cite http://objectmix.com/graphics/133793-coordinates-closest-points-pair-skew-lines.html
    */
    Vector3 closestPoint(const Line& B, float& minDist) const;

    inline Vector3 closestPoint(const Line& B) const {
        float m;
        return closestPoint(B, m);
    }
	// ROBLOX
	bool static parallel(const Line& line0, const Line& line1, float epsilon) {
		Vector3 crossAxis = line0.direction().cross(line1.direction());
		return (crossAxis.squaredMagnitude() < (epsilon * epsilon));
	}

	// Returns false if unable to find closest points (i.e. lines are parallel)
	static bool closestPoints(const Line& line0, const Line& line1, Vector3& p0, Vector3& p1) 
	{
		const Vector3& pa = line0.point();
		const Vector3& pb = line1.point();

		const Vector3& ua = line0.direction();
		const Vector3& ub = line1.direction();

		Vector3 p = pb - pa;
		float uaub = ua.dot(ub);
		float q1 = ua.dot(p);
		float q2 = -ub.dot(p);
		float d = 1 - uaub*uaub;
		if (d > 1e-6f) {
			d = 1.0f / d;
			float distance0 = (q1 + uaub*q2)*d;
			float distance1 = (uaub*q1 + q2)*d;
			p0 = pa + distance0 * ua;
			p1 = pb + distance1 * ub;
			return true;
		}
		else {
			p0 = pa;
			p1 = pb;
			return false;
		}
	}
	// =============================

};

};// namespace


#endif
