#pragma once

#include "stdafx.h"
#include "Util/G3DCore.h"
#include "Util/Extents.h"
#include "V8Kernel/ContactConnector.h"


namespace RBX {
	
	class Tolerance 
	{
	public:
		//////////////////////////////////////////////////////////
		//
		static const Extents& maxExtents() {
			// cds: this is the no-clip hack patch. 1777.7 is arbitrary.
			const float fuzzyMil = 1e6 + 1777.7 + (*((int*)(__DATE__ + 2)) % 1000);
			static Extents millionCube(Vector3(-fuzzyMil - (rand()%65536), 
			                                   -fuzzyMil - (rand()%65536), 
			                                   -fuzzyMil - (rand()%65536)), 
			                           Vector3( fuzzyMil + (rand()%65536),
			                                  	fuzzyMil + (rand()%65536), 
			                                  	fuzzyMil + (rand()%65536)));
			return millionCube;
		}

		//  Tolerance for joining
		static float mainGrid()								{return 0.1f;}
		static float jointMaxUnaligned()					{return 0.05f;}
		static float jointOverlapMin()						{return 0.35f;}		// plate thickness is 0.4
		static float jointOverlapMin2()						{return 0.1f;}		

		static bool pointsUnaligned(const Vector3& p0, const Vector3& p1) {
			float magSqr = (p1-p0).squaredMagnitude();
			return (magSqr > (jointMaxUnaligned() * jointMaxUnaligned()));
		}

		// Joint, Spawn: tight parameters, only achieved by a snap
		static float jointAngleMax()						{return 0.01f;}		// radians
		static float jointPlanarMax()						{return 0.01f;}

		// Rotate: loose parameters
		static float rotateAngleMax()						{return jointMaxUnaligned() * 0.5f;}	// length of the axle is always == 2
		static float rotatePlanarMax()						{return jointMaxUnaligned();}

		// Glue: loose parameters
		static float glueAngleMax()							{return jointMaxUnaligned();} // radians 
		static float gluePlanarMax()						{return jointMaxUnaligned();}

		// Tolerance for dragger and for primitive::fuzzyExtents
		// For now this is nasty - it equals the connector overlap tolerance
		static float maxOverlapOrGap()						{return ContactConnector::overlapGoal();}		// 0.01;

		static float maxOverlapAllowedForResize()			{return 3.0f * maxOverlapOrGap();}				// 0.03;
	};

} // namespace

