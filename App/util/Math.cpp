/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"


#include "Util/Math.h"
#include "Util/PV.h"
#include "Util/Units.h"
#include "RbxG3D/RbxRay.h"
#include "rbx/Debug.h"
#include "V8DataModel/Camera.h"
#include "boost/functional/hash/hash.hpp"
#include "boost/math/special_functions/fpclassify.hpp"

extern "C"
{
	#include "Util/gpc.h"
}

namespace G3D
{
	std::size_t hash_value(const G3D::Vector3& v)
	{
		return v.hashCode();
	}

	std::size_t hash_value(const G3D::Vector3int16& v)
	{
		return Vector3int16::boost_compatible_hash_value()(v);
	}
}

namespace RBX {

const float kEpsilon = 1e-3;


float Math::sumDeltaAxis(const Matrix3& r0, const Matrix3& r1)
{
	float answer = 0.0f;
	for (int i = 0; i < 3; ++i) {		// using row instead of columns (true vectors) - should give similar results
		Vector3 v0 = Math::getColumn(r0, i);
		Vector3 v1 = Math::getColumn(r1, i);
		answer += Math::taxiCabMagnitude(v0 - v1);
	}
	return answer;
}


// = _mat * _mat(diagVector) * _mat
void Math::mulMatrixDiagVector(const Matrix3& _mat, const Vector3& _vec, Matrix3& _answer) 
{
	const float* vec = &_vec[0];
	const float* mat = &_mat[0][0];

	float* answer = _answer[0];
	answer[0] = mat[0]*vec[0];
	answer[1] = mat[1]*vec[1];
	answer[2] = mat[2]*vec[2];
	answer[3] = mat[3]*vec[0];
	answer[4] = mat[4]*vec[1];
	answer[5] = mat[5]*vec[2];
	answer[6] = mat[6]*vec[0];
	answer[7] = mat[7]*vec[1];
	answer[8] = mat[8]*vec[2];
}

// answer = m0 * m1.transpose
void Math::mulMatrixMatrixTranspose(const Matrix3& _m0, const Matrix3& _m1, Matrix3& _answer) 
{
	float* answer = &_answer[0][0];
	const float* m0 = &_m0[0][0];
	const float* m1 = &_m1[0][0];
    for (int iRow = 0; iRow < 3; iRow++) {
        for (int iCol = 0; iCol < 3; iCol++) {
            answer[iRow*3+iCol] =
                m0[iRow*3  ] * m1[iCol*3  ] +
                m0[iRow*3+1] * m1[iCol*3+1] +
                m0[iRow*3+2] * m1[iCol*3+2];
        }
    }
}

// answer = m0.transpose() * m1;
void Math::mulMatrixTransposeMatrix(const Matrix3& _m0, const Matrix3& _m1, Matrix3& _answer) 
{
	float* answer = &_answer[0][0];
	const float* m0 = &_m0[0][0];
	const float* m1 = &_m1[0][0];
    for (int iRow = 0; iRow < 3; iRow++) {
        for (int iCol = 0; iCol < 3; iCol++) {
            answer[iRow*3+iCol] =
                m0[iRow] * m1[iCol] +
                m0[iRow+3] * m1[iCol+3] +
                m0[iRow+6] * m1[iCol+6];
        }
    }
}





/*
	Quadrants:

			pi/2
	2		|		1
			|
pi	__________________  0rads
-pi			|
			|
	3		|		4
			-pi/2
*/

int getQuadrant(double angle)
{
	RBXASSERT((angle >= -Math::pi()) && (angle <= Math::pi()));
	if (angle > Math::piHalf()) {
		return 2;
	}
	else if (angle > 0.0) {
		return 1;
	}
	else if (angle < -Math::piHalf()) {
		return 3;
	}
	else {
		return 4;
	}
}

float Math::deltaRotationClose(float aRot, float bRot)		// computes aRot - bRot, assuming angles are close.  Undoes wrapping
{
	float delta = aRot - bRot;
	return static_cast<float>(radWrap(delta));
}

float Math::averageRotationClose(float aRot, float bRot)		// computes average aRot, bRot, assuming angles are close.  Undoes wrapping
{
	float answer = bRot + 0.5f * deltaRotationClose(aRot, bRot);
	return static_cast<float>(radWrap(answer));
}

float Math::clampRotationClose(float rot, float limitLo, float limitHi)
{
	RBXASSERT(G3D::fuzzyGe(limitLo, -Math::pif()) && G3D::fuzzyLe(limitLo, Math::pif()));
	RBXASSERT(G3D::fuzzyGe(limitHi, -Math::pif()) && G3D::fuzzyLe(limitHi, Math::pif()));
	rot = radWrap(rot);
	if ( limitLo <= limitHi )
	{
		if ( rot < limitLo )
		{
			if ( limitLo - rot < rot + Math::twoPi() - limitHi )
				return limitLo;
			else
				return limitHi;
		} else if ( rot > limitHi )
		{
			if (rot - limitHi < limitLo + Math::twoPi() - rot )
				return limitHi;
			else
				return limitLo;
		} else
			return rot;
	} else
	{
		if ( rot >= limitLo || rot <= limitHi )
			return rot;
		else 
		{	
			// limitHi < rot < limitLo
			if ( limitLo - rot < rot - limitHi )
				return limitLo;
			else
				return limitHi;
		}
	}
}

double Math::advanceWoundRotation(double currentRotationWound, double newRotationNotWound)
{
	RBXASSERT((newRotationNotWound >= -pi()) && (newRotationNotWound <= pi()));

	double windings = windingPart(currentRotationWound);
	double extra = radWrap(currentRotationWound);

	int currentQuadrant = getQuadrant(extra);
	int newQuadrant = getQuadrant(newRotationNotWound);

	if ((currentQuadrant == 2) && (newQuadrant == 3)) {					// increasing angle over the threshold
		windings++;
	}
	
	if ((currentQuadrant == 3) && (newQuadrant == 2)) {
		windings--;
	}

	double answer = windings * twoPi() + newRotationNotWound;

	RBXASSERT(fabs(answer - currentRotationWound) < 0.5);

	return answer;
}

/*
	Focus space - defined as a coordinate frame where:

	1.  Origin == focus origin
	2.	elevation == 0
	3.  heading == focus heading
*/


CoordinateFrame Math::getFocusSpace(const CoordinateFrame& focus)
{
	float heading, elevation;
	getHeadingElevation(focus, heading, elevation);

	CoordinateFrame answer(focus);
	setHeadingElevation(answer, heading, 0.0f);

	return answer;
}

void Math::lerpArray(const G3D::Array<float>& before,
					const G3D::Array<float>& after,
					G3D::Array<float>& answer,
					float alpha)
{
	RBXASSERT(before.size() == after.size());
	RBXASSERT(before.size() == answer.size());
	for (int i = 0; i < before.size(); ++i)
	{
		answer[i] = G3D::lerp(before[i], after[i], alpha);
	}
}

bool Math::lessThan(const Vector3& min, const Vector3& max)
{
	return (	(min.x < max.x)
			&&	(min.y < max.y)
			&&	(min.z < max.z)	);
}


bool Math::isDenormal(float f)
{
	return (boost::math::fpclassify<float>(f) == FP_SUBNORMAL);
}

bool Math::isNanInf(float f)
{
	return !boost::math::isfinite<float>(f);
}


bool Math::isNanInfDenorm(float f)
{
	int fpClass = boost::math::fpclassify<float>(f);
	return ((fpClass != FP_ZERO) && (fpClass != FP_NORMAL));
}


bool Math::isNanInfVector3(const Vector3& v)
{
	return (isNanInf(v.x) || isNanInf(v.y) || isNanInf(v.z));
}

bool Math::isNanInfDenormVector3(const Vector3& v)
{
	return (isNanInfDenorm(v.x) || isNanInfDenorm(v.y) || isNanInfDenorm(v.z));
}

bool Math::isNanInfDenormMatrix3(const Matrix3& m)
{
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			if (isNanInfDenorm(m[i][j])) {
				return true;
			}
		}
	}
	return false;
}

