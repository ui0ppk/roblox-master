/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/BallCellContact.h"
#include "V8Kernel/PolyConnectors.h"
#include "V8World/Ball.h"
#include "V8World/Poly.h"
#include "V8World/Mesh.h"
#include "V8World/Primitive.h"
#include "V8World/MegaClusterPoly.h"
#include "V8DataModel/MegaCluster.h"

namespace RBX {
using namespace Voxel;


BallCellContact::BallCellContact(Primitive* p0, Primitive* p1, const Vector3int16& cell)
	: CellMeshContact(p0, p1, Vector3int32(cell))
{
	RBXASSERT(rbx_static_cast<Ball*>(p1->getGeometry()));
	RBXASSERT(rbx_static_cast<MegaClusterPoly*>(p0->getGeometry()));

    cellMesh = new POLY::Mesh;

    Vector3 size(kCELL_SIZE, kCELL_SIZE, kCELL_SIZE);
    Vector3 cellOffset = kCELL_SIZE * Vector3(cell) + Vector3(Voxel::kHALF_CELL, Voxel::kHALF_CELL, Voxel::kHALF_CELL);

    Grid* grid = static_cast<MegaClusterInstance*>(p0->getOwner())->getVoxelGrid();
    Cell cellData = grid->getCell(cell);

    CellOrientation orientation = cellData.solid.getOrientation();
    CellBlock type = cellData.solid.getBlock();

    switch( type )
    {
        case CELL_BLOCK_Solid:
        default:
            cellMesh->makeCell(size, cellOffset);
            break;
        case CELL_BLOCK_VerticalWedge:
            cellMesh->makeVerticalWedgeCell(size, cellOffset, (int)orientation);
            break;
        case CELL_BLOCK_HorizontalWedge:
            cellMesh->makeHorizontalWedgeCell(size, cellOffset, (int)orientation);
            break;
        case CELL_BLOCK_CornerWedge:
            cellMesh->makeCornerWedgeCell(size, cellOffset, (int)orientation);
            break;
        case CELL_BLOCK_InverseCornerWedge:
            cellMesh->makeInverseCornerWedgeCell(size, cellOffset, (int)orientation);
            break;
    }
}

BallCellContact::~BallCellContact()
{
	//resetBestPair(NULL);
    delete cellMesh;
}

void BallCellContact::findClosestFeatures(ConnectorArray& newConnectors)
{
    if(!contactParams) 
    {
        generateDataForMovingAssemblyStage();
    }

    Vector3 ballInCell = getPrimitive(0)->getCoordinateFrame().pointToObjectSpace(getPrimitive(1)->getCoordinateFrame().translation);
    float radius = ball()->getRadius();

    float planeToCenter;
    const POLY::Face* face = getFarthestPlane(planeToCenter, ballInCell);
    if (!face) 
    {
	    RBXASSERT(0);
	    return;
    }

    if (radius < planeToCenter) 
    {
	    ;	// bail out - we found a separating plane
    }
    else if (planeToCenter <= 0.0f) 
    {
	    newConnectors.push_back(newBallPlaneConnector(face));		// this should handle it
    }
    else 
    {		// 0.0 < planeToCenter <= radius
	    if (face->pointInExtrusion(ballInCell)) 
        {
		    newConnectors.push_back(newBallPlaneConnector(face));
	    }
	    else 
        {
            float edgeToCenter;
            const POLY::Edge* edge = getClosestInVoronoiEdge(face, edgeToCenter, ballInCell);
            if (edge)
            {
                if (radius < edgeToCenter) 
                {
                    ; // bail out - we are not inside the face, and far from the closest, valid edge
                }
                else 
                {
                    newConnectors.push_back(newBallEdgeConnector(edge));
                }
            }
            else 
            {
                edge = getClosestEdge(face, edgeToCenter, ballInCell);
                float vertexToCenter;
                const POLY::Vertex* vertex = getClosestVertex(edge, vertexToCenter, ballInCell);
                if (radius < vertexToCenter) 
                {
                    ; // bail out - we are not inside the face, not inside the edges, and too far from vertex;
                }
                else 
                {
                    newConnectors.push_back(newBallVertexConnector(vertex));
                }
            }
	    }
    }
}

const POLY::Face* BallCellContact::getFarthestPlane(float& planeToCenter, const Vector3& ballInCell)
{
	planeToCenter = -FLT_MAX;			// deep inside to start
	const POLY::Face* answer = NULL;
	for (size_t i = 0; i < getCellMesh()->numFaces(); ++i) {
		const POLY::Face* face = cellMesh->getFace(i);
		const Plane& plane = face->plane();
		float distance = plane.distance(ballInCell);		// positive distance == away from this plane
		if (distance > planeToCenter) {
			answer = face;
			planeToCenter = distance;
		}
	}
	RBXASSERT(answer);
	return answer;
}

const POLY::Edge* BallCellContact::getClosestEdge(const POLY::Face* face, float& edgeToCenter, const Vector3& ballInMC)
{
	edgeToCenter = FLT_MAX;
	POLY::Edge* answer = NULL;
	for (size_t i = 0; i < face->numEdges(); ++i) {
		POLY::Edge* testEdge = face->getEdge(i);
		Line line = testEdge->computeLine();
		double distance = line.distance(ballInMC);
		if (distance < edgeToCenter) {
			answer = testEdge;
			edgeToCenter = static_cast<float>(distance);
		}
	}
	RBXASSERT(answer);
	return answer;
}

const POLY::Edge* BallCellContact::getClosestInVoronoiEdge(const POLY::Face* face, float& edgeToCenter, const Vector3& ballInCell)
{
	edgeToCenter = FLT_MAX;
	POLY::Edge* answer = NULL;
	for (size_t i = 0; i < face->numEdges(); ++i) {
		POLY::Edge* testEdge = face->getEdge(i);
		Line line = testEdge->computeLine();
		double distance = line.distance(ballInCell);
		if (distance < edgeToCenter && testEdge->pointInVaronoi(ballInCell)) {
			answer = testEdge;
			edgeToCenter = static_cast<float>(distance);
		}
	}
	return answer;
}

const POLY::Vertex* BallCellContact::getClosestVertex(const POLY::Edge* edge, float& vertexToCenter, const Vector3& ballInMC)
{
	vertexToCenter = FLT_MAX;
	const POLY::Vertex* answer = NULL;
	for (size_t i = 0; i < 2; ++i) {
		const POLY::Vertex* testVertex = edge->getVertex(NULL, i);
		const Vector3& testOffset =  testVertex->getOffset();
		float distance = (ballInMC - testOffset).magnitude();
		if (distance < vertexToCenter) {
			answer = testVertex;
			vertexToCenter = distance;
		}
	}
	RBXASSERT(answer);
	return answer;
}

BallPlaneConnector* BallCellContact::newBallPlaneConnector(const POLY::Face* face)
{
    RBXASSERT(contactParams);
    if(contactParams)
    {
	    return new BallPlaneConnector(	getPrimitive(1)->getBody(),
									    getPrimitive(0)->getBody(),
									    *contactParams,
									    ball()->getRadius(),
									    face->getVertexOffset(0),
									    face->normal(),
									    face->getId()	);			// id of this face
    }
    else
        return NULL;
}

BallEdgeConnector* BallCellContact::newBallEdgeConnector(const POLY::Edge* edge)
{
    RBXASSERT(contactParams);
    if(contactParams)
    {
	    return new BallEdgeConnector(	getPrimitive(1)->getBody(),
									    getPrimitive(0)->getBody(),
									    *contactParams,
									    ball()->getRadius(),
									    edge->getVertexOffset(NULL, 0),		// face == NULL means get vertex 0
									    edge->computeNormal(NULL),
									    edge->getId()		);		// id of edge 
    }
    else
        return NULL;
}



BallVertexConnector* BallCellContact::newBallVertexConnector(const POLY::Vertex* vertex)
{
    RBXASSERT(contactParams);
    if(contactParams)
    {
	    return new BallVertexConnector(	getPrimitive(1)->getBody(),
									    getPrimitive(0)->getBody(),
									    *contactParams,
									    ball()->getRadius(),
									    vertex->getOffset(),
									    vertex->getId()			);		// for comparing
    }
    else
        return NULL;
}


const Ball* BallCellContact::ball() const
{
	return rbx_static_cast<const Ball*>(getConstPrimitive(1)->getConstGeometry());
}

const Poly* BallCellContact::poly() const
{
	return rbx_static_cast<const MegaClusterPoly*>(getConstPrimitive(0)->getConstGeometry());
}

void BallCellContact::generateDataForMovingAssemblyStage(void)
{
    Contact::generateDataForMovingAssemblyStage();
}


} // namespace
