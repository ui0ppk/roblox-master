/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/NormalId.h"
#include "Util/G3DCore.h"
#include "Util/PV.h"
#include "rbx/Debug.h"
#include "G3D/Array.h"
#include "RbxG3D/RbxRay.h"
#include <limits>

namespace RBX {

	inline int fastFloorInt(float value)
	{
		return value < 0 ? static_cast<int>(value - 0.999f) : static_cast<int>(value);
	}

	inline int fastCeilInt(float value)
	{
		return value < 0 ? static_cast<int>(value) : static_cast<int>(value + 0.999f);
	}

	inline Vector3int16 fastFloorInt16(const Vector3& v)
	{
		return Vector3int16(fastFloorInt(v.x), fastFloorInt(v.y), fastFloorInt(v.z));
	}

	typedef enum  {
		AXIS_X = 0,
		AXIS_Y = 1,
		AXIS_Z = 2
	} AxisIndex;

	namespace Math 
	{
		inline double pi()		{return 3.14159265358979323846;}
		inline double piHalf()	{return pi() * 0.5f;}
		inline double twoPi()	{return pi() * 2.0f;}
		inline float pif()		{return static_cast<float>(pi());}
		inline float piHalff()	{return static_cast<float>(piHalf());}
		inline float twoPif()	{return static_cast<float>(twoPi());}
		inline const float& inf() {
			static const float i = std::numeric_limits<float>::infinity();
			return i;
		}

		// Returns the 0-based most-significant bit (-1 if v is 0)
		inline size_t computeMSB(size_t v)
		{
			size_t msb = -1;
			while (v>0)
			{
				v >>= 1;
				++msb;
			}
			return msb;
		}

		inline int iRound(float value) {
			return G3D::iRound(value);
		}
		inline int iFloor(float value) {
			return G3D::iRound(::floor(value));
		}
		inline float polarity(float value) {
			return  (value >= 0.0f) ? 1.0f : -1.0f;
		}
		inline float sign(float value) {
			return  (value > 0.0f)	
						? 1.0f 
						: (value < 0.0f ? -1.0f : 0.0f);
		}
		
		////////////////////////////////////////////////////////
		// 
		// Denormalized detection

		bool isDenormal(float f);
        bool isNan(float f);
        bool isNan(const Vector3& v);
		bool isNanInf(float f);
		bool isNanInfDenorm(float f);
		bool isNanInfVector3(const Vector3& v);
		bool isNanInfDenormVector3(const Vector3& v);
		bool isNanInfDenormMatrix3(const Matrix3& m);
		bool hasNanOrInf(const CoordinateFrame& c);
		bool hasNanOrInf(const Matrix3& m);

		// Sets denormalized values to 0.0
		bool fixDenorm(float& f);
		bool fixDenorm(Vector3& v);

		////////////////////////////////////////////////////////
		// 
		// fuzzyEq stuff
		inline float epsilonf() { return 1.0e-6f; }
		inline bool fuzzyEq(float a, float b, float epsilon) {
			float aa = fabsf(a) + 1.0f;
		    return (a == b) || (fabsf(a - b) <= (aa * epsilon));
		}

		inline bool fuzzyEq(double a, double b, double epsilon) {
			double aa = fabs(a) + 1.0f;
		    return (a == b) || (fabs(a - b) <= (aa * epsilon));
		}

		bool fuzzyEq(const Vector3& v0, const Vector3& v1, float epsilon = 1.0e-5f);	// Note G3D::eps == 1e-6;
		bool fuzzyEq(const Matrix3& m0, const Matrix3& m1, float epsilon = 1.0e-5f);	// Note G3D::eps == 1e-6;
		bool fuzzyEq(const Matrix4& m0, const Matrix4& m1, float epsilon = 1.0e-5f);	// Note G3D::eps == 1e-6;
		bool fuzzyEq(const CoordinateFrame& c0, const CoordinateFrame& c1, float epsT = 1.0e-5f, float epsRad = 1.0e-5f);
		bool fuzzyAxisAligned(const Matrix3& m0, const Matrix3& m1, float radTolerance);

		////////////////////////////////////////////////////////
		// 
		// odd / even stuff
		inline bool isEven(int value) {
			return ((value % 2) == 0);
		}