bool Math::isNan(float f)
{
    return boost::math::isnan<float>(f);
}

bool Math::isNan(const Vector3& v)
{
    return (isNan(v.x) || isNan(v.y) || isNan(v.z));
}

bool Math::hasNanOrInf(const CoordinateFrame& c)
{
	for (int i = 0; i < 3; ++i)
		if (Math::isNanInf(c.translation[i]))
			return true;
	return hasNanOrInf(c.rotation);
}

bool Math::hasNanOrInf(const Matrix3& m)
{
	for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 3; ++j)
			if (Math::isNanInf(m[i][j]))
				return true;
	return false;
}

bool Math::fixDenorm(float& f)
{
	if (isDenormal(f))
	{
		f = 0;
		return true;
	}
	return false;
}

bool Math::fixDenorm(Vector3& v)
{
	bool result = false;
	if (isDenormal(v.x))
	{
		v.x = 0;
		result = true;
	}
	if (isDenormal(v.y))
	{
		v.y = 0;
		result = true;
	}
	if (isDenormal(v.z))
	{
		v.z = 0;
		result = true;
	}
	return result;
}


//////////////////////////////////////////////////////////////

bool Math::isIntegerVector3(const Vector3& v)
{
	return (
				(v.x == floor(v.x))
			&&	(v.y == floor(v.y))
			&&	(v.z == floor(v.z))
			);
}

size_t Math::hash(const Vector3& v)
{
	return v.hashCode();
}


///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////



float segSizeRadians()
{
	static float s = (Math::twoPif() / 256.0f);
	return s;
}

/*
	0 == -pi
	64 == -pi/2
	128 = 0
	192 = pi/2
	255 = ~ pi
*/

unsigned char rotationToByteBase(float angle)
{
	RBXASSERT(angle <= (Math::pif()+0.0001f));
	RBXASSERT(angle >= -(Math::pif()+0.0001f));

	float fAngle = (angle + Math::pif()) / segSizeRadians();

	int iAngle = Math::iRound(fAngle);
	RBXASSERT(iAngle >= -1);
	RBXASSERT(iAngle <= 256);

	iAngle = std::max(0, iAngle);
	iAngle = std::min(255, iAngle);

	unsigned char answer = static_cast<unsigned char>(iAngle);	
	return answer;
}

float rotationFromByteBase(unsigned char byteAngle)
{
	float fSegs = byteAngle;
	float answer = (fSegs * segSizeRadians()) - Math::pif();
	return answer;
}


unsigned char Math::rotationToByte(float angle)
{
	float wrapped = static_cast<float>(Math::radWrap(angle));
	unsigned char answer = rotationToByteBase(wrapped);

#ifdef _DEBUG
	float repeat = rotationFromByteBase(answer);
	float delta = fabs(repeat - wrapped);
	RBXASSERT(delta <= segSizeRadians());
#endif

	return answer;
}


float Math::rotationFromByte(unsigned char byteAngle)
{
	float answer = rotationFromByteBase(byteAngle);

#ifdef _DEBUG
	unsigned char repeat = rotationToByteBase(answer);
	RBXASSERT(repeat == byteAngle);
#endif

	return answer;
}

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

// see  http://kwon3d.com/theory/moi/triten.html
Matrix3 Math::getIWorldAtPoint(const Vector3& cofmPos,
							   const Vector3& worldPos,
							   const Matrix3& iWorldAtCofm,
							   float mass)
{
	Vector3 d = worldPos - cofmPos;			//pos;
	float x2 = d.x * d.x;
	float y2 = d.y * d.y;
	float z2 = d.z * d.z;
	float xy = d.x * d.y;
	float xz = d.x * d.z;
	float yz = d.y * d.z;
	Matrix3 answer = 
			iWorldAtCofm
		+	mass		 * Matrix3(	y2 + z2,	-xy,		-xz,
										-xy,	x2 + z2,	-yz,
										-xz,	-yz,		x2 + y2	);
	return answer;
}

// see  http://kwon3d.com/theory/moi/triten.html
Matrix3 Math::getIBodyAtPoint(const Vector3& p,
							  const Matrix3& iBody,
							  float mass)
{
	float x2 = p.x * p.x;
	float y2 = p.y * p.y;
	float z2 = p.z * p.z;
	float xy = p.x * p.y;
	float xz = p.x * p.z;
	float yz = p.y * p.z;
	Matrix3 answer = 
			iBody
		+	mass		 * Matrix3(	y2 + z2,	-xy,		-xz,
										-xy,	x2 + z2,	-yz,
										-xz,	-yz,		x2 + y2	);

	return answer;
}

// see  http://kwon3d.com/theory/moi/triten.html
Matrix3 Math::momentToObjectSpace(const Matrix3& iWorld, const Matrix3& bodyRotation)
{
	return bodyRotation.transpose() * iWorld * bodyRotation;
}


Matrix3 Math::momentToWorldSpace(const Matrix3& iBody, const Matrix3& bodyRotation)
{
	return bodyRotation * iBody * bodyRotation.transpose();
}


Vector3 Math::toDiagonal(const Matrix3& m)
{
	return Vector3(m[0][0], m[1][1], m[2][2]);
}

