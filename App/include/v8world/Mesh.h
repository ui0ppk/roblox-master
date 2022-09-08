#pragma once

#include "Util/G3DCore.h"
#include "rbx/Debug.h"
#include "V8World/GeometryPool.h"

namespace RBX {


	namespace POLY {
		class Edge;
		class Face;

		/* 
			V - E - V
			|   |   |
			E -	F - E
			|   |	| 
			V -	E - V
		*/
		
		class Vertex {
		private:
			size_t id;
			Vector3 offset;
			std::vector<Edge*> edges;

		public:
			Vertex() {}

			Vertex(size_t id, const Vector3& offset) : id(id), offset(offset)
			{}

			const Vector3& getOffset() const {return offset;}

			int getId() const {return id;}

			void addEdge(Edge* value) {
				RBXASSERT(std::find(edges.begin(), edges.end(), value) == edges.end());
				edges.push_back(value);
			}

			Edge* findEdge(const Vertex* other);

			size_t numEdges() const {return edges.size();}
			size_t numFaces() const {return edges.size();}

			Edge* getEdge(size_t i) const {
				return edges[i];
			}

			static const Edge* recoverEdge(const Vertex* v0, const Vertex* v1);

			const Face* getFace(size_t i) const;
		};

		class Edge {
		private:
			size_t id;
			const Vertex* vertex[2];
			const Face* forward;
			const Face* backward;

		public:
			Edge(size_t id, const Vertex* v0, const Vertex* v1)
				: id(id)
				, forward(NULL)
				, backward(NULL)
			{
				vertex[0] = v0;
				vertex[1] = v1;
			}

			const Face* getForward() {return forward;}
			const Face* getBackward() {return backward;}

			const Face* otherFace(const Face* test) const {
				if (test == forward) {
					return backward;
				}
				else {
					RBXASSERT(test == backward);
					return forward;
				}
			}

			bool contains(const Vertex* v) const {
				return ((vertex[0] == v) || (vertex[1] == v));
			}

			void addFace(const Face* face) {
				if (!forward) {
					forward = face;
				}
				else {
					RBXASSERT(face != forward);
					RBXASSERT(!backward);
					backward = face;
				}
			}

			const Vertex* getVertex(const Face* face, size_t id) const {
				if (!face || (face == forward)) {
					RBXASSERT(vertex[id]);
					return vertex[id];
				}
				else {
					RBXASSERT(face == backward);
					RBXASSERT(vertex[(id+1)%2]);
					return vertex[(id+1)%2];
				}
			}

			const Vector3& getVertexOffset(const Face* face, size_t id) const {
				return getVertex(face, id)->getOffset();
			}

			const Face* getVertexFace(const Vertex* v) const {
				if (v == vertex[0]) {
					return forward;
				}
				else {
					RBXASSERT(v == vertex[1]);
					return backward;
				}
			}

			Vector3 computeNormal(const Face* face) const {
				return (getVertexOffset(face, 1) - getVertexOffset(face, 0)).direction();
			}

			Line computeLine() const {
				return Line::fromTwoPoints(vertex[0]->getOffset(), vertex[1]->getOffset());
			}

			size_t getId() const {return id;}

			bool pointInVaronoi(const Vector3& point) const;
		};

		class Face {
		private:
			size_t id;						// id of face
			std::vector<Edge*> edges;		
			Plane outwardPlane;

			bool lineCrossesExtrusionSide(const Vector3& p0, const Vector3& p1, size_t edgeId) const;
			bool lineCrossesExtrusionSideBelow(const Vector3& p0, const Vector3& p1, size_t edgeId) const;

			bool pointInExtrusionSide(const Vector3& pointOnSide, const Plane& sidePlane, size_t edgeId) const;

			bool pointInInternalExtrusion(const Vector3& point) const {
				return ((plane().distance(point) <= 0.0f)  &&	pointInExtrusion(point));
			}

		public:
			Face(size_t id, Edge* e0, Edge* e1, Edge* e2);
			Face(size_t id, Edge* e0, Edge* e1, Edge* e2, Edge* e3);
			Face( size_t id, std::vector<Edge*>& edgeList );
			void initPlane();

			const Vertex* getVertex(int id) const {
				return edges[id]->getVertex(this, 0);
			}
			
			const Vector3& getVertexOffset(int id) const {
				return getVertex(id)->getOffset();
			}