		inline bool isOdd(int value) {
			return ((value % 2) != 0);
		}

		inline int nextEven(int value) {
			return (value + 1 + ((value + 1) % 2));
		}

		inline int nextOdd(int value) {
			return (value + 1 + (value % 2));
		}

		////////////////////////////////////////////////////////
		//
		// Vector2 stuff
		inline Vector2 expandVector2(const Vector2& v, int expand) {
			Vector2 answer(v);
			for (int i = 0; i < 2; ++i) {
				answer[i] += expand * RBX::Math::sign(v[i]);
			}
			return answer;
		}
        
        inline Vector2 roundVector2(const Vector2& v) {
            return Vector2(iRound(v.x), iRound(v.y));
        }

		////////////////////////////////////////////////////////
		// 
		// Vector3 stuff

		size_t hash(const Vector3& v);
		bool isIntegerVector3(const Vector3& v);
		Vector3 iRoundVector3(const Vector3& point);
		float angle(const Vector3& v0, const Vector3& v1);
		float smallAngle(const Vector3& v0, const Vector3& v1);
		float elevationAngle(const Vector3& look);
		Vector3 vector3Abs(const Vector3& v);
		float volume(const Vector3& v);
		float maxAxisLength(const Vector3& v);
		Vector3 sortVector3(const Vector3& v);
		Vector3 safeDirection(const Vector3& v);	// handles case where V == vector3::zero();
		Velocity calcTrajectory(const Vector3& launch, const Vector3& target, float speed);
		Vector3 toGrid(const Vector3& v, const Vector3& grid);
		Vector3 toGrid(const Vector3& v, float grid);
		bool lessThan(const Vector3& min, const Vector3& max);
		inline float longestVector3Component(const Vector3& v) {
			return std::max(fabs(v.x), std::max(fabs(v.y), fabs(v.z)));
		}
		inline float planarSize(const Vector3& v) {
			return (v.x < v.y) 
				? ((v.x < v.z) ? v.y * v.z : v.y * v.x)
				: ((v.y < v.z) ? v.x * v.z : v.x * v.y);
		}
		inline float taxiCabMagnitude(const Vector3& v) {
			return fabs(v.x) + fabs(v.y) + fabs(v.z);
		}
		float sumDeltaAxis(const Matrix3& r0, const Matrix3& r1);

		inline const Plane& yPlane() {static Plane p(Vector3(0.0, 1.0, 0.0), Vector3::zero()); return p;}
		Vector3 closestPointOnRay(const RBX::RbxRay& pointOnRay, const RBX::RbxRay& otherRay);
		

		////////////////////////////////////////////////////////
		// 
		// Manipulate/rotate Matrix3 and CoordinateFrame

		Vector3 rotateAboutYGlobal(const Vector3& v, float radians);
		Vector3 toSmallAngles(const Matrix3& matrix);
		Matrix3 snapToAxes(const Matrix3& matrix);
		bool isOrthonormal(const Matrix3& m);
		bool orthonormalizeIfNecessary(Matrix3& m);	// true if an orthonormalize was necessary

		Vector3 toFocusSpace(const Vector3& goal, const CoordinateFrame& focus);
		Vector3 fromFocusSpace(const Vector3& goal, const CoordinateFrame& focus);

		Vector3 toDiagonal(const Matrix3& m);

		inline Matrix3 fromDiagonal(const Vector3& v) {
			return Matrix3(	v[0],	0.0f,	0.0f,
							0.0f,	v[1],	0.0f,
							0.0f,	0.0f,	v[2]	);
		}
		
		// Return the skew symmetric matrix for the the given vector.
		//	a.cross( b ) = A_* b where A_is the skew symmetric matrix for a.
		// 
		inline Matrix3 toSkewSymmetric(const Vector3& v) {
			return Matrix3(	0.0f,	-v.z,	v.y,
							v.z,	0.0f,	-v.x,
							-v.y,	v.x,	0.0f	);
		}