Matrix3 Math::fromVectorToVectorRotation( const Vector3& fromVec, const Vector3& toVec )
{
	Matrix3 rotMatrix = Matrix3::identity();
	Vector3 rotationAxis = fromVec.cross(toVec);

	if( rotationAxis.magnitude() > kEpsilon )
	{
		float rotAngle = acos(fromVec.dot(toVec)/(fromVec.magnitude() * toVec.magnitude()));
		rotationAxis /= rotationAxis.magnitude();
		rotMatrix = fromRotationAxisAndAngle(rotationAxis, rotAngle);
	}

	return rotMatrix;
}

Matrix3 Math::fromRotationAxisAndAngle( const Vector3& axis, const float& angleRads )
{
	Matrix3 rotMatrix = Matrix3::identity();

	if( axis.magnitude() > kEpsilon )
	{
		float r00 = 1 + (1-cos(angleRads))*(axis.x*axis.x-1);
		float r01 = -axis.z*sin(angleRads)+(1-cos(angleRads))*axis.x*axis.y;
		float r02 = axis.y*sin(angleRads)+(1-cos(angleRads))*axis.x*axis.z;
		float r10 = axis.z*sin(angleRads)+(1-cos(angleRads))*axis.x*axis.y;
		float r11 = 1 + (1-cos(angleRads))*(axis.y*axis.y-1);
		float r12 = -axis.x*sin(angleRads)+(1-cos(angleRads))*axis.y*axis.z;
		float r20 = -axis.y*sin(angleRads)+(1-cos(angleRads))*axis.x*axis.z;
		float r21 = axis.x*sin(angleRads)+(1-cos(angleRads))*axis.y*axis.z;
		float r22 = 1 + (1-cos(angleRads))*(axis.z*axis.z-1);
		rotMatrix.set(r00, r01, r02, r10, r11, r12, r20, r21, r22);
	}
	Math::orthonormalizeIfNecessary(rotMatrix);

	return rotMatrix;

}

Matrix3 Math::fromShortestPlanarRotation( const Vector3& targetX, const Vector3& targetY )
{
	float dotProdMax = 0.0;
	Vector3 goalX, goalY;
	if( Vector3::unitX().dot(targetX) > dotProdMax )
	{
		dotProdMax = Vector3::unitX().dot(targetX);
		goalX = targetX;
		goalY = targetY;
	}
	if( Vector3::unitX().dot(targetY) > dotProdMax )
	{
		dotProdMax = Vector3::unitX().dot(targetY);
		goalX = targetY;
		goalY = -targetX;
	}
	if( Vector3::unitX().dot(-targetX) > dotProdMax )
	{
		dotProdMax = Vector3::unitX().dot(-targetX);
		goalX = -targetX;
		goalY = -targetY;
	}
	if( Vector3::unitX().dot(-targetY) > dotProdMax )
	{
		dotProdMax = Vector3::unitX().dot(-targetY);
		goalX = -targetY;
		goalY = targetX;
	}

	Vector3 goalZ = goalX.cross(goalY);

	return fromDirectionCosines(goalX, goalY, goalZ, Vector3::unitX(), Vector3::unitY(), Vector3::unitZ());
}

Matrix3 Math::fromDirectionCosines(	const Vector3& fromX, const Vector3& fromY, const Vector3& fromZ,
									const Vector3& toX, const Vector3& toY, const Vector3& toZ )
{
	float r00 = toX.dot(fromX);
	float r01 = toX.dot(fromY);
	float r02 = toX.dot(fromZ);
	float r10 = toY.dot(fromX);
	float r11 = toY.dot(fromY);
	float r12 = toY.dot(fromZ);
	float r20 = toZ.dot(fromX);
	float r21 = toZ.dot(fromY);
	float r22 = toZ.dot(fromZ);

	Matrix3 rotMatrix(r00, r01, r02, r10, r11, r12, r20, r21, r22);
	Math::orthonormalizeIfNecessary(rotMatrix);

	return rotMatrix;

}


/*
	OrientId = 6 * NormalX + NormalY

	Note - this some values are bad, because should be 24 values and this allows 36, 
	but easier to come and go...
*/

// catches bad matrices set by users
bool Math::isAxisAligned(const Matrix3& matrix)
{
	int x[3] = {0,0,0};
	int y[3] = {0,0,0};

	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			float t = matrix[i][j];
			RBXASSERT_VERY_FAST(0.0f == -0.0f);
			if ((t == 1.0f) || (t == -1.0f)) {
				x[i] += 1;
				y[j] += 1;
			}
			else {
				if ((t != 0.0f) && (t != -0.0f)) {
					return false;
				}
			}
		}
	}

	return (	(x[0] == 1)
			&&	(x[1] == 1)
			&&	(x[2] == 1)
			&&	(y[0] == 1)
			&&	(y[1] == 1)
			&&	(y[2] == 1)	);
}

bool legalOrientId(int orientId)
{
	int xNormalAbs = (orientId / 6) % 3;
	int yNormalAbs = orientId % 3;
	return (xNormalAbs != yNormalAbs);
}

int Math::getOrientId(const Matrix3& matrix)
{
	RBXASSERT_VERY_FAST(isAxisAligned(matrix));
	
	int xNormal = Vector3ToNormalId(matrix.column(0));
	int yNormal = Vector3ToNormalId(matrix.column(1));
	int answer = (6 * xNormal) + yNormal;
	RBXASSERT_VERY_FAST(legalOrientId(answer));
	return answer;
}

void Math::idToMatrix3(int orientId, Matrix3& matrix)
{
	RBXASSERT_VERY_FAST(legalOrientId(orientId));
	// TODO: Optimization: Do a table lookup
	NormalId xNormal = intToNormalId(orientId / 6);
	NormalId yNormal = intToNormalId(orientId % 6);
	Vector3 vX = normalIdToVector3(xNormal);
	Vector3 vY = normalIdToVector3(yNormal);
	Vector3 vZ = vX.cross(vY);
	matrix.setColumn(0, vX);
	matrix.setColumn(1, vY);
	matrix.setColumn(2, vZ);

	RBXASSERT_VERY_FAST(isAxisAligned(matrix));
}


 
void Math::pan(const Vector3& focusPosition, CoordinateFrame& camera, float radians)
{
	float heading, elevation, distance;

	camera.lookAt(focusPosition); // just to be sure it is pointed at the focus
	Math::getHeadingElevation(camera, heading, elevation);
	distance = (camera.translation - focusPosition).length();
	heading += radians;
	Math::setHeadingElevation(camera, heading, elevation);

	// use the cameras new look vector to shoot its new position back from the focus point
	camera.translation = focusPosition - distance * camera.lookVector();
}


// Joints have the Z axis pointing out of body 0.


