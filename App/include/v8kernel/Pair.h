/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Kernel/ContactParams.h"
#include "Util/NormalID.h"
#include "Util/G3DCore.h"
#include "rbx/Debug.h"

namespace RBX {

	class Body;

	class PairParams {
	public:
		Vector3	normal;
		union {
			float length;
			float rotation;
		};
		Vector3	position;
		PairParams() {
			normal = position = Vector3::zero();
			length = 0.0f;
		}
		bool operator==(const PairParams& other) {
			return (length == other.length && position == other.position && normal == other.normal);
		}
	};

	///////////////////////////////////////////////////////////////////

	class GeoPair
	{
	public:
		GeoPairType	geoPairType;

		// note, pair point0 and point1 have the following polarity
		// ball ball: radius0 == point0 body
		// ball block: ball->point0, block->point1
		// point plane: pointBlock->0, planeBlock->1
		// edge edge plane:  planeBlock->1
		// fixed, not allocated here

		// This is defining data
		Vector3		offset0;	
		Vector3		offset1;
		Body*		body0;
		Body*		body1;
		float		edgeLength0;
		float		edgeLength1;
		struct {
			union {
				RBX::NormalId	normalID0;
				float			radius0;		};
			union {
				RBX::NormalId	normalID1;
				float			radiusSum;		};
			union {
				RBX::NormalId	planeID;// edge/edge/plane coords - the normal from the plane
				int				point0ID;		};
		} pairData;

	private:
		void computePointPlane(PairParams& _params);
		void computeEdgeEdgePlane(PairParams& _params);
		void computeEdgeEdgePlane2(PairParams& _params);
		void computeEdgeEdge(PairParams& _params);

	public:
		GeoPair();

		////////// Kernel Update

		inline void computeLengthNormalPosition(PairParams& _params) 
		{
			switch (geoPairType) {
				case (POINT_PLANE_PAIR):		computePointPlane(_params);			break;
				case (EDGE_EDGE_PLANE_PAIR):	computeEdgeEdgePlane2(_params);		break;
				case (EDGE_EDGE_PAIR):			computeEdgeEdge(_params);			break;
				default:	RBXASSERT(0);
			}
		}

		///////////////// GeoPair geometric functions

		void setPointPlane(const Vector3* _offsetPoint,
							const Vector3* _offsetPlane, int _pointID, RBX::NormalId _planeNormalID) {
			offset0 = *_offsetPoint;
			offset1 = *_offsetPlane;
			pairData.point0ID = _pointID;		// purely here for the match
			pairData.normalID1 = _planeNormalID;
			geoPairType = POINT_PLANE_PAIR;
		}

		void setEdgeEdgePlane(const Vector3* _edge0, const Vector3* _edge1, 
				RBX::NormalId _normal0, RBX::NormalId _normal1, RBX::NormalId _planeID, float _edgeLength0, float _edgeLength1) {
			offset0 = *_edge0;
			offset1 = *_edge1;
			pairData.normalID0 = _normal0;
			pairData.normalID1 = _normal1;
			pairData.planeID = _planeID;
			edgeLength0 = _edgeLength0;
			edgeLength1 = _edgeLength1;
			geoPairType = EDGE_EDGE_PLANE_PAIR;
		}

		void setEdgeEdge(const Vector3* _edge0, const Vector3* _edge1, 
							RBX::NormalId _normal0, RBX::NormalId _normal1) {
			offset0 = *_edge0;
			offset1 = *_edge1;
			pairData.normalID0 = _normal0;
			pairData.normalID1 = _normal1;
			geoPairType = EDGE_EDGE_PAIR;
		}

		bool match(Body* _b0, Body* _b1, GeoPairType _pairType, int param0, int param1) {
			if  (_pairType == POINT_PLANE_PAIR) {
				return (	(_b0 == body0)
						&&	(_b1 == body1) 
						&&  (param0 == pairData.point0ID)
						&&	(param1 == pairData.normalID1)			);
			}

			else if  (_pairType == EDGE_EDGE_PLANE_PAIR) {
				return (	(_b0 == body0)
						&&	(_b1 == body1) 
						&&  (param0 == pairData.normalID0)
						&&	(param1 == pairData.normalID1)			);
			}

			else {
				RBXASSERT(_pairType == EDGE_EDGE_PAIR);
				return (	(	(_b0 == body0)
							&&	(_b1 == body1) 
							&&  (param0 == pairData.normalID0)
							&&	(param1 == pairData.normalID1)			)		
						||	
							(	(_b0 == body1)
							&&	(_b1 == body0) 
							&&  (param0 == pairData.normalID1)
							&&	(param1 == pairData.normalID0)			)		
						);
			}
		}
	};

} // namespace RBX