/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8World/PolyContact.h"
#include "V8Kernel/ContactParams.h"

namespace RBX {

	class Poly;
	class PolyPolyContact;
	class FaceVertexConnector;
	class FaceEdgeConnector;
	class EdgeEdgeConnector;

	namespace POLY {
		class Face;
		class Edge;
		class Vertex;
		class Mesh;
	}



	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////

	class PolyPair
	{
	protected:
		Primitive* primitive[2];
		ContactParams contactParams;

		const Poly* poly0() const;
		const Poly* poly1() const;

		const Poly* poly(size_t i) {
			return (i == 0) ? poly0() : poly1();
		}

		/*implement*/ virtual bool isFaceFace() const = 0;

	public:
		PolyPair(Primitive* p0, Primitive* p1, const ContactParams& contactParams) 
			: contactParams(contactParams)
		{
			primitive[0] = p0;
			primitive[1] = p1;
		}
		virtual ~PolyPair() {}

		/*implement*/ virtual PolyPair* allocateClone() = 0;
		/*implement*/ virtual float test() = 0;						
		/*implement*/ virtual void loadConnectors(ConnectorArray& newConnectors) = 0;

		bool match(const PolyPair* other) const {
			return (	(isFaceFace() == other->isFaceFace())
					&&	(primitive[0] == other->primitive[0])	);
		}
	};


	class FaceFacePair : public PolyPair
	{
	private:
		const POLY::Face* mainFace;
		const POLY::Face* otherFace;
		const POLY::Face* nextBestOtherFace;

		const POLY::Face* face(size_t i) {
			return (i == 0) ? mainFace : otherFace;
		}

		const Poly* facePoly() const	{return poly0();}
		const Poly* otherPoly() const	{return poly1();}

		typedef enum {ABOVE_INSIDE, ABOVE_OUTSIDE, BELOW_INSIDE, BELOW_OUTSIDE} VertexStatus;

		//void computeVertices(FixedArray<Vector3, 8>& verticesInObject, const CoordinateFrame& otherInMe);
		void computeVertices(FixedArray<Vector3, CONTACT_ARRAY_SIZE>& verticesInObject, const CoordinateFrame& otherInMe);
		//float closestVertex(const POLY::Face* face, const FixedArray<Vector3, 8>& verticesInObject, const POLY::Vertex* &closestVertex);
		float closestVertex(const POLY::Face* face, const FixedArray<Vector3, CONTACT_ARRAY_SIZE>& verticesInObject, const POLY::Vertex* &closestVertex);

		const POLY::Face* findOtherFace(const POLY::Vertex* closeVertex);

		//bool loadVertices(FixedArray<VertexStatus, 8>* vertexStatus, CoordinateFrame* vertexInFace, ConnectorArray& newConnectors);
		bool loadVertices(FixedArray<VertexStatus, CONTACT_ARRAY_SIZE>* vertexStatus, CoordinateFrame* vertexInFace, ConnectorArray& newConnectors);
		//bool testVerticesInside(size_t faceId, FixedArray<VertexStatus, 8>& vertexStatus, const CoordinateFrame& vertexInFace, ConnectorArray& newConnectors);
		bool testVerticesInside(size_t faceId, FixedArray<VertexStatus, CONTACT_ARRAY_SIZE>& vertexStatus, const CoordinateFrame& vertexInFace, ConnectorArray& newConnectors);
		VertexStatus vertexInPoly(const POLY::Face* planeFace, const POLY::Mesh* planeMesh, const POLY::Vertex* vertex, const CoordinateFrame& otherInMe);

		void vertexInside(
			Primitive* pFace,
			Primitive* pVertex,
			const POLY::Vertex* inside, 
			const POLY::Face* planeFace, 
			ConnectorArray& newConnectors);

		void checkOneSideIntersection(const POLY::Vertex* v0, const POLY::Vertex* v1, const CoordinateFrame& otherInMe, ConnectorArray& newConnectors);
		void validateOneSideIntersection(const POLY::Vertex* belowInside, const POLY::Vertex* belowOutside, const CoordinateFrame& otherInMe, ConnectorArray& newConnectors);
		void checkTwoSideIntersections(const POLY::Vertex* v0, const POLY::Vertex* v1, const CoordinateFrame& otherInMe, ConnectorArray& newConnectors);

		FaceEdgeConnector* newFaceEdgeConnector(size_t mainFaceEdgeId, const POLY::Vertex* v0, const POLY::Vertex* v1);

		/*override*/ bool isFaceFace() const {return true;}
		/*override*/ PolyPair* allocateClone();
		/*override*/ float test();						
		/*override*/ void loadConnectors(ConnectorArray& newConnectors);

	public:
		FaceFacePair(Primitive* p0, Primitive* p1, const ContactParams& contactParams); 
		void setOtherFace(const POLY::Face* aFace) { otherFace = aFace; }
		const POLY::Face* getNextBestOtherFace(void) { return nextBestOtherFace; }
	};

	class EdgeEdgePair : public PolyPair
	{
	private:
		const POLY::Edge* bestEdge0;
		const POLY::Edge* bestEdge1;

		void computeMinMax(const Plane& planeInMesh, const POLY::Mesh* mesh, float& min, float& max);

		EdgeEdgeConnector* newEdgeEdgeConnector();

		/*override*/ bool isFaceFace() const {return false;}
		/*override*/ PolyPair* allocateClone();
		/*override*/ float test();				
		/*override*/ void loadConnectors(ConnectorArray& newConnectors);

	public:
		EdgeEdgePair(Primitive* p0, Primitive* p1, const ContactParams& contactParams);
	};



	class PolyPolyContact 
		: public PolyContact 
		, public Allocator<PolyPolyContact>
	{
	private:
		PolyPair* bestPair;

		void findBestPair();
		void resetBestPair(PolyPair* pairOnStack);

		/*override*/ void findClosestFeatures(ConnectorArray& newConnectors);
	public:
		PolyPolyContact(Primitive* p0, Primitive* p1);
		~PolyPolyContact();

		static float epsilonDistance();	// distance to switch 

        void generateDataForMovingAssemblyStage(void); /*override*/
	};

} // namespace