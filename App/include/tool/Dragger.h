/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once
#include "Util/G3DCore.h"
#include "Util/Extents.h"
#include <vector>
#include <boost/unordered_set.hpp>

namespace RBX {

	class PVInstance;
	class Primitive;
	class ContactManager;
	class PartInstance;

	class Dragger {
	private:
		static void primitivesFromInstances(const std::vector<PVInstance*>& pvInstances,
											G3D::Array<Primitive*>& primitives);

		// Intersection tests
		static bool intersectingWorldOrOthers(	const G3D::Array<Primitive*>& primitives, 
												ContactManager& contactManager,
												const float bottomPlaneHeight);
		
		static void movePrimitives(	const G3D::Array<Primitive*>& primitives, 
									const Vector3& delta,
                                    bool snapToWorld = true);

		static void movePrimitivesDelta(const G3D::Array<Primitive*>& primitives, 
										const Vector3& delta, 
										Vector3& movedSoFar);

		static bool movePrimitivesGoal( const G3D::Array<Primitive*>& primitives,
										const Vector3& goal, 
										Vector3& movedSoFar,
                                        bool snapToWorld = true);

		static void safePlaceAlongLine(	const G3D::Array<Primitive*>& primitives, 
										const Vector3& startMove, 
										const Vector3& endMove,
										Vector3& movedSoFar,
										ContactManager& contactManager,
                                        bool snapToWorld = true);

		static void searchFine(			const G3D::Array<Primitive*> primitives,
										Vector3& movedSoFar,
										ContactManager& contactManager,
										const float bottomPlaneHeight,
										Vector3 insidePosition,
										Vector3 outsidePosition);
				
		static void searchUpGross(		const G3D::Array<Primitive*>& primitives, 
										Vector3& movedSoFar,
										ContactManager& contactManager,
										const float bottomPlaneHeight);

		static void searchDownGross(	const G3D::Array<Primitive*>& primitives, 
										Vector3& movedSoFar,
										ContactManager& contactManager,
										const float bottomPlaneHeight);

		static bool intersectingGroundPlane(const G3D::Array<Primitive*>& primitives,  float yHeight);

		static Vector3 safeMoveYDrop_EXT(	const G3D::Array<Primitive*>& primitives,
											const Vector3& tryMove,
											ContactManager& contactManager,
											const float customPlaneHeight = groundPlaneDepth());

		static void searchUpGross_EXT(	std::vector<RBX::Extents> &primExtents,
										const G3D::Array<Primitive*>& primitives, 
										const boost::unordered_set<const Primitive*> &ignorePrimitives,
										ContactManager& contactManager,
										const float bottomPlaneHeight,
										Vector3& movedSoFar);

		static void searchDownGross_EXT(	std::vector<RBX::Extents> &primExtents,
											const G3D::Array<Primitive*>& primitives, 
											const boost::unordered_set<const Primitive*> &ignorePrimitives,
											ContactManager& contactManager,
											const float bottomPlaneHeight,
											Vector3& movedSoFar);

		static void searchUpFine_EXT(	std::vector<RBX::Extents> &primExtents,
										const G3D::Array<Primitive*>& primitives, 
										const boost::unordered_set<const Primitive*> &ignorePrimitives,
										ContactManager& contactManager,
										const float bottomPlaneHeight,
										Vector3& movedSoFar);

		static void searchDownFine_EXT(	std::vector<RBX::Extents> &primExtents,
										const G3D::Array<Primitive*>& primitives, 
										const boost::unordered_set<const Primitive*> &ignorePrimitives,
										ContactManager& contactManager,
										const float bottomPlaneHeight,
										Vector3& movedSoFar);

		static bool intersectingWorldOrOthers_EXT(	std::vector<RBX::Extents> &primExtents,
													const G3D::Array<Primitive*>& primitives, 
													const boost::unordered_set<const Primitive*> &ignorePrimitives,
													ContactManager& contactManager,
													const float bottomPlaneHeight,  
													const Vector3& movedSoFar);

