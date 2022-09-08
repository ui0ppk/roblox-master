/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Kernel/ContactParams.h"
#include "V8Kernel/ContactConnector.h"
#include "Util/G3DCore.h"
#include "rbx/Debug.h"

namespace RBX {

	//////////////////////////////////////////////////////////////////////////////////
	//
	// BLOCK BLOCK Types

	class PolyConnector	: public ContactConnector
	{
	private:
		int param0;
		int param1;			// for matching

	protected:
		/*implement*/ virtual GeoPairType getConnectorType() const = 0;

		PolyConnector(
			Body* b0, 
			Body* b1,
			const ContactParams& contactParams, 
			int param0,
			int param1)
			: ContactConnector(b0, b1, contactParams)
			, param0(param0)
			, param1(param1)
		{}

	public:
		static bool match(PolyConnector* p0, PolyConnector* p1) {
			return (	(p0->param0 == p1->param0)
					&&	(p0->param1 == p1->param1)
					&&	(p0->getConnectorType() == p1->getConnectorType())	);
		}
	};

	////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////

	class FaceVertexConnector :	public PolyConnector, 
								public Allocator<FaceVertexConnector>
	{
	private:
		Plane facePlane;
		Vector3 vertexOffset;

		/*override*/ GeoPairType getConnectorType() const {return VERTEX_PLANE_CONNECTOR;}

	public:
		FaceVertexConnector(
			Body* b0, 
			Body* b1,
			const ContactParams& contactParams, 
			const Plane& facePlane,
			const Vector3& vertexOffset,
			int planeId,
			int vertexId)
			: PolyConnector(b0, b1, contactParams, planeId, vertexId)
			, facePlane(facePlane)
			, vertexOffset(vertexOffset)
		{}
		
		/*override*/ void updateContactPoint();
	};


	class FaceEdgeConnector :	public PolyConnector, 
								public Allocator<FaceEdgeConnector>
	{
	private:
		Plane facePlane;
		Plane sideFacePlane;
		Line faceLine;
		Line edgeLine;

		/*override*/ GeoPairType getConnectorType() const {return EDGE_EDGE_PLANE_CONNECTOR;}

	public:
		FaceEdgeConnector(
			Body* b0, 
			Body* b1,
			const ContactParams& contactParams, 
			const Plane& facePlane,
			const Plane& sideFacePlane,
			Line faceLine,
			Line edgeLine,
			const int faceId,
			const int edgeId)
			: PolyConnector(b0, b1, contactParams, faceId, edgeId)
			, facePlane(facePlane)
			, sideFacePlane(sideFacePlane)
			, faceLine(faceLine)
			, edgeLine(edgeLine)
		{}

		/*override*/ void updateContactPoint();
	};

	class EdgeEdgeConnector :	public PolyConnector, 
								public Allocator<EdgeEdgeConnector>
	{
	private:
		Line edgeLine0;
		Line edgeLine1;

		/*override*/ GeoPairType getConnectorType() const {return EDGE_EDGE_CONNECTOR;}

	public:
		EdgeEdgeConnector(
			Body* b0, 
			Body* b1,
			const ContactParams& contactParams, 
			Line edgeLine0,
			Line edgeLine1,
			int edgeId0,
			int edgeId1	)
			: PolyConnector(b0, b1, contactParams, edgeId0, edgeId1)
			, edgeLine0(edgeLine0)
			, edgeLine1(edgeLine1)
		{
		}

		/*override*/ void updateContactPoint();
	};


	////////////////////////////////////////////////////////////////////////////////

	class BallVertexConnector :	public PolyConnector, 
								public Allocator<BallVertexConnector>
	{
	private:
		float radius;
		Vector3 offset;

		/*override*/ GeoPairType getConnectorType() const {return BALL_VERTEX_CONNECTOR;}

	public:
		BallVertexConnector(
			Body* b0, 
			Body* b1,
			const ContactParams& contactParams, 
			float radius,
			const Vector3& offset,
			int vertexId)
			: PolyConnector(b0, b1, contactParams, 0, vertexId)
			, radius(radius)
			, offset(offset)
		{}

		/*override*/ void updateContactPoint();
	};

	class BallEdgeConnector :	public PolyConnector, 
								public Allocator<BallEdgeConnector>
	{
	private:
		float radius;
		Vector3 offset;
		Vector3 normal;

		/*override*/ GeoPairType getConnectorType() const {return BALL_EDGE_CONNECTOR;}

	public:
		BallEdgeConnector(
			Body* b0, 
			Body* b1,
			const ContactParams& contactParams, 
			float radius,
			const Vector3& offset,
			const Vector3& normal,
			int edgeId)
			: PolyConnector(b0, b1, contactParams, 0, edgeId)
			, radius(radius)
			, offset(offset)
			, normal(normal)
		{}
		
		/*override*/ void updateContactPoint();
	};

	class BallPlaneConnector :	public PolyConnector, 
								public Allocator<BallPlaneConnector>
	{
	private:
		float radius;
		Vector3 offset;
		Vector3 normal;

		/*override*/ GeoPairType getConnectorType() const {return BALL_PLANE_CONNECTOR;}

	public:
		BallPlaneConnector(
			Body* b0, 
			Body* b1,
			const ContactParams& contactParams, 
			float radius,
			const Vector3& offset,
			const Vector3& normal,
			int faceId)
			: PolyConnector(b0, b1, contactParams, 0, faceId)
			, radius(radius)
			, offset(offset)
			, normal(normal)
		{}

		/*override*/ void updateContactPoint();
	};

} // namespace RBX