Matrix3 Math::rotateAboutZ(const Matrix3& matrix, float radians)
{
	Matrix3 rotMatrix(Matrix3::identity());

	float sinR = sin(radians);
	float cosR = cos(radians);

	rotMatrix.setColumn(0, Vector3(cosR, sinR, 0.0f));
	rotMatrix.setColumn(1, Vector3(-sinR, cosR, 0.0f));

	return matrix * rotMatrix;
}



// http://en.wikipedia.org/wiki/Trajectory_of_a_projectile

float Math::computeLaunchAngle(float v, float x, float y, float g)
{
	g = fabs(g);
	float inRoot = v*v*v*v - g*(g*x*x + 2*y*v*v);
	if (inRoot <= 0.0) {
		return static_cast<float>(0.5 * Math::piHalf());
	}
	float root = ::sqrt(inRoot);
	float inATan1 = (v*v + root) / (g*x);
	float inATan2 = (v*v - root) / (g*x);
	float answer1 = ::atan(inATan1);
	float answer2 = ::atan(inATan2);
	return std::min(answer1, answer2);
}


// Returns a matrix with axes aligned to the Identity matrix, but
// with axes closest match to the incoming matrix.

Matrix3 Math::snapToAxes(const Matrix3& align)
{
	// First - find best major axis
	int aBest = -1;
	int tBest = -1;
	float best = 0.0;
	float dots[3][3];

	for (int a = 0; a < 3; ++a) {
		Vector3 aAxis = align.column(a);
		for (int t = 0; t < 3; ++t) {
			dots[a][t] = aAxis.dot(Matrix3::identity().column(t));
			if (fabs(dots[a][t]) > fabs(best)) {
				aBest = a;
				tBest = t;
				best = dots[a][t];
			}
		}
	}
    if (aBest == -1) // degenerate case
    {
        return Matrix3::identity();
    }
	Vector3 bestV = Matrix3::identity().column(tBest);
	if (best < 0.0) {
		bestV *= -1.0;
	}

	int aSecond = -1;
	int tSecond = -1;
	float second = 0.0;
	for (int a = 0; a < 3; ++a) {
		if (a != aBest) {
			for (int t = 0; t < 3; ++t) {
				if (t != tBest) {
					if (fabs(dots[a][t]) > fabs(second)) {
						aSecond = a;
						tSecond = t;
						second = dots[a][t];
					}
				}
			}
		}
	}
    if (aSecond == -1) // degenerate case
    {
        return Matrix3::identity();
    };
	Vector3 secondV = Matrix3::identity().column(tSecond);
	if (second < 0.0) {
		secondV *= -1.0;
	}

	int aThird = 3 - aBest - aSecond;
	int tThird = 3 - tBest - tSecond;
	Vector3 thirdV = Matrix3::identity().column(tThird);
	if (dots[aThird][tThird] < 0.0) {
		thirdV *= -1.0;
	}

	Matrix3 answer;
	answer.setColumn(aBest, bestV);
	answer.setColumn(aSecond, secondV);
	answer.setColumn(aThird, thirdV);

#ifdef _DEBUG
	RBXASSERT(isOrthonormal(answer));

	for (int i = 0; i < 3; ++i) {
		Vector3 newV = answer.column(i);
		Vector3 orgV = align.column(i);
		float dot = newV.dot(orgV);
		if (i == aBest) {
			RBXASSERT(dot > 0.65);
		}
		else {
			RBXASSERT(dot > 0.2);
		}
	}
#endif

	return answer;
}

Vector3 Math::toGrid(const Vector3& v, const Vector3& grid)
{
	Vector3 units = v / grid;
	return iRoundVector3(units) * grid;
}

Vector3 Math::toGrid(const Vector3& v, float grid)
{
	return toGrid(v, Vector3(grid, grid, grid));
}

CoordinateFrame Math::snapToGrid(const CoordinateFrame& snap, float grid)
{
	return CoordinateFrame(	snapToAxes(snap.rotation),
							toGrid(snap.translation, grid)	);
}

CoordinateFrame Math::snapToGrid(const CoordinateFrame& snap, const Vector3& grid)
{
	return CoordinateFrame( snapToAxes(snap.rotation),
							toGrid(snap.translation, grid)	);
}

Vector3 Math::safeDirection(const Vector3& v)
{
	float fMagnitude = v.magnitude();

    if (fMagnitude > 1e-12f) {
		float fInvMagnitude = 1.0f / fMagnitude;
		Vector3 answer = Vector3(v.x * fInvMagnitude, v.y * fInvMagnitude, v.z * fInvMagnitude);
		RBXASSERT(answer.magnitude() < 1.01f);
		return answer;
    } else {
		RBXASSERT(0);
		return Vector3::unitX();
    }
}


Vector3 Math::iRoundVector3(const Vector3& point)
{
	return Vector3(	(float)iRound(point.x),
					(float)iRound(point.y),
					(float)iRound(point.z)	);
}

// return 0..pi
float Math::angle(const Vector3& v0, const Vector3& v1)
{
	RBXASSERT_FISHING(v0.isUnit());
	RBXASSERT_FISHING(v1.isUnit());

	const float dot = v0.dot(v1);

	const float answer = (dot >= 1.0f)
							? 0.0f 
							: (
								(dot<=-1.0f)
									? pif() 
									: ::acosf(dot)
								);
	return answer;
}

float Math::elevationAngle(const Vector3& look)
{
	RBXASSERT_VERY_FAST(look.y >= -1.0f);
	RBXASSERT_VERY_FAST(look.y <= 1.0f);

	const float answer = (look.y >= 1.0) 
							? piHalff() 
							: (
								(look.y <= -1.0)
									? -piHalff()
									: ::asin(look.y)
								);
	return answer;
}

float Math::smallAngle(const Vector3& v0, const Vector3& v1)
{
	RBXASSERT_VERY_FAST(angle(v0, v1) < (Math::pif() * 0.10));

	return v0.cross(v1).magnitude();
}


bool fuzzyColinear(const Vector3& v0, const Vector3& v1, float radTolerance)
{
	RBXASSERT_VERY_FAST(Math::fuzzyEq(v0.magnitude(), 1.0f, 1e-4f));		// lower tolerance
	RBXASSERT_VERY_FAST(Math::fuzzyEq(v1.magnitude(), 1.0f, 1e-4f));		// lower tolerance

	return (v0.cross(v1).magnitude() < radTolerance);
}

bool Math::fuzzyAxisAligned(const Matrix3& m0, const Matrix3& m1, float radTolerance)
{
	Vector3 a1[3];
	for (int i = 0; i < 3; ++i) {
		a1[i] = m1.column(i);
	}
	for (int i = 0; i < 3; ++i) {
		Vector3 a0 = m0.column(i);
		bool found = false;

		for (int j = 0; j < 3; ++j) {
			if (fuzzyColinear(a0, a1[j], radTolerance)) {
				found = true;
				break;
			}
		}
		if (!found) {
			return false;
		}
	}
	return true;
}		

