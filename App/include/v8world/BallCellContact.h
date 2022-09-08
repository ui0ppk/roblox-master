/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8World/CellContact.h"

namespace RBX {

	class Poly;
	class BallPlaneConnector;
	class BallEdgeConnector;
	class BallVertexConnector;

	namespace POLY {
		class Face;
		class Edge;
		class Vertex;
	}

	class BallCellContact 
		: public CellMeshContact
		, public Allocator<BallCellContact>
	{
	private:
		const POLY::Face* getFarthestPlane(float& planeToCenter, const Vector3& ballInCell);
		const POLY::Edge* getClosestEdge(const POLY::Face* face, float& edgeToCenter, const Vector3& ballInCell);
        const POLY::Edge* getClosestInVoronoiEdge(const POLY::Face* face, float& edgeToCenter, const Vector3& ballInCell);
		const POLY::Vertex* getClosestVertex(const POLY::Edge* edge, float& vertexToCenter, const Vector3& ballInCell);
	
		BallPlaneConnector* newBallPlaneConnector(const POLY::Face* face);
		BallEdgeConnector* newBallEdgeConnector(const POLY::Edge* edge);
		BallVertexConnector* newBallVertexConnector(const POLY::Vertex* vertex);

		const Ball* ball() const;
		const Poly* poly() const;

		/*override*/ void findClosestFeatures(ConnectorArray& newConnectors);
	public:
		BallCellContact(Primitive* p0, Primitive* p1, const Vector3int16& cell);
		~BallCellContact();

        void generateDataForMovingAssemblyStage(void); /*override*/
	};


} // namespace