		static bool intersectingGroundPlane_EXT(	const std::vector<RBX::Extents>& primExtents, 
													const G3D::Array<Primitive*>& primitives, 
													const float yHeight, 
													const Vector3& movedSoFar);

		static bool isIntersecting(const Primitive* prim1, const CoordinateFrame &cFrame1, const Primitive* prim2, const CoordinateFrame &cFrame2);

		static bool checkBallPolyIntersection(const Primitive* ballPrim, const CoordinateFrame &ballCFrame, const Primitive* polyPrim, const CoordinateFrame &polyCFrame);
		static bool checkBallBallIntersection(const Primitive* ballPrim1, const CoordinateFrame &ballCFrame1, const Primitive* ballPrim2, const CoordinateFrame &ballCFrame2);
		static bool checkPolyPolyIntersection(const Primitive* polyPrim1, const CoordinateFrame &polyCFrame1, const Primitive* polyPrim2, const CoordinateFrame &polyCFrame2);

		static void moveExtents(std::vector<RBX::Extents> &primExtents, const Vector3& delta);
		static void moveExtentsDelta(std::vector<RBX::Extents> &primExtents, const Vector3& delta, Vector3& movedSoFar);

	public:
		static const Vector3& dragSnap() {
			static Vector3 v(1.0f, 0.1f, 1.0f);
			return v;
		}
		// Physics automatically removes parts that fall lower than -500.
		// We'll allow dragging, moving, resizing down to -400.
		static float maxDragDepth() { return -400.0f; }
		static float groundPlaneDepth() { return 0.0f; }

		// Moves up as necessary for no overlap
		static Vector3 safeMoveNoDrop(		const G3D::Array<Primitive*>& primitives,
											const Vector3& tryMove,
											ContactManager& contactManager);

		// Floating - move down;  Intersecting - move up
		static Vector3 safeMoveYDrop(		const G3D::Array<Primitive*>& primitives,
											const Vector3& tryMove,
											ContactManager& contactManager,
											const float customPlaneHeight = groundPlaneDepth() );

		// Move along line - quickly find farthest safe move
		static Vector3 safeMoveAlongLine(	const G3D::Array<Primitive*>& primitives,
											const Vector3& tryMove,
											ContactManager& contactManager,
											const float customPlaneHeight = groundPlaneDepth(),
                                            bool snapToWorld = true );

		static Vector3 safeRotateAlongLine(	const G3D::Array<Primitive*>& primitives,
											const Vector3& tryMove,
											ContactManager& contactManager);

		// Rotate around a grid point, then find a safe place
		static void safeRotate(				const G3D::Array<Primitive*>& primitives,
											const Matrix3& rotate,
											ContactManager& contactManager);

		static void safeRotate2(			const G3D::Array<Primitive*>& primitives,
											const Matrix3& rotate,
											ContactManager& contactManager);

		static Extents computeExtents(const std::vector<Primitive*>& primitives);

		static Extents computeExtentsRelative(const std::vector<Primitive*>& primitives, CoordinateFrame& relativeFrame);

		static Extents computeExtentsRelative(const G3D::Array<Primitive*>& primitives, CoordinateFrame& relativeFrame);

		static PartInstance* computePrimaryPart(const std::vector<Primitive*>& primitives);

		static PartInstance* computePrimaryPart(const G3D::Array<Primitive*>& primitives);

		// Intersection tests
		static bool intersectingWorldOrOthers(	PartInstance& partInstance,
												ContactManager& contactManager,
												const float tolerance,
												const float bottomPlaneHeight);

		static bool intersectingWorldOrOthers(	const G3D::Array<Primitive*>& primitives, 
												ContactManager& contactManager,
												const float tolerance,
												const float bottomPlaneHeight);

		// ToDo::Deprecate Array Version
		static Extents computeExtents(const G3D::Array<Primitive*>& primitives);

		static Extents computeExtents(const std::vector<PVInstance*>& instances);


										
	};

} // namespace