bool Math::orthonormalizeIfNecessary(Matrix3& m)
{
	if (!isOrthonormal(m)) {
		m.orthonormalize();
		RBXASSERT_IF_VALIDATING(isOrthonormal(m));
		return true;
	}
	else {
		return false;
	}
}

bool Math::isOrthonormal(const Matrix3& m)
{
	return m.isOrthonormal();
//	Matrix3 compare(m);
//	compare.orthonormalize();
//	return Math::fuzzyEq(compare, m);
}

bool Math::fuzzyEq(const Vector3& v0, const Vector3& v1, float epsilon)	// Note G3D::eps == 1e-6;
{
	for (int i = 0; i < 3; ++i) {
		if (! Math::fuzzyEq(v0[i], v1[i], epsilon)) {
			return false;
		}
	}
	return true;
}

bool Math::fuzzyEq(const Matrix3& m0, const Matrix3& m1, float epsilon)
{
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
			if (! Math::fuzzyEq(m0[r][c], m1[r][c], epsilon)) {
                return false;
            }
        }
    }
    return true;
}

bool Math::fuzzyEq(const Matrix4& m0, const Matrix4& m1, float epsilon)
{
	for (int r = 0; r < 4; ++r) {
		for (int c = 0; c < 4; ++c) {
			if (! Math::fuzzyEq(m0[r][c], m1[r][c], epsilon)) {
				return false;
			}
		}
	}
	return true;
}

bool Math::fuzzyEq(const CoordinateFrame& c0, const CoordinateFrame& c1, float epsT, float epsR)
{
	if (!fuzzyEq(c0.translation, c1.translation, epsT)) {
		return false;
	}

	if (!fuzzyEq(c0.rotation, c1.rotation, epsR)) {
		return false;
	}

	return true;
}



Velocity Math::calcTrajectory(const Vector3& launch, const Vector3& target, float speed)
{
	Vector3 direction = Math::safeDirection(target - launch);
	return Velocity(speed * direction, Vector3::zero());
}


// Tricky - returns a Matrix3 that is axis aligned with "align", but whos
// axes are now in the best position to match those of "target"
//
Matrix3 Math::alignAxesClosest(const Matrix3& align, const Matrix3& target)
{
	// First - find best major axis
	int aBest = -1;
	int tBest = -1;
	float best = 0.0;
	float dots[3][3];

	for (int a = 0; a < 3; ++a) {
		Vector3 aAxis = align.column(a);
		for (int t = 0; t < 3; ++t) {
			Vector3 tAxis = target.column(t);
			dots[a][t] = aAxis.dot(tAxis);
			if (fabs(dots[a][t]) > fabs(best)) {
				aBest = a;
				tBest = t;
				best = dots[a][t];
			}
		}
	}
    if (aBest == -1)
    {
        return Matrix3::identity();
    }
	Vector3 bestV = align.column(aBest) * polarity(best);

	int aSecond = -1;
	int tSecond = -1;
	float second = 0.0;
	for (int a = 0; a < 3; ++a) {
		if (a != aBest) {
			for (int t = 0; t < 3; ++t) {
				if (t != tBest) {
					if (fabs(dots[a][t]) > fabs(second)) {
						aSecond = a;
						tSecond = t;
						second = dots[a][t];
					}
				}
			}
		}
	}
    if (aSecond == -1)
    {
        return Matrix3::identity();
    }
	Vector3 secondV = align.column(aSecond) * polarity(second);

	int aThird = 3 - aBest - aSecond;
	int tThird = 3 - tBest - tSecond;
	Vector3 thirdV = align.column(aThird) * polarity(dots[aThird][tThird]);

	Matrix3 answer;
	answer.setColumn(tBest, bestV);
	answer.setColumn(tSecond, secondV);
	answer.setColumn(tThird, thirdV);

	RBXASSERT_VERY_FAST(isOrthonormal(answer));

	return answer;
}



// Definition of small angles - 
//
// Small rotation about X axis, for example, == average of the sin-1 of the y and z
// axis about x.  sin-1 of the y axis around x ~ the z value of the y normal vector.
// sin-1 of the z axis around x ~ the -y value of the z normal vector.

Vector3 Math::toSmallAngles(const Matrix3& matrix)
{
//	Vector3 x = matrix.column(0);
//	Vector3 y = matrix.column(1);
//	Vector3 z = matrix.column(2);
//	Vector3 angles;
//	angles.x = 0.5 * (y.z - z.y);
//	angles.y = 0.5 * (z.x - x.z);
//	angles.z = 0.5 * (x.y - y.x);
	
	return Vector3(	0.5f * (matrix[2][1] - matrix[1][2]),
					0.5f * (matrix[0][2] - matrix[2][0]),
                    0.5f * (matrix[1][0] - matrix[0][1])	);
}


void Math::rotateAboutYLocal(CoordinateFrame& c, float radians)
{
	const Vector3 lookVector = c.lookVector();
	const Vector3 perp(lookVector.z, lookVector.y, -lookVector.x);

	const Vector3 lookDelta = lookVector + radians * perp;
	c.lookAt(c.translation + lookDelta);
}

void Math::rotateAboutYGlobal(CoordinateFrame& c, float radians)
{
	Matrix3 rot = Matrix3::fromAxisAngleFast(Vector3::unitY(), radians);
	c.rotation = rot * c.rotation;
}

Vector3 Math::rotateAboutYGlobal(const Vector3& v, float radians)
{
	Matrix3 rot = Matrix3::fromAxisAngleFast(Vector3::unitY(), radians);
	Vector3 answer = rot * v;
	return answer;
}

NormalId Math::getClosestObjectNormalId(const G3D::Vector3& worldV, const G3D::Matrix3& objectR)
{
	// essentially do three dot products - columns of rotMatrix
	// are the x,y,z normals of body in world coords
    Vector3 dotResult = worldV * objectR;

	double nx = fabs(dotResult.x);
    double ny = fabs(dotResult.y);
    double nz = fabs(dotResult.z);

    if (nx > ny) {
        if (nx > nz) {
			return (dotResult.x > 0) ? NORM_X : NORM_X_NEG;
		}
		else {
			return (dotResult.z > 0) ? NORM_Z : NORM_Z_NEG;
        }
    } 
	else {
        if (ny > nz) {
			return (dotResult.y > 0) ? NORM_Y : NORM_Y_NEG;
        } 
		else {
			return (dotResult.z > 0) ? NORM_Z : NORM_Z_NEG;
        }
	}
}

Vector3 Math::sortVector3(const Vector3& v)
{
	Vector3 ans = v;

	if (ans.z < ans.y) {
		std::swap(ans.z, ans.y);
	}
	if (ans.y < ans.x) {
		std::swap(ans.y, ans.x);
	}
	if (ans.z < ans.y) {
		std::swap(ans.z, ans.y);
	}
	RBXASSERT_VERY_FAST(ans.z >= ans.y);
	RBXASSERT_VERY_FAST(ans.y >= ans.x);
	return ans;
}


float Math::volume(const G3D::Vector3& v)
{
	return v.x * v.y * v.z;
}

float Math::maxAxisLength(const G3D::Vector3& v)
{
	Vector3 answer = vector3Abs(v);
	return std::max(answer.x, std::max(answer.y, answer.z));
}


Vector3 Math::vector3Abs(const G3D::Vector3& v)
{
	return Vector3(fabs(v.x), fabs(v.y), fabs(v.z));
}

void Math::getHeadingElevation(const CoordinateFrame& c, float& heading, float& elevation)
{
	Vector3 look = c.lookVector();
	heading = getHeading(look);
	elevation = getElevation(look);
}

void Math::setHeadingElevation(CoordinateFrame& c, float heading, float elevation)
{
	float elevationVector = sinf(elevation);

	float xzLength = sqrtf(1 - elevationVector * elevationVector);

	Vector3 look(-sinf(heading) * xzLength, elevationVector, -cosf(heading) * xzLength);
	
	Vector3 lookAt = c.translation + look.direction();

	c.lookAt(lookAt);
}


Vector3 Math::closestPointOnRay(const RBX::RbxRay& pointOnRay, 
								const RBX::RbxRay& otherRay)
{
	Vector3 pa = pointOnRay.origin();
	Vector3 pPlane = otherRay.origin();

	Vector3 ua = pointOnRay.direction();
	Vector3 ub = otherRay.direction();
	
	Vector3 p = pPlane - pa;
	float uaub = ua.dot(ub);
	float q1 = ua.dot(p);
	float q2 = -ub.dot(p);

	float d = 1 - uaub*uaub;

	if (d > 1e-5) {						// work here
		float alpha = (q1 + uaub*q2) / d;
		return pa + alpha * ua;
	}
	else {
		return pa;
	}
}


void Math::rotateMatrixAboutX90(Matrix3& matrix, int times)
{
	static const Matrix3 x90(1,0,0,	0,0,-1,		0,1,0);

	RBXASSERT((times <4) && (times >= 0));
	for (int i = 0; i < times; ++i) {
		matrix = x90 * matrix;
//		Matrix3 answer(matrix);
//		answer.setColumn(1, matrix.column(2));		// new y column is old z
//		answer.setColumn(2, -matrix.column(1));		// new z column is - old y
//		matrix = answer;
	}
}

void Math::rotateMatrixAboutY90(Matrix3& matrix, int times)
{
	static const Matrix3 y90(0,0,1,	0,1,0,	-1,0,0);

	RBXASSERT((times <4) && (times >= 0));
	for (int i = 0; i < times; ++i) {
		matrix = y90 * matrix;
	}
}




void Math::rotateMatrixAboutZ90(Matrix3& matrix)
{
	static const Matrix3 z90(0,1,0,	-1,0,0,		0,0,1);
//	Matrix3 answer(matrix);
//	answer.setColumn(0, matrix.column(1));		// new x column is old y column
//	answer.setColumn(1, -matrix.column(0));		// new y column is reverse of old x column;
//	matrix = answer;
	matrix = z90 * matrix;
}



const Matrix3& Math::matrixRotateX() {
    static const Matrix3 m(1,0,0,	0,0,-1,	0,1,0);
    return m;
}

const Matrix3& Math::matrixRotateY() {
    static const Matrix3 m(0,0,1,	0,1,0,	-1,0,0);
    return m;
}

const Matrix3& Math::matrixRotateNegativeY() {
    static const Matrix3 m(0,0,-1,	0,1,0,	1,0,0);
    return m;
}

const Matrix3& Math::matrixTiltZ() {
    static const Matrix3 m(0,1,0,	-1,0,0,	0,0,1);
    return m;
}

const Matrix3& Math::matrixTiltNegativeZ() {
    static const Matrix3 m(0,-1,0,	 1,0,0,	0,0,1);
    return m;
}


// Octant defines an axis in the XZ plane

const Matrix3 Math::matrixTiltQuadrant(int quadrant)
{
	switch (quadrant)
	{
	// 0: tilt around the x axis
	case (0):	return Matrix3(1,0,0,	0,0,-1,	0,1,0);

	// 1: tilt around the z axis
	case (1):	return matrixTiltZ();

	// 2:  tilt around the x axis negatively
	case (2):	return Matrix3(1,0,0,	0,0,1,	0,-1,0);

	// 3:  tilt around the z axis negatively
	case (3):	return matrixTiltNegativeZ();

	default:	RBXASSERT(0);	
				return Matrix3::identity();
	}
}



int Math::radiansToQuadrant(float radians)
{
	radians += (float)(G3D::pi() * 2.25);		// extra wrap plus one eight turn;
	RBXASSERT(radians >= 0.0);
	int quadrant = G3D::iFloor(4.0 * radians / G3D::twoPi());
	return quadrant % 4;
}


int Math::radiansToOctant(float radians)
{
	radians += (float)(G3D::pi() * 2.125);		// extra wrap plus one sixteenth turn;
	RBXASSERT(radians >= 0.0);
	int octant = G3D::iFloor(8.0 * radians / G3D::twoPi());
	return octant % 8;
}


int Math::toYAxisQuadrant(const CoordinateFrame& c)
{
	Vector3 look = c.lookVector();
	float angle = atan2(-look.x, -look.z);
	return radiansToQuadrant(angle);
}

bool Math::intersectRayConvexPolygon(const RBX::RbxRay& ray,  const std::vector<Vector3>& poly, Vector3& hit, bool oneSided)
{
    if( poly.size() < 3 )
    {
        RBXASSERT(0);
        return false;
    }
    
    Vector3 pNormal = ((poly[1] - poly[0]).cross(poly[2] - poly[0])).direction();

    // if ray hits back side of polygon
    if( oneSided && pNormal.dot(ray.direction()) > 0 )
        return false;

    float pDis = poly[0].dot(pNormal);
    Plane polyPlane(pNormal, pDis);

    if( !intersectRayPlane(ray, polyPlane, hit) )
        return false;

    // see if hit is within the bounds of the polygon
    for( unsigned int i = 0; i < poly.size(); i++ )
    {
        unsigned int next = i == poly.size() - 1 ? 0 : i + 1;
        if( pNormal.dot((poly[next] - poly[i]).cross(hit - poly[i])) < 0.0 )
            return false;
    }

    return true;
}