		Matrix3 fromVectorToVectorRotation( const Vector3& fromVec, const Vector3& toVec );
		Matrix3 fromRotationAxisAndAngle( const Vector3& axis, const float& angleRads );
		Matrix3 fromShortestPlanarRotation( const Vector3& targetX, const Vector3& targetY );
		Matrix3 fromDirectionCosines( const Vector3& fromX, const Vector3& fromY, const Vector3& fromZ,
									 const Vector3& toX, const Vector3& toY, const Vector3& toZ );

		inline Vector3 getColumn(const Matrix3& m, int iCol) {
			RBXASSERT_VERY_FAST((0 <= iCol) && (iCol < 3));
			return Vector3(m[0][iCol], m[1][iCol], m[2][iCol]);
		}

		void mulMatrixDiagVector(const Matrix3& _mat, const Vector3& _vec, Matrix3& _answer); 
		void mulMatrixMatrixTranspose(const Matrix3& _m0, const Matrix3& _m1, Matrix3& _answer);
		void mulMatrixTransposeMatrix(const Matrix3& _m0, const Matrix3& _m1, Matrix3& _answer);

		// Byte Angles
		unsigned char rotationToByte(float angle);
		float rotationFromByte(unsigned char byteAngle);

		// Axis Aligned Matrix / OrientId
		static const int maxOrientationId = 36;
		static const int minOrientationId = 0;
		bool isAxisAligned(const Matrix3& matrix);
		int getOrientId(const Matrix3& matrix);
		void idToMatrix3(int orientId, Matrix3& matrix);

		const Matrix3& matrixRotateX();
		//static const Matrix3& matrixRotateNegativeX();
		const Matrix3& matrixRotateY();
		const Matrix3& matrixRotateNegativeY();
		const Matrix3& matrixTiltZ();
		const Matrix3& matrixTiltNegativeZ();
		const Matrix3 matrixTiltQuadrant(int quadrant);
		void rotateMatrixAboutX90(Matrix3& matrix, int times = 1);
		void rotateMatrixAboutY90(Matrix3& matrix, int times = 1);
		void rotateMatrixAboutZ90(Matrix3& matrix);
		Matrix3 rotateAboutZ(const Matrix3& matrix, float radians);
		Matrix3 getWellFormedRotForZVector(const Vector3& vec);
		Matrix3 momentToObjectSpace(const Matrix3& iWorld, const Matrix3& bodyRotation);
		Matrix3 momentToWorldSpace(const Matrix3& iBody, const Matrix3& bodyRotation);
		Matrix3 getIWorldAtPoint(const Vector3& cofmPos,
							   const Vector3& worldPos,
							   const Matrix3& iWorldAtCofm,
							   float mass);
		Matrix3 getIBodyAtPoint(const Vector3& pos,
							const Matrix3& iBody,
							float mass);

		// CoordinateFrame
		void rotateAboutYLocal(CoordinateFrame& c, float radians);
		void rotateAboutYGlobal(CoordinateFrame& c, float radians);
		CoordinateFrame snapToGrid(const CoordinateFrame& snap, float grid);
		CoordinateFrame snapToGrid(const CoordinateFrame& snap, const Vector3& grid);
		// http://www.vlfeat.org/api/mathop_8h-source.html#l00227
		inline float atan2Fast(float y, float x) {
			float angle, r;
			float const c3 = 0.1821f;
			float const c1 = 0.9675f;
			float abs_y = fabsf(y) + 1.19209290e-07f;
			if (x >= 0) {
				r = (x - abs_y) / (x + abs_y) ;
				angle = Math::pif() / 4.0f;
			} else {
				r = (x + abs_y) / (abs_y - x) ;
				angle = 3.0f * Math::pif() / 4.0f ;
			} 
			angle += (c3*r*r - c1) * r ; 
			return (y < 0) ? - angle : angle ;
		}

		inline float zAxisAngle(const Matrix3& matrix) {
			Vector3 look = matrix.column(0);
			float angle = (float) atan2(look.y, look.x);
//			float angle = Math::atan2Fast(look.y, look.x);
			return angle;
		}
		void pan(const Vector3& focusPosition, CoordinateFrame& camera, float radians);

		// std::vector
		void lerpArray(
			const G3D::Array<float>& before,
			const G3D::Array<float>& after,
			G3D::Array<float>& answer,
			float alpha);

