#include "stdafx.h"

#include "V8World/Mesh.h"
#include "util/Math.h"

namespace RBX {

	namespace POLY {
/*
	WEDGE

	Back:	Looking along -z
		^ y
	3	0
	4	1	> x

	Front:	Looking along z
		^ y
	0	3
	2	5	> -x

	Right: Looking along -x
		^ y
	0
	1	2	> -z

	Left: Looking along x
		^ y
		3
	5	4	> z

	Bottom: Looking along -y
		^ -z
	2	5
	1	4	> -x

	Top: Looking along -y
		^ -z
	5	2
	3	0	> x

*/

/*
	BLOCK

vertID		x,y,z 
0			1,1,1
1			1,1,-1
2			1,-1,1
3			1,-1,-1
4			-1,1,1
5			-1,1,-1
6			-1,-1,1
7			-1,-1,-1

0			+x
1			+y
2			+z
3			-x
4			-y
5			-z
	Right: Looking at x
		^ y
	0	1
	2	3	> -z

	Top: Looking at y
		^ -z
	5	1
	4	0	> x

	Back:	Looking at z
		^ y
	4	0
	6	2	> x

	Left: Looking at -x
		^ y
	5	4
	7	6	> z

	Bottom: Looking at -y
		^ -z
	3	7
	2   6	> -x

	Front:	Looking at -z
		^ y
	1	5
	3	7	> -x
*/

void Mesh::makeBlock(const Vector3& size)
{
	float x = 0.5f * size.x;
	float y = 0.5f * size.y;
	float z = 0.5f * size.z;

	clear(); 

	// important - don't reallocate while we are building
	vertices.reserve(8);
	faces.reserve(6);
	edges.reserve(12);

	addVertex(x,  y,  z);
	addVertex(x,  y, -z);
	addVertex(x, -y,  z);
	addVertex(x, -y, -z);
	addVertex(-x, y,  z);
	addVertex(-x, y, -z);
	addVertex(-x, -y, z);
	addVertex(-x, -y, -z);

	// counter clockwise
	addFace(1, 0, 2, 3);	// right
	addFace(1, 5, 4, 0);	// top
	addFace(0, 4, 6, 2);	// back
	addFace(4, 5, 7, 6);	// left
	addFace(7, 3, 2, 6);	// bottom
	addFace(5, 1, 3, 7);	// front
}


void Mesh::makeCell(const Vector3& size, const Vector3& offset)
{
	float x = 0.5f * size.x;
	float y = 0.5f * size.y;
	float z = 0.5f * size.z;

    float sumx = x + offset.x;
    float diffx = offset.x - x;
    float sumy = y + offset.y;
    float diffy = offset.y - y;
    float sumz = z + offset.z;
    float diffz = offset.z - z;

	clear(); 

	// important - don't reallocate while we are building
	vertices.reserve(8);
	faces.reserve(6);
	edges.reserve(12);

	addVertex(sumx,  sumy,  sumz);
	addVertex(sumx,  sumy, diffz);
	addVertex(sumx, diffy,  sumz);
	addVertex(sumx, diffy, diffz);
	addVertex(diffx, sumy,  sumz);
	addVertex(diffx, sumy, diffz);
	addVertex(diffx, diffy, sumz);
	addVertex(diffx, diffy, diffz);

	// counter clockwise
	addFace(1, 0, 2, 3);	// right
	addFace(1, 5, 4, 0);	// top
	addFace(0, 4, 6, 2);	// back
	addFace(4, 5, 7, 6);	// left
	addFace(7, 3, 2, 6);	// bottom
	addFace(5, 1, 3, 7);	// front
}

void Mesh::makeVerticalWedgeCell(const Vector3& size, const Vector3& offset, const int& orient)
{
	float x = 0.5f * size.x;
	float y = 0.5f * size.y;
	float z = 0.5f * size.z;

    float sumx = x + offset.x;
    float diffx = offset.x - x;
    float sumy = y + offset.y;
    float diffy = offset.y - y;
    float sumz = z + offset.z;
    float diffz = offset.z - z;

	clear(); 

	// important - don't reallocate while we are building
	vertices.reserve(6);
	faces.reserve(5);
	edges.reserve(9);

    // add wedge base verts
	addVertex(sumx, diffy,  sumz);
	addVertex(sumx, diffy, diffz);
    addVertex(diffx, diffy, diffz);
    addVertex(diffx, diffy, sumz);
    addFace(0, 3, 2, 1);

    switch( orient )
    {
        case 0: // UpperLeft
        default:
            addVertex(sumx, sumy, diffz);
            addVertex(diffx, sumy, diffz);
	        addFace(0, 4, 5, 3);
	        addFace(1, 2, 5, 4);
	        addFace(1, 4, 0);
	        addFace(2, 3, 5);
            break;
        case 1: // UpperRight
            addVertex(diffx, sumy, diffz);
            addVertex(diffx, sumy, sumz);
	        addFace(4, 2, 3, 5);
	        addFace(0, 1, 4, 5);
	        addFace(0, 5, 3);
	        addFace(2, 4, 1);
            break;
        case 2: // LowerRight
            addVertex(diffx, sumy, sumz);
            addVertex(sumx, sumy, sumz);
	        addFace(0, 5, 4, 3);
	        addFace(1, 2, 4, 5);
	        addFace(3, 4, 2);
	        addFace(0, 1, 5);
            break;
        case 3: // LowerLeft
            addVertex(sumx, sumy, sumz);
            addVertex(sumx, sumy, diffz);
	        addFace(0, 1, 5, 4);
	        addFace(3, 4, 5, 2);
	        addFace(0, 4, 3);
	        addFace(1, 2, 5);
            break;
    }
}

void Mesh::makeHorizontalWedgeCell(const Vector3& size, const Vector3& offset, const int& orient)
{
	float x = 0.5f * size.x;
	float y = 0.5f * size.y;
	float z = 0.5f * size.z;

    float sumx = x + offset.x;
    float diffx = offset.x - x;
    float sumy = y + offset.y;
    float diffy = offset.y - y;
    float sumz = z + offset.z;
    float diffz = offset.z - z;

	clear(); 

	// important - don't reallocate while we are building
	vertices.reserve(6);
	faces.reserve(5);
	edges.reserve(9);

    switch( orient )
    {
        case 0: // UpperLeft
        default:
            addVertex(sumx, diffy, sumz);
            addVertex(sumx, diffy, diffz);
            addVertex(diffx, diffy, diffz);
            addVertex(sumx, sumy, sumz);
            addVertex(sumx, sumy, diffz);
            addVertex(diffx, sumy, diffz);
            break;
        case 1: // UpperRight
            addVertex(sumx, diffy, diffz);
            addVertex(diffx, diffy, diffz);
            addVertex(diffx, diffy, sumz);
            addVertex(sumx, sumy, diffz);
            addVertex(diffx, sumy, diffz);
            addVertex(diffx, sumy, sumz);
            break;
        case 2: // LowerRight
            addVertex(diffx, diffy, diffz);
            addVertex(diffx, diffy, sumz);
            addVertex(sumx, diffy, sumz);
            addVertex(diffx, sumy, diffz);
            addVertex(diffx, sumy, sumz);
            addVertex(sumx, sumy, sumz);
            break;
        case 3: // LowerLeft
            addVertex(diffx, diffy, sumz);
            addVertex(sumx, diffy, sumz);
            addVertex(sumx, diffy, diffz);
            addVertex(diffx, sumy, sumz);
            addVertex(sumx, sumy, sumz);
            addVertex(sumx, sumy, diffz);
            break;
    }
    addFace(3,4,5);
    addFace(2,1,0);    
    addFace(0,3,5,2);
    addFace(0,1,4,3);
    addFace(1,2,5,4);

}

void Mesh::makeCornerWedgeCell(const Vector3& size, const Vector3& offset, const int& orient)
{
	float x = 0.5f * size.x;
	float y = 0.5f * size.y;
	float z = 0.5f * size.z;

    float sumx = x + offset.x;
    float diffx = offset.x - x;
    float sumy = y + offset.y;
    float diffy = offset.y - y;
    float sumz = z + offset.z;
    float diffz = offset.z - z;

	clear(); 

	// important - don't reallocate while we are building
	vertices.reserve(4);
	faces.reserve(4);
	edges.reserve(6);

    switch( orient )
    {
        case 0: // UpperLeft
        default:
            addVertex(sumx, diffy, sumz);
            addVertex(sumx, diffy, diffz);
            addVertex(diffx, diffy, diffz);
            addVertex(sumx, sumy, diffz);
            break;
        case 1: // UpperRight
            addVertex(sumx, diffy, diffz);
            addVertex(diffx, diffy, diffz);
            addVertex(diffx, diffy, sumz);
            addVertex(diffx, sumy, diffz);
            break;
        case 2: // LowerRight
            addVertex(diffx, diffy, diffz);
            addVertex(diffx, diffy, sumz);
            addVertex(sumx, diffy, sumz);
            addVertex(diffx, sumy, sumz);
            break;
        case 3: // LowerLeft
            addVertex(diffx, diffy, sumz);
            addVertex(sumx, diffy, sumz);
            addVertex(sumx, diffy, diffz);
            addVertex(sumx, sumy, sumz);
            break;
    }
    addFace(2,1,0);
    addFace(0,1,3);
    addFace(1,2,3);
    addFace(0,3,2);

}

void Mesh::makeInverseCornerWedgeCell(const Vector3& size, const Vector3& offset, const int& orient)
{
	float x = 0.5f * size.x;
	float y = 0.5f * size.y;
	float z = 0.5f * size.z;

    float sumx = x + offset.x;
    float diffx = offset.x - x;
    float sumy = y + offset.y;
    float diffy = offset.y - y;
    float sumz = z + offset.z;
    float diffz = offset.z - z;

	clear(); 

	// important - don't reallocate while we are building
	vertices.reserve(7);
	faces.reserve(7);
	edges.reserve(12);

    switch( orient )
    {
        case 0: // UpperLeft
        default:
            addVertex(sumx, diffy, sumz);
            addVertex(sumx, diffy, diffz);
            addVertex(diffx, diffy, diffz);
            addVertex(diffx, diffy, sumz);
            addVertex(sumx, sumy, sumz);
            addVertex(sumx, sumy, diffz);
            addVertex(diffx, sumy, diffz);
            break;
        case 1: // UpperRight
            addVertex(sumx, diffy, diffz);
            addVertex(diffx, diffy, diffz);
            addVertex(diffx, diffy, sumz);
            addVertex(sumx, diffy, sumz);
            addVertex(sumx, sumy, diffz);
            addVertex(diffx, sumy, diffz);
            addVertex(diffx, sumy, sumz);
            break;
        case 2: // LowerRight
            addVertex(diffx, diffy, diffz);
            addVertex(diffx, diffy, sumz);
            addVertex(sumx, diffy, sumz);
            addVertex(sumx, diffy, diffz);
            addVertex(diffx, sumy, diffz);
            addVertex(diffx, sumy, sumz);
            addVertex(sumx, sumy, sumz);
            break;
        case 3: // LowerLeft
            addVertex(diffx, diffy, sumz);
            addVertex(sumx, diffy, sumz);
            addVertex(sumx, diffy, diffz);
            addVertex(diffx, diffy, diffz);
            addVertex(diffx, sumy, sumz);
            addVertex(sumx, sumy, sumz);
            addVertex(sumx, sumy, diffz);
            break;
    }
    addFace(3,4,6);
    addFace(3,6,2);
    addFace(0,4,3);
    addFace(0,1,5,4);
    addFace(1,2,6,5);
    addFace(0,3,2,1);
    addFace(4,5,6);

}

void Mesh::makeWedge(const Vector3& size)
{
	float x = 0.5f * size.x;
	float y = 0.5f * size.y;
	float z = 0.5f * size.z;

	clear(); 

	// important - don't reallocate while we are building
	vertices.reserve(6);
	faces.reserve(5);
	edges.reserve(9);

	addVertex(x, y, z);
	addVertex(x, -y, z);
	addVertex(x, -y, -z);
	addVertex(-x, y, z);
	addVertex(-x, -y, z);
	addVertex(-x, -y, -z);

	addFace(0, 3, 4, 1);	// back
	addFace(3, 0, 2, 5);	// front / top
	addFace(0, 1, 2);		// right
	addFace(3, 5, 4);		// left
	addFace(5, 2, 1, 4);	// bottom
}

void Mesh::makePrism(const Vector3_2Ints& params, Vector3& cofm)
{
	int sides = params.int1;
	Vector3 size = params.vectPart;
	RBXASSERT(sides < 21);

	// no assert for this case since it happens safely during initialization
	if( sides < 3 )
		sides = 6;

	clear(); 

	// Set cofm to zero and recompute for this prism
	cofm = Vector3::zero();

	// Arrays of indices collected during construction of side walls to be used for end cap face creation.
	int *baseVertIndices = new int[sides];
	int *topVertIndices = new int[sides];

	vertices.reserve(2*sides);
	faces.reserve(sides + 2);
	edges.reserve(3*sides);

	// Start creation of prism - this sets the maximum width, which is the x-width in properties
	float alpha = (float)(180.0/sides);
	Vector2 startPoint = Math::polygonStartingPoint(sides, size.x);

	// Apply 2D startPoint from Math to 3D starting point for the polyhedron
	G3D::Vector3 Sa(startPoint.x, 0.0, startPoint.y);
	G3D::Vector3 Sb(0, 0, 0);
	G3D::Vector3 axis(0, 1, 0);
	
	// start at bottom corner vertex for first side wall
	addVertex(startPoint.x, (float)(-0.5*size.y), startPoint.y);
	baseVertIndices[0] = 0;
	addVertex(startPoint.x, (float)(0.5*size.y), startPoint.y);
	topVertIndices[0] = 1;
	cofm += Sa;

	// Walk perimeter of prism until last closing section is reached
	for( int i = 0; i < sides - 1; ++i )
	{
		G3D::Vector3 baseNorm = G3D::normalize(axis.cross(Sa));
		G3D::Vector3 SbRevNorm = G3D::normalize(-Sa);
		float effRad = Sa.magnitude();
		double sideLen = 2.0 * effRad * sin(0.0174533 * alpha);
		
		// update Sa to next facet
		Sb = Sa + sideLen * sin(0.0174533 * alpha) * SbRevNorm + sideLen * cos(0.0174533 * alpha) * baseNorm;
		cofm += Sb;
		Sa = Sb;

		addVertex(Sb.x, (float)(-0.5*size.y), Sb.z);
		baseVertIndices[i+1] = 2*(i+1);
		addVertex(Sb.x, (float)(0.5*size.y), Sb.z);
		topVertIndices[i+1] = 2*(i+1) + 1;

		addFace(2*i, 2*i+2, 2*i+3, 2*i+1);
	}

	// Finish off last side face
	addFace(2*(sides-1), 0, 1, 2*(sides-1)+1);
	cofm.x = cofm.y = cofm.z = 0.0f;

	// Now the base and top
	addFace(sides, topVertIndices, false);
	addFace(sides, baseVertIndices, true);

	delete [] topVertIndices;
	delete [] baseVertIndices;
}

void Mesh::makePyramid(const Vector3_2Ints& params, Vector3& cofm )
{
	int sides = params.int1;
	Vector3 size = params.vectPart;
	RBXASSERT(sides < 21);

	// no assert for this case since it happens safely during initialization
	if( sides < 3 )
		sides = 6;

	clear(); 

	// Set cofm to zero and recompute for this prism
	cofm = Vector3::zero();

	// Arrays of indices collected during construction of side walls to be used for end cap face creation.
	int *baseVertIndices = new int[sides];

	vertices.reserve(sides + 1);
	faces.reserve(sides + 1);
	edges.reserve(2*sides);

	// Start creation of prism
	float alpha = (float)(180.0/sides);
	Vector2 startPoint = Math::polygonStartingPoint(sides, size.x);

	float xStart = startPoint.x;
	float zStart = startPoint.y;
	G3D::Vector3 Sa(xStart, 0.0, zStart);
	G3D::Vector3 Sb(0, 0, 0);
	G3D::Vector3 axis(0, 1, 0);

	// One vertex at apex
	// Vertex 0
	addVertex(0.0f, (float)(0.5*size.y), 0.0f);
	
	// start at bottom corner vertex for first side wall
	// Vertex 1
	addVertex(xStart, (float)(-0.5*size.y), zStart);
	baseVertIndices[0] = 1;

	// start accumulating cofm info.
	cofm += Sa;

	// Walk perimeter until last closing section is reached
	for( int i = 0; i < sides - 1; ++i )
	{
		G3D::Vector3 baseNorm = G3D::normalize(axis.cross(Sa));
		G3D::Vector3 SbRevNorm = G3D::normalize(-Sa);
		float effRad = Sa.magnitude();
		double sideLen = 2.0 * effRad * sin(0.0174533 * alpha);
		
		// update Sa to next facet
		Sb = Sa + sideLen * sin(0.0174533 * alpha) * SbRevNorm + sideLen * cos(0.0174533 * alpha) * baseNorm;
		cofm += Sb;
		Sa = Sb;

		addVertex(Sb.x, (float)(-0.5*size.y), Sb.z);
		baseVertIndices[i+1] = i+2;

		addFace(i+1, i+2, 0);
	}

	// Finish off last side face
	addFace(sides, 1, 0);
	cofm.x = cofm.z = 0.0f;
	cofm.y = (float)(-size.y/6.0);

	// Now the base
	addFace(sides, baseVertIndices, true);

	delete [] baseVertIndices;
}

void Mesh::makeParallelRamp( const Vector3& size, Vector3& cofm )
{
	float x = 0.5f * size.x;
	float y = 0.5f * size.y;
	float z = 0.5f * size.z;

	//float connectionThickness = RBX::PartInstance::brickHeight();
	// Move away from 1.2 form factor
	float connectionThickness = 1.0f;

	clear(); 

	// important - don't reallocate while we are building
	vertices.reserve(8);
	faces.reserve(6);
	edges.reserve(12);

	addVertex(x,  -y + connectionThickness,  z);
	addVertex(x,  -y + connectionThickness, -z);
	addVertex(x, -y,  z);
	addVertex(x, -y, -z);
	addVertex(-x, y,  z);
	addVertex(-x, y, -z);
	addVertex(-x, y - connectionThickness,  z);
	addVertex(-x, y - connectionThickness, -z);

	addFace(1, 0, 2, 3);	// Legacy norm: x
	addFace(1, 5, 4, 0);	// Legacy norm: y
	addFace(0, 4, 6, 2);	// Legacy norm: z
	addFace(4, 5, 7, 6);	// Legacy norm: -x
	addFace(7, 3, 2, 6);	// Legacy norm: -y
	addFace(5, 1, 3, 7);	// Legacy norm: -z
}

void Mesh::makeRightAngleRamp( const Vector3& size, Vector3& cofm )
{
	float x = 0.5f * size.x;
	float y = 0.5f * size.y;
	float z = 0.5f * size.z;

	//float connectionThickness = RBX::PartInstance::brickHeight();
	// Move away from 1.2 form factor
	float connectionThickness = 1.0f;

	clear(); 

	// important - don't reallocate while we are building
	vertices.reserve(8);
	faces.reserve(6);
	edges.reserve(12);

	addVertex(x,  -y,  z);
	addVertex(x,  -y, -z);
	addVertex(x - connectionThickness, -y,  z);
	addVertex(x - connectionThickness, -y, -z);
	addVertex(-x, y,  z);
	addVertex(-x, y, -z);
	addVertex(-x, y - connectionThickness,  z);
	addVertex(-x, y - connectionThickness, -z);

	addFace(1, 0, 2, 3);	// Legacy norm: x
	addFace(1, 5, 4, 0);	// Legacy norm: y
	addFace(0, 4, 6, 2);	// Legacy norm: z
	addFace(4, 5, 7, 6);	// Legacy norm: -x
	addFace(7, 3, 2, 6);	// Legacy norm: -y
	addFace(5, 1, 3, 7);	// Legacy norm: -z
}

void Mesh::makeCornerWedge( const Vector3& size, Vector3& cofm )
{
	float x = 0.5f * size.x;
	float y = 0.5f * size.y;
	float z = 0.5f * size.z;

	clear(); 

	// important - don't reallocate while we are building
	vertices.reserve(5);
	faces.reserve(5);
	edges.reserve(8);

	addVertex(x, -y, z);
	addVertex(x, -y, -z);
	addVertex(-x, -y, -z);
	addVertex(-x, -y, z);
	addVertex(x, y, -z);

	addFace(0, 1, 4);		// Legacy norm: x
	addFace(0, 4, 3);		// Legacy norm: z
	addFace(2, 3, 4);		// Legacy norm: -x
	addFace(1, 0, 3, 2);	// Legacy norm: -y
	addFace(1, 2, 4);		// Legacy norm: -z

	// calculate center-of-mass here: for a corner-wedge should lie 1/4 of the way up from the base (height is -y/2)
	//     and in the center of the rectangular cross-section there
	cofm = Vector3(x/4.0f, -y/2.0f, -z/4.0f);
}

void Mesh::clear()
{
	vertices.clear();	
	edges.clear();
	faces.clear();
}

void Mesh::addVertex(float x, float y, float z)
{
	size_t id = vertices.size();
	vertices.push_back(Vertex(id, Vector3(x, y, z)));
}

void Mesh::addFace(size_t i, size_t j, size_t k)
{
	Edge* e0 = findOrMakeEdge(i, j);
	Edge* e1 = findOrMakeEdge(j, k);
	Edge* e2 = findOrMakeEdge(k, i);

	size_t id = faces.size();
	faces.push_back(Face(id, e0, e1, e2));
	Face* face = &faces[id];

	e0->addFace(face);
	e1->addFace(face);
	e2->addFace(face);

	face->initPlane();
}

void Mesh::addFace(size_t i, size_t j, size_t k, size_t l)
{
	Edge* e0 = findOrMakeEdge(i, j);
	Edge* e1 = findOrMakeEdge(j, k);
	Edge* e2 = findOrMakeEdge(k, l);
	Edge* e3 = findOrMakeEdge(l, i);

	size_t id = faces.size();
	faces.push_back(Face(id, e0, e1, e2, e3));
	Face* face = &faces[id];

	e0->addFace(face);
	e1->addFace(face);
	e2->addFace(face);
	e3->addFace(face);

	face->initPlane();
}

void Mesh::addFace( int numVerts, int vertIndexList[], bool reverseOrder = false )
{
	size_t id = faces.size();
	Edge* anEdge = NULL;
	std::vector<Edge*> faceEdges;

	// find or make the edges and add them to a faceEdge container
	if( !reverseOrder )
	{
		for( int i = 0; i < numVerts-1; i++ )
		{
			anEdge = findOrMakeEdge(vertIndexList[i], vertIndexList[i+1]);
			if( anEdge )
				faceEdges.push_back(anEdge);
		}
		anEdge = findOrMakeEdge(vertIndexList[numVerts-1], vertIndexList[0]);
		if( anEdge )
			faceEdges.push_back(anEdge);
	}
	else
	{
		for( int i = 0; i < numVerts-1; i++ )
		{
			anEdge = findOrMakeEdge(vertIndexList[numVerts-1-i], vertIndexList[numVerts-2-i]);
			if( anEdge )
				faceEdges.push_back(anEdge);
		}
		anEdge = findOrMakeEdge(vertIndexList[0], vertIndexList[numVerts-1]);
		if( anEdge )
			faceEdges.push_back(anEdge);
	}

	
	// Create a face based on this edge list and add it to the face container
	faces.push_back(Face(id, faceEdges));
	Face* face = &faces[id];
	
	// Set each edge's face
	if( !reverseOrder )
	{
		for( int i = 0; i < numVerts; i++ )
		{
			anEdge = faceEdges[i];
			faceEdges.at(i)->addFace(face);
		}
	}
	else
	{
		for( int i = 0; i < numVerts; i++ )
		{
			anEdge = faceEdges[numVerts-1-i];
			faceEdges.at(numVerts-1-i)->addFace(face);
		}
	}


	face->initPlane(); 
	
}

Edge* Mesh::findOrMakeEdge(size_t v0, size_t v1)
{
	Vertex* vert0 = &vertices[v0];
	Vertex* vert1 = &vertices[v1];
	if (Edge* found = vert0->findEdge(vert1)) {
		RBXASSERT(found->getVertex(NULL, 0) == vert1);		// should be backwards - this is second face
		RBXASSERT(found->getVertex(NULL, 1) == vert0);
		RBXASSERT(found->getForward());
		RBXASSERT(!found->getBackward());
		return found;
	}
	else {
		return addEdge(vert0, vert1);
	}
}

Edge* Mesh::addEdge(Vertex* vert0, Vertex* vert1)
{
	size_t id = edges.size();
	Edge edge(id, vert0, vert1);
	edges.push_back(edge);
	Edge* answer = &edges[id];
	vert0->addEdge(answer);
	vert1->addEdge(answer);
	return answer;
}



bool Mesh::pointInMesh(const Vector3& point) const
{
	for (size_t i = 0; i < numFaces(); ++i) {
		if (!getFace(i)->plane().pointOnOrBehind(point)) {
			return false;
		}
	}
	return true;
}


const Face* Mesh::findFaceIntersection(const Vector3& inside, const Vector3& outside) const
{
	RBXASSERT(pointInMesh(inside));
	RBXASSERT(!pointInMesh(outside));
	
	RbxRay ray = RbxRay::fromOriginAndDirection(inside, (outside-inside).direction());

	Vector3 tempHitPoint;

	for (size_t i = 0; i < numFaces(); ++i) {
		const Face* face = getFace(i);
		if (rayIntersectsFace(ray, face, tempHitPoint)) {
			return face;
		}
	}
	RBXASSERT(0);

	return NULL;
}

void Mesh::findFaceIntersections(const Vector3& p0, const Vector3& p1, const Face* &f0, const Face* &f1) const
{
	RBXASSERT(!pointInMesh(p0));
	RBXASSERT(!pointInMesh(p1));
	f0 = NULL;
	f1 = NULL;
	Line line = Line::fromTwoPoints(p0, p1);
	
	for (size_t i = 0; i < numFaces(); ++i) {
		const Face* face = getFace(i);
		if (lineIntersectsFace(line, face)) {
			if (!f0) {
				f0 = face;
			}
			else {
				f1 = face;
				return;
			}
		}
	}
}


bool Mesh::rayIntersectsFace(const RbxRay& ray, const Face* face, Vector3& intersection) const
{
	const Plane& plane = face->plane();
	intersection = ray.intersectionPlane(plane);

	if (intersection == Vector3::inf()) {
		return false;
	}
	else {
		if (face->pointInFaceBorders(intersection)) {
			return true;
		}
		else {
			intersection = Vector3::inf();
			return false;
		}
	}
}

bool Mesh::lineIntersectsFace(const Line& line, const Face* face) const
{
	Vector3 point = line.intersection(face->plane());
	if (point == Vector3::inf()) {
		return false;
	}
	else {
		return face->pointInFaceBorders(point);
	}
}



const Vertex* Mesh::farthestVertex(const Vector3& direction) const
{
	const Vertex* answer = NULL;
	float farthestDistance = -FLT_MAX;

	for (size_t i = 0; i < numVertices(); ++i) {
		const Vertex* vertex = getVertex(i);
		float dot = direction.dot(vertex->getOffset());
		if (dot > farthestDistance) {
			farthestDistance = dot;
			answer = vertex;
		}
	}
	return answer;
}

bool Mesh::hitTest(const RbxRay& ray, Vector3& hitPoint, Vector3& surfaceNormal) const
{
	float distance = FLT_MAX;
	int intersects = 0;

	for (size_t i = 0; i < numFaces(); ++i) {
		const Face* face = getFace(i);
		Vector3 tempHit;
		if (rayIntersectsFace(ray, face, tempHit)) {
			intersects++;
			float tempDistance = (tempHit - ray.origin()).squaredMagnitude();
			RBXASSERT(tempDistance >= 0.0f);
			if (tempDistance < distance) {
				hitPoint = tempHit;
				distance = tempDistance;
                surfaceNormal = face->normal();
			}
		}
	}
	return (intersects > 0);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Face::initPlane()
{
	outwardPlane = Plane(getVertexOffset(0), getVertexOffset(1), getVertexOffset(2));
}

const Face* Vertex::getFace(size_t i) const
{
	return edges[i]->getVertexFace(this);
}



Face::Face(size_t id, Edge* e0, Edge* e1, Edge* e2) 
: id(id)
{
	edges.push_back(e0);
	edges.push_back(e1);
	edges.push_back(e2);
}

Face::Face(size_t id, Edge* e0, Edge* e1, Edge* e2, Edge* e3) 
: id(id)
{
	edges.push_back(e0);
	edges.push_back(e1);
	edges.push_back(e2);
	edges.push_back(e3);
}

Face::Face( size_t id, std::vector<Edge*>& edgeList ) 
: id(id)
{
	for( unsigned int i = 0; i < edgeList.size(); i++  )
		edges.push_back(edgeList[i]);
}

// TODO _ turn back on after identifying bug here
#pragma optimize( "", off )
	bool Face::pointInFaceBorders(const Vector3& point) const
	{
		for (size_t i = 0; i < numEdges(); ++i) {
			const Edge* edge = getEdge(i);
			const Face* sideFace = edge->otherFace(this);
			if (!sideFace->plane().pointOnOrBehind(point)) {
				return false;
			}
		}
		return true;
	}
#pragma optimize( "", on )



int Face::findInternalExtrusionIntersection(const Vector3& p0, const Vector3& p1) const
{
	for (size_t i = 0; i < numEdges(); ++i) {
		if (lineCrossesExtrusionSideBelow(p0, p1, i)) {
			return i;
		}
	}
	return -1;
}

int Face::getInternalExtrusionIntersection(const Vector3& pBelowInside, const Vector3& pBelowOutside) const
{
	RBXASSERT(pointInInternalExtrusion(pBelowInside));
	RBXASSERT(!pointInInternalExtrusion(pBelowOutside));

	for (size_t i = 0; i < numEdges(); ++i) {
		if (lineCrossesExtrusionSide(pBelowInside, pBelowOutside, i)) {
			return i;
		}
	}
	return -1;
}


void Face::findInternalExtrusionIntersections(const Vector3& p0, const Vector3& p1, int& side0, int& side1) const
{
	RBXASSERT(!pointInInternalExtrusion(p0));
	RBXASSERT(!pointInInternalExtrusion(p1));

	side0 = -1;
	side1 = -1;
	int found = 0;

	for (size_t i = 0; i < numEdges(); ++i) {
		if (lineCrossesExtrusionSideBelow(p0, p1, i)) {
			found++;
			if (side0 == -1) {
				side0 = i;
			}
			else {
				//RBXASSERT(side1 == -1);
				side1 = i;
			}
		}
	}
}


bool Face::lineCrossesExtrusionSideBelow(const Vector3& p0, const Vector3& p1, size_t edgeId) const
{
	Plane sidePlane = getSidePlane(edgeId);

	if (sidePlane.halfSpaceContains(p0) != sidePlane.halfSpaceContains(p1)) {		// points must be on opposite sides!

		Line line = Line::fromTwoPoints(p0, p1);

		Vector3 pointOnSide = line.intersection(sidePlane);

		if (plane().pointOnOrBehind(pointOnSide)) {

			return pointInExtrusionSide(pointOnSide, sidePlane, edgeId);
		}
	}

	return false;
}


bool Face::lineCrossesExtrusionSide(const Vector3& p0, const Vector3& p1, size_t edgeId) const
{
	Plane sidePlane = getSidePlane(edgeId);

	if (sidePlane.halfSpaceContains(p0) != sidePlane.halfSpaceContains(p1)) {		// points must be on opposite sides!

		Line line = Line::fromTwoPoints(p0, p1);

		Vector3 pointOnSide = line.intersection(sidePlane);

		return pointInExtrusionSide(pointOnSide, sidePlane, edgeId);
	}

	return false;
}



bool Face::pointInExtrusionSide(const Vector3& pointOnSide, const Plane& sidePlane, size_t edgeId) const
{
	Vector3 sidePlaneV = plane().normal().cross(sidePlane.normal());

	Plane sidePlane0 = Plane(sidePlaneV, getEdge(edgeId)->getVertexOffset(this, 0));

	if (sidePlane0.halfSpaceContains(pointOnSide)) {

		Plane sidePlane1 = Plane(-sidePlaneV, getEdge(edgeId)->getVertexOffset(this, 1));

		if (sidePlane1.halfSpaceContains(pointOnSide)) {
			return true;
		}
	}
	return false;
}

Vector3 Face::getCentroid( void ) const
{
	Vector3 centroid = Vector3::zero();
	for( size_t i = 0; i < numVertices(); ++i )
		centroid += getVertex(i)->getOffset();

	centroid /= (float)numVertices();

	return centroid;
}

void Face::getOrientedBoundingBox( const Vector3& xDir, const Vector3& yDir, Vector3& boxMin, Vector3& boxMax, Vector3& boxCenter ) const
{
	// This function finds a bounding box that is oriented with the x and y vector passed in.  It computes the min corner,
	// max corner, and the center of this box, in body coords.
	size_t numVerts = numVertices();
	Vector3 centroid = getCentroid();
	boxMin = Vector3::zero();
	boxMax = Vector3::zero();
	float xMax = -1e10f;
	float yMax = -1e10f;
	float xMin = 1e10f;
	float yMin = 1e10f;

	for( size_t i = 0; i < numVerts; i++ )
	{
		// max vector
		if( (getVertex(i)->getOffset() - centroid).dot(xDir) > xMax )
			xMax = (getVertex(i)->getOffset() - centroid).dot(xDir);
		if( (getVertex(i)->getOffset() - centroid).dot(yDir) > yMax )
			yMax = (getVertex(i)->getOffset() - centroid).dot(yDir);
		boxMax = centroid + xDir * xMax + yDir * yMax;

		// Min vector
		if( (getVertex(i)->getOffset() - centroid).dot(xDir) < xMin )
			xMin = (getVertex(i)->getOffset() - centroid).dot(xDir);
		if( (getVertex(i)->getOffset() - centroid).dot(yDir) < yMin )
			yMin = (getVertex(i)->getOffset() - centroid).dot(yDir);
		boxMin = centroid + xDir * xMin + yDir * yMin;
	}

	boxCenter = (boxMax + boxMin) * 0.5;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Edge* Vertex::findEdge(const Vertex* other)
{
	for (size_t i = 0; i < edges.size(); ++i) {
		Edge* e = edges[i];
		RBXASSERT(e->contains(this));
		if (e->contains(other)) {
			return e;
		}
	}
	return NULL;
}

const Edge* Vertex::recoverEdge(const Vertex* v0, const Vertex* v1) {
	for (size_t i = 0; i < v0->numEdges(); ++i) {
		Edge* edge = v0->getEdge(i);
		if (edge->contains(v1)) {
			return edge;
		}
	}
	RBXASSERT(0);
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Edge::pointInVaronoi(const Vector3& point) const
{
	const Vector3& v0 = getVertexOffset(NULL, 0);
	const Vector3& v1 = getVertexOffset(NULL, 1);

	Vector3 v0v1 = v1 - v0;
	Vector3 v0p = point - v0;
	Vector3 v1p = point - v1;

	if (v0v1.dot(v0p) < 0.0) {
		return false;
	}
	else if (v0v1.dot(v1p) > 0.0) {
		return false;
	}
	return true;
}




} // namespace POLY

} // namespace RBX