const float areaTolerance = 0.5;

std::vector<Vector3> Math::spatialPolygonIntersection(const std::vector<Vector3>& polyA,  const std::vector<Vector3>& polyB)
{
    std::vector<Vector3> result;

    if( polyA.size() < 3 || polyB.size() < 3 )
    {
        RBXASSERT(0);
        return result;
    }
    
    // check normals (they must be opposite direction and aligned)
    const Vector3 normalA = ((polyA[1] - polyA[0]).cross(polyA[2] - polyA[0])).direction();
    const Vector3 normalB = ((polyB[1] - polyB[0]).cross(polyB[2] - polyB[0])).direction();
    const float normalCheck = fabs(normalA.dot(-normalB) - 1.0);
    if( normalCheck > kEpsilon )
        return result;

    // both polygons must be at same reference distance
    const float distanceCheck = fabs((polyA[0] - polyB[0]).dot(normalA));
    if( distanceCheck > 0.1 )
        return result;

	// represent the other polygon and this polyhrons face polygon in 2D coordinates.
    std::vector<Vector2> polyA2d, polyB2d;
    const Vector3 origin2d = polyA[0];
    const Vector3 xAxis = (polyA[1] - polyA[0]).unit();
    const Vector3 yAxis = (normalA.cross(xAxis)).unit();

    for( unsigned int i = 0; i < polyA.size(); i++ )
    {
        Vector2 pointA((polyA[i] - origin2d).dot(xAxis), (polyA[i] - origin2d).dot(yAxis));
        polyA2d.push_back(pointA);
    }

    for( unsigned int i = 0; i < polyB.size(); i++ )
    {
        Vector2 pointB((polyB[i] - origin2d).dot(xAxis), (polyB[i] - origin2d).dot(yAxis));
        polyB2d.push_back(pointB);
    }

    std::vector<Vector2> intersection2d = Math::planarPolygonIntersection(polyA2d, polyB2d);

    for( unsigned int i = 0; i < intersection2d.size(); i++ )
    {
        Vector3 xComp = xAxis * intersection2d[i].x;
        Vector3 yComp = yAxis * intersection2d[i].y;
        result.push_back(origin2d + xComp + yComp);
    }
    
    // do some checks to make sure a non-empty result is physically meaningful
    if( result.size() < 3 )
    {
        result.clear();
        return result;
    }

    // check contents of result for coincident points which indicates a zero area overlap
    float area = 0.0f;
    for( unsigned int i = 1; i < result.size()-1; i++ )
    {
        Vector3 v1 = result[i] - result[0];
        Vector3 v2 = result[i+1] - result[0];
        area += (v1.cross(v2)).magnitude();
    }
    if( fabs(area) < areaTolerance )
    {
        result.clear();
        return result;
    }

    return result;
}

bool Math::intersectRayPlane(const RBX::RbxRay& ray, const G3D::Plane& plane, G3D::Vector3& hit)
{
	float dotProd = ray.direction().dot(plane.normal());
	bool onPositiveSide = plane.halfSpaceContains(ray.origin());
	
	if (	(onPositiveSide && (dotProd < 0.0))
		||	(!onPositiveSide && (dotProd > 0.0))	) 
	{
		Line line = Line::fromPointAndDirection(ray.origin(), ray.direction());
		hit = line.intersection(plane);
		RBXASSERT(hit != Vector3::inf());
		return true;
	}
	else {
		hit = Vector3::inf();
		return false;
	}
}


bool intersectLinePlaneOld(const RBX::RbxRay& ray, const G3D::Plane& plane, G3D::Vector3& hit)
{
	if (Math::intersectRayPlane(ray, plane, hit)) {
		return true;
	}

	RBX::RbxRay oppositeRay = RBX::RbxRay::fromOriginAndDirection(ray.origin(), -ray.direction());
	if (Math::intersectRayPlane(oppositeRay, plane, hit)) {
		return true;
	}

	hit = Vector3::inf();
	return false;
}


bool Math::intersectLinePlane(const Line& line, const Plane& plane, Vector3& hit)
{
	hit = line.intersection(plane);
	bool answer = (hit != Vector3::inf());

#ifdef _DEBUG
	RbxRay tempRay = RBX::RbxRay::fromOriginAndDirection(line.point(), line.direction());
	Vector3 tempHit;
	intersectLinePlaneOld(tempRay, plane, tempHit);
	RBXASSERT(tempHit.fuzzyEq(hit));
#endif

	return answer;
}





// return false if origin outside the box
// and sets endPoint = origin
bool Math::clipRay(Vector3& origin, Vector3& ray, Vector3 box[], Vector3& endPoint) 
{
	Vector3 minB = box[0];
	Vector3 maxB = box[1];
	Vector3 maxT(-1.0, -1.0, -1.0);

	// Find candidate planes.
	for(int i = 0; i < 3 ; i++)
	{
		if ((origin[i] < minB[i]) || (origin[i] > maxB[i])){
			endPoint = origin;
			return false;			// origin outside of box
		}

		if (ray[i] > 0.0) {
			endPoint[i]	= maxB[i];
			maxT[i] = (maxB[i] - origin[i]) / ray[i];
		}
		else if (ray[i] < 0.0) {
			endPoint[i]	= minB[i];
			maxT[i] = (minB[i] - origin[i]) / ray[i];
		}
		else {			// ray[i] == 0.0
			maxT[i] = 0.0;
		}
	}

	// Get largest of the maxT's for final choice of intersection
	int whichPlane = 0;
	if(maxT[1] > maxT[whichPlane])	whichPlane = 1;
	if(maxT[2] > maxT[whichPlane])	whichPlane = 2;

	for (int i = 0; i < 3; i++) {
		if (i != whichPlane) {
			endPoint[i] = origin[i] + maxT[whichPlane] * ray[i];
		}
	}
	return true;	// origin inside the box
}

const Matrix3& Math::getAxisRotationMatrix(int face)
{
	static Matrix3 y = Matrix3::fromEulerAnglesXYZ(0.0f, 0.0f, (float)G3D::halfPi());
	static Matrix3 z = Matrix3::fromEulerAnglesXYZ(0.0f, (float)G3D::halfPi(), 0.0f);
	static Matrix3 y_neg = y;
	static Matrix3 z_neg = z;

	switch (face) {
		case (0):	return Matrix3::identity();
		case (1):	return y;
		case (2):	return z;
		case (3):	return Matrix3::identity();
		case (4):	return y_neg;
		case (5):	return z_neg;
		default:	RBXASSERT(0); return Matrix3::identity();
	}
}