		// Pitch, Yaw stuff - replaces Euler Angles
		int radiansToQuadrant(float radians);
		int radiansToOctant(float radians);
		inline float radiansToDegrees(float radians)	{return radians * (180.0f / Math::pif());}
		inline float degreesToRadians(float degrees)	{return degrees * (Math::pif() / 180.0f);}

		/**
		 Returns the heading as an angle in radians, where
		north is 0 and west is PI/2
		North == -z

		Elevation is angle above (+) or below(-) horizon
		 */
		inline float getHeading(const Vector3& look) { return atan2( -look.x, -look.z); }
		inline float getElevation(const Vector3& look) { return asin(look.y); }

		void getHeadingElevation(const CoordinateFrame& c, float& heading, float& elevation);
		void setHeadingElevation(CoordinateFrame& c, float heading, float elevation);

		CoordinateFrame getFocusSpace(const CoordinateFrame& focus);

		int toYAxisQuadrant(const CoordinateFrame& c);		// 0..3

		Matrix3 alignAxesClosest(const Matrix3& align, const Matrix3& target);

		// NormalId stuff
		NormalId getClosestObjectNormalId(const Vector3& worldV, const Matrix3& objectR);

		inline Vector3 getWorldNormal(NormalId objId, const Matrix3& objectR) {
//			return (objId < 3) ? getColumn(objectR, objId) : -getColumn(objectR, objId - 3);
			int column = objId % 3;
			int polarity = ((objId / 3) * (-2)) + 1;
			return polarity * getColumn(objectR, column);
		}

		inline Vector3 getWorldNormal(NormalId objId, const CoordinateFrame& objectC) {
			return getWorldNormal(objId, objectC.rotation);
		}

		// wraps from -pi to pi

		float deltaRotationClose(float aRot, float bRot);		// computes aRot - bRot, assuming angles are close.  Undoes wrapping
		float averageRotationClose(float aRot, float bRot);		// computes average aRot, bRot, assuming angles are close.  Undoes wrapping

		double advanceWoundRotation(double currentRotationWound, double newRotationNotWound);	// properly increments a wound up rotation - detects flips
		float clampRotationClose(float rot, float limitLo, float limitHi);
		// -3pi to -pi: -1
		// -pi to pi: 0
		// pi to 3pi: 1
		inline double windingPart(double rad) {
			return ::floor((rad + pi()) / twoPi()) ;		
		}

		inline float radWrap(double rad) {			// extra part
			if ((rad >= -pi()) && (rad < pi())) {
				return static_cast<float>(rad);
			}
			double answer = rad - (twoPi() * windingPart(rad));
			RBXASSERT((answer >= -pi()) && (answer <= pi()));
			return static_cast<float>(answer);
		}

	    // Matrix operations
		const Matrix3& getAxisRotationMatrix(int face);

		// Vector to Object Space
		// == mat.transpose() * vec
		inline Vector3 vectorToObjectSpace(const Vector3& vec, const Matrix3& mat);

		// Ray, Line
		bool clipRay(Vector3& origin, Vector3& ray, Vector3 box[], Vector3& endPoint); 
		bool intersectLinePlane(const Line& line, const Plane& plane, Vector3& hit);
		bool intersectRayPlane(const RbxRay& ray, const Plane& plane, Vector3& hit);
        bool intersectRayConvexPolygon(const RBX::RbxRay& ray,  const std::vector<Vector3>& poly, Vector3& hit, bool oneSided);
		bool lineSegmentDistanceIfCrossing(const Vector3& line1Pt1, const Vector3& line1Pt2, const Vector3& line2Pt1, const Vector3& line2Pt2, float& distance, float adjustEdgeTol = 0.0f);
        std::vector<Vector3> spatialPolygonIntersection(const std::vector<Vector3>& polyA,  const std::vector<Vector3>& polyB);
		std::vector<Vector2> planarPolygonIntersection(const std::vector<Vector2>& poly1, const std::vector<Vector2>& poly2);


		// Misc.
		float computeLaunchAngle(float v, float x, float y, float g);
		Vector2 polygonStartingPoint(int numSides, float maxWidth);
		bool evenWholeNumber( const float& rawInput );
		bool evenWholeNumberFuzzy( const float& rawInput );
	}
}	// namespace

#include "Math.inl"