			const Vector3& normal() const {
				return outwardPlane.normal();
			}

			size_t numEdges() const {return edges.size();}
			size_t numVertices() const {return edges.size();}

			Edge* getEdge(size_t i) const {
				return edges[i];
			}

			bool pointInExtrusion(const Vector3& point) const {
				Vector3 pointOnPlane = plane().closestPoint(point);
				return pointInFaceBorders(pointOnPlane);
			}

			bool pointInFaceBorders(const Vector3& point) const;			// point must be on face plane

			const Plane& plane() const {
				RBXASSERT(edges.size() >= 3);
				//RBXASSERT(!(outwardPlane == Plane()));
				return outwardPlane;
			}

			const Plane getSidePlane(size_t edgeId) const {
				Edge* edge = getEdge(edgeId);
				Vector3 sideVector = edge->computeNormal(this).cross(plane().normal());
				return Plane(sideVector, edge->getVertexOffset(this, 0));
			}

			int getInternalExtrusionIntersection(const Vector3& pBelowInside, const Vector3& pBelowOutside) const;
			int findInternalExtrusionIntersection(const Vector3& p0, const Vector3& p1) const;
			void findInternalExtrusionIntersections(const Vector3& p0, const Vector3& p1, int& side0, int& side1) const;

			size_t getId() const {return id;}

			Vector3 getCentroid( void ) const;
			void getOrientedBoundingBox( const Vector3& xDir, const Vector3& yDir, Vector3& boxMin, Vector3& boxMax, Vector3& boxCenter ) const;

		};

		class Mesh {
		private:
			std::vector<Vertex> vertices;
			std::vector<Edge> edges;			
			std::vector<Face> faces;	

			void clear();

			void addVertex(float x, float y, float z);
			void addFace(size_t i, size_t j, size_t k);
			void addFace(size_t i, size_t j, size_t k, size_t l);
			void addFace( int numVerts, int vertIndexList[], bool reverseOrder );

			Edge* findOrMakeEdge(size_t v0, size_t v1);
			Edge* addEdge(Vertex* vert0, Vertex* vert1);

			bool lineIntersectsFace(const Line& line, const Face* face) const;
			bool rayIntersectsFace(const RbxRay& ray, const Face* face, Vector3& intersection) const;

		public:
			Mesh() {}

			size_t numFaces() const {return faces.size();}
			const POLY::Face* getFace(int i) const {return &faces[i];}

			size_t numVertices() const {return vertices.size();}
			const POLY::Vertex* getVertex(int i) const {return &vertices[i];}

			size_t numEdges() const {return edges.size();}
			const POLY::Edge* getEdge(int i) const {return &edges[i];}

			bool containsFace(const Face* face) const {
				for (size_t i = 0; i < numFaces(); ++i) {
					if (face == getFace(i)) {
						return true;
					}
				}
				return false;
			}

			const POLY::Face* findFace(size_t i0, size_t i1, size_t i2);

			const Vertex* farthestVertex(const Vector3& direction) const;

			bool pointInMesh(const Vector3& point) const;

			const Face* findFaceIntersection(const Vector3& inside, const Vector3& outside) const;

			void findFaceIntersections(const Vector3& p0, const Vector3& p1, const Face* &f0, const Face* &f1) const;

			bool hitTest(const RbxRay& ray, Vector3& hitPoint, Vector3& surfaceNormal) const;

			void makeWedge(const Vector3& size);

			void makePrism(const Vector3_2Ints& params, Vector3& cofm);
			void makePyramid(const Vector3_2Ints& params, Vector3& cofm);
			void makeParallelRamp(const Vector3& size, Vector3& cofm);
			void makeRightAngleRamp(const Vector3& size, Vector3& cofm);
			void makeCornerWedge(const Vector3& size, Vector3& cofm);


			void makeBlock(const Vector3& size);
            void makeCell(const Vector3& size, const Vector3& offset);
            void makeVerticalWedgeCell(const Vector3& size, const Vector3& offset, const int& orient);
            void makeHorizontalWedgeCell(const Vector3& size, const Vector3& offset, const int& orient);
            void makeCornerWedgeCell(const Vector3& size, const Vector3& offset, const int& orient);
            void makeInverseCornerWedgeCell(const Vector3& size, const Vector3& offset, const int& orient);
		};
	}	// namespace POLY
} // namespace