bool Math::lineSegmentDistanceIfCrossing(const Vector3& line1Pt1, const Vector3& line1Pt2, const Vector3& line2Pt1, const Vector3& line2Pt2, float& distance, float adjustEdgeTol)
{
	const float tolerance = adjustEdgeTol - 1.0f;

	// Compute unit vectors for each line segment from point1 to point2.
	Vector3 u1((line1Pt2 - line1Pt1)/(line1Pt2 - line1Pt1).magnitude());
	Vector3 u2((line2Pt2 - line2Pt1)/(line2Pt2 - line2Pt1).magnitude());

	// if they're nearly parallel, return false
	if( fabs(fabs(u1.dot(u2)) - 1.0) < tolerance )
      return false;

	float a1 = (line1Pt1 - line2Pt1).dot(u1);
	float a2 = (line1Pt1 - line2Pt1).dot(u2);  
	float b = u1.dot(u2);
   
	float t1 = (b*a2 - a1*b*b)/(1 - b*b) - a1;
	float t2 = (a2 - a1*b)/(1 - b*b);

	Vector3 line1ClosestPoint = line1Pt1 + t1*u1;
	Vector3 line2ClosestPoint = line2Pt1 + t2*u2;

	// Check that the clost points are within the bounds of each respective line segment
	// See if the "t" parameters compute a point completely off the corresponding edge.
	if( t1 < -tolerance || t2 < -tolerance )
		return false;

	if( (t1 * u1).magnitude() > (line1Pt2 - line1Pt1).magnitude() + tolerance )
		return false;
	if( (t2 * u2).magnitude() > (line2Pt2 - line2Pt1).magnitude() + tolerance )
		return false;

	distance = (line1ClosestPoint - line2ClosestPoint).magnitude();

	return true;
}

Vector2 Math::polygonStartingPoint(int numSides, float maxWidth)
{
	// Returns an x, y planar vector for the appropriate starting point for a polygon based
	// upon the number of sides and the desired width.  Optimized for common sizes to best
	// represent the size desired by the user.  Function is used by the physics and
	// rendering mesh builders for prisms and pyramids.
	Vector2 ans(1.0, 1.0);
	float alpha = (float)(Math::pif()/numSides);

	switch(numSides)
	{
		case 3: 
			ans.x = (float)(-0.5 * maxWidth);
			ans.y = (float)(0.5 * maxWidth / tan(alpha));
			return ans;
		case 5:
			ans.x = (float)((-maxWidth * sin(alpha)) / (2.0 * sin(2.0 * alpha)));
			ans.y = (float)((maxWidth * cos(alpha)) / (2.0 * sin(2.0 * alpha)));
			return ans;
		case 4:
		case 8:
		default:
			ans.x = (float)(-tan(alpha) * 0.5*maxWidth);
			ans.y = (float)(0.5*maxWidth);
			return ans;
		case 6:
			ans.x = (float)(-sin(alpha) * 0.5*maxWidth);
			ans.y = (float)(cos(alpha) * 0.5*maxWidth);
			return ans;
	}
}

Matrix3 Math::getWellFormedRotForZVector(const Vector3& zNew)
{
	// Find best projection of y-axis onto the plane of input vector.
	// Set this as the new y-axis.
	// Then, use xN = yN X zN 
	Vector3 yOrig = Vector3::unitY();
	Vector3 zOrig = Vector3::unitZ();
	Vector3 yNew;

	float magYProjection = (yOrig - (yOrig.dot(zNew) * zNew)).magnitude();

	if( fabs(magYProjection) > kEpsilon )
	{
		yNew = (yOrig - (yOrig.dot(zNew) * zNew)) / magYProjection;
	}
	else
	{
		float magZProjection = (zOrig - (zOrig.dot(zNew) * zNew)).magnitude();
		yNew = (zOrig - (zOrig.dot(zNew) * zNew)) / magZProjection;
	}
	Vector3 xNew = yNew.cross(zNew);

	// Determine rotation matrix to express face vectors into body vectors

	Matrix3 ans;
	ans.setColumn(0, xNew);
	ans.setColumn(1, yNew);
	ans.setColumn(2, zNew);

	return ans;
}

bool Math::evenWholeNumber( const float& input )
{
	// True if a float has an even non fractional part.
	float integerPart;
	modff(input, &integerPart);
	return (int)integerPart % 2 == 0;
}

bool Math::evenWholeNumberFuzzy( const float& rawInput )
{
	// True if a float has an even whole number, but will round numbers that are close to
	// an integer if within tolerance.  This helps situations where we have 1.9998 instead of 2.0000 due
	// to numerical imprecision, and we don't want to treat this as 1.0 and return false.
	float round = floor(rawInput + 0.5f);
	float refined = (fabs(round - rawInput) < kEpsilon) ? round : rawInput;

	return evenWholeNumber(refined);
}

std::vector<Vector2> Math::planarPolygonIntersection(const std::vector<Vector2>& poly1, const std::vector<Vector2>& poly2 )
{
	// Form internal vertex list poly required by GPC library API
	gpc_vertex *vertList1 = new gpc_vertex[poly1.size()];
	gpc_vertex_list gpcVertList1;
	gpcVertList1.vertex = vertList1;
	gpcVertList1.num_vertices = poly1.size();
	for( unsigned int i = 0; i < poly1.size(); i++ )
	{
		vertList1[i].x = poly1[i].x;
		vertList1[i].y = poly1[i].y;
	}

	gpc_polygon gpcPoly1;
	gpcPoly1.num_contours = 1;
	gpcPoly1.hole = NULL;
	gpcPoly1.contour = &gpcVertList1;

	gpc_vertex *vertList2 = new gpc_vertex[poly2.size()];
	gpc_vertex_list gpcVertList2;
	gpcVertList2.vertex = vertList2;
	gpcVertList2.num_vertices = poly2.size();
	for( unsigned int i = 0; i < poly2.size(); i++ )
	{
		vertList2[i].x = poly2[i].x;
		vertList2[i].y = poly2[i].y;
	}

	gpc_polygon gpcPoly2;
	gpcPoly2.num_contours = 1;
	gpcPoly2.hole = NULL;
	gpcPoly2.contour = &gpcVertList2;

	gpc_polygon gpcResult;
	gpc_polygon_clip(GPC_INT, &gpcPoly1, &gpcPoly2, &gpcResult);

	// set the result std::vector
	std::vector<Vector2> result;
	if(gpcResult.contour)
	{
		for( int i = 0; i < gpcResult.contour->num_vertices; i++ )
		{
			Vector2 resultVert((float)gpcResult.contour->vertex[i].x, (float)gpcResult.contour->vertex[i].y);
			result.push_back(resultVert);
		}
	}

	gpc_free_polygon(&gpcResult);
	delete [] vertList1;
	delete [] vertList2;

	return result;
}



} // namespace
