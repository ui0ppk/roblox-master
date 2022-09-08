/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/BallPolyContact.h"
#include "V8Kernel/PolyConnectors.h"
#include "V8World/Ball.h"
#include "V8World/Poly.h"
#include "V8World/Mesh.h"
#include "V8World/Primitive.h"


namespace RBX {


BallPolyContact::BallPolyContact(Primitive* p0, Primitive* p1)
	: PolyContact(p0, p1)
{
	RBXASSERT(rbx_static_cast<Ball*>(p0->getGeometry()));
	RBXASSERT(rbx_static_cast<Poly*>(p1->getGeometry()));
}


void BallPolyContact::findClosestFeatures(ConnectorArray& newConnectors)
{
    if(!contactParams) 
    {
        generateDataForMovingAssemblyStage();
    }
    Vector3 ballInPoly = getPrimitive(1)->getCoordinateFrame().pointToObjectSpace(getPrimitive(0)->getCoordinateFrame().translation);
    float radius = ball()->getRadius();

    float planeToCenter;
    const POLY::Face* face = getFarthestPlane(planeToCenter, ballInPoly);
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
	    if (face->pointInExtrusion(ballInPoly)) 
        {
		    newConnectors.push_back(newBallPlaneConnector(face));
	    }
	    else 
        {
            float edgeToCenter;
            const POLY::Edge* edge = getClosestInVoronoiEdge(face, edgeToCenter, ballInPoly);
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
                edge = getClosestEdge(face, edgeToCenter, ballInPoly);
                float vertexToCenter;
                const POLY::Vertex* vertex = getClosestVertex(edge, vertexToCenter, ballInPoly);
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

const POLY::Face* BallPolyContact::getFarthestPlane(float& planeToCenter, const Vector3& ballInPoly)
{
	planeToCenter = -FLT_MAX;			// deep inside to start
	const POLY::Face* answer = NULL;
	for (size_t i = 0; i < poly()->getMesh()->numFaces(); ++i) {
		const POLY::Face* face = poly()->getMesh()->getFace(i);
		const Plane& plane = face->plane();
		float distance = plane.distance(ballInPoly);		// positive distance == away from this plane
		if (distance > planeToCenter) {
			answer = face;
			planeToCenter = distance;
		}
	}
	RBXASSERT(answer);
	return answer;
}

const POLY::Edge* BallPolyContact::getClosestEdge(const POLY::Face* face, float& edgeToCenter, const Vector3& ballInPoly)
{
	edgeToCenter = FLT_MAX;
	POLY::Edge* answer = NULL;
	for (size_t i = 0; i < face->numEdges(); ++i) {
		POLY::Edge* testEdge = face->getEdge(i);
		Line line = testEdge->computeLine();
		double distance = line.distance(ballInPoly);
		if (distance < edgeToCenter) {
			answer = testEdge;
			edgeToCenter = static_cast<float>(distance);
		}
	}
	RBXASSERT(answer);
	return answer;
}

const POLY::Edge* BallPolyContact::getClosestInVoronoiEdge(const POLY::Face* face, float& edgeToCenter, const Vector3& ballInPoly)
{
	edgeToCenter = FLT_MAX;
	POLY::Edge* answer = NULL;
	for (size_t i = 0; i < face->numEdges(); ++i) {
		POLY::Edge* testEdge = face->getEdge(i);
		Line line = testEdge->computeLine();
		double distance = line.distance(ballInPoly);
		if (distance < edgeToCenter && testEdge->pointInVaronoi(ballInPoly)) {
			answer = testEdge;
			edgeToCenter = static_cast<float>(distance);
		}
	}
	return answer;
}


const POLY::Vertex* BallPolyContact::getClosestVertex(const POLY::Edge* edge, float& vertexToCenter, const Vector3& ballInPoly)
{
	vertexToCenter = FLT_MAX;
	const POLY::Vertex* answer = NULL;
	for (size_t i = 0; i < 2; ++i) {
		const POLY::Vertex* testVertex = edge->getVertex(NULL, i);
		const Vector3& testOffset =  testVertex->getOffset();
		float distance = (ballInPoly - testOffset).magnitude();
		if (distance < vertexToCenter) {
			answer = testVertex;
			vertexToCenter = distance;
		}
	}
	RBXASSERT(answer);
	return answer;
}

BallPlaneConnector* BallPolyContact::newBallPlaneConnector(const POLY::Face* face)
{
    if(contactParams)
    {
	    return new BallPlaneConnector(	getPrimitive(0)->getBody(),
									    getPrimitive(1)->getBody(),
									    *contactParams,
									    ball()->getRadius(),
									    face->getVertexOffset(0),
									    face->normal(),
									    face->getId()	);			// id of this face
    }
    else
        return NULL;
}

BallEdgeConnector* BallPolyContact::newBallEdgeConnector(const POLY::Edge* edge)
{
    if(contactParams)
    {
	    return new BallEdgeConnector(	getPrimitive(0)->getBody(),
									    getPrimitive(1)->getBody(),
									    *contactParams,
									    ball()->getRadius(),
									    edge->getVertexOffset(NULL, 0),		// face == NULL means get vertex 0
									    edge->computeNormal(NULL),
									    edge->getId()		);		// id of edge 
    }
    else
        return NULL;
}



BallVertexConnector* BallPolyContact::newBallVertexConnector(const POLY::Vertex* vertex)
{
    if(contactParams)
    {
	    return new BallVertexConnector(	getPrimitive(0)->getBody(),
									    getPrimitive(1)->getBody(),
									    *contactParams,
									    ball()->getRadius(),
									    vertex->getOffset(),
									    vertex->getId()			);		// for comparing
    }
    else
        return NULL;
}


const Ball* BallPolyContact::ball() const
{
	return rbx_static_cast<const Ball*>(getConstPrimitive(0)->getConstGeometry());
}

const Poly* BallPolyContact::poly() const
{
	return rbx_static_cast<const Poly*>(getConstPrimitive(1)->getConstGeometry());
}

void BallPolyContact::generateDataForMovingAssemblyStage(void)
{
    Contact::generateDataForMovingAssemblyStage();
}


} // namespace