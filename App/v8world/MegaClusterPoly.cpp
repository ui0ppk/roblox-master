#include "stdafx.h"

#include "V8World/MegaClusterPoly.h"
#include "Util/Math.h"
#include "FastLog.h"
#include "V8DataModel/MegaCluster.h"
#include "V8World/BulletGeometryPoolObjects.h"
#include "V8World/TerrainPartition.h"
#include "Voxel/Grid.h"
#include "Voxel/AreaCopy.h"

#include <boost/math/special_functions/fpclassify.hpp>

#include "BulletCollision/NarrowPhaseCollision/btRaycastCallback.h"


namespace RBX {
using namespace Voxel;

MegaClusterPoly::MegaClusterPoly(Primitive* p)
    : myPrim(p)
{
	Grid* grid = rbx_static_cast<MegaClusterInstance*>(myPrim->getOwner())->getVoxelGrid();

	myTerrainPartition.reset(new TerrainPartitionMega(grid));

	for (unsigned int i = 0; i < CELL_BLOCK_Empty; i++)
		bulletCellShapes.push_back(new btConvexHullShape());

	createBulletCellShapes();
}

MegaClusterPoly::~MegaClusterPoly(void)
{
	for(unsigned int i = 0; i < bulletCellShapes.size(); i++)
		delete bulletCellShapes[i];

	bulletCellShapes.clear();
}

void MegaClusterPoly::buildMesh()
{
	Vector3 key = getSize();    

	aMegaClusterMesh =	MegaClusterMeshPool::getToken(key);

	mesh = 	aMegaClusterMesh->getMesh();
}

bool MegaClusterPoly::hitTest(const RbxRay& rayInMe, Vector3& localHitPoint, Vector3& surfaceNormal, float searchRayMax, bool treatCellsAsBlocks, bool ignoreWater)
{
    int dummySurf = -1;
    CoordinateFrame surfaceCf;
    return hitTestMC(rayInMe, localHitPoint, surfaceNormal, dummySurf, surfaceCf, searchRayMax, treatCellsAsBlocks, ignoreWater);
}

bool MegaClusterPoly::hitTestTerrain(const RbxRay& rayInMe, Vector3& localHitPoint, int& surfId, CoordinateFrame& surfCf)
{
	Vector3 unusedSurfaceNormal;

	return hitTestMC(rayInMe, localHitPoint, unusedSurfaceNormal, surfId, surfCf);
}

const Grid::Region getRegionForCellLocation(const Grid* store,
		const Vector3int16& location, Grid::Region* previousRegion=NULL) {
	if (previousRegion && previousRegion->contains(location)) {
		return *previousRegion;
	} else {
		Region3int16 extents = SpatialRegion::inclusiveVoxelExtentsOfRegion(
				SpatialRegion::regionContainingVoxel(location));
		return store->getRegion(extents.getMinPos(), extents.getMaxPos());
	}
}

bool MegaClusterPoly::hitTestMC(const RbxRay& rayInMe, Vector3& localHitPoint, Vector3& surfaceNormal, int& surfId, CoordinateFrame& surfaceCf, float searchRayMax, bool treatCellsAsBlocks, bool ignoreWater)
{
	// we assume rayInMe is a unit ray, which means when time passed > searchRayMax, then the distance the ray has traveled will also be > searchRayMax
	if (searchRayMax > MC_SEARCH_RAY_MAX) searchRayMax = MC_SEARCH_RAY_MAX;

	for (int index = 0; index < 3; ++index)
    {
		// DE2702 fix: degenerate ray origins overflow our time-step calculations and cause infinite loops; just early-exit if values are unreasonable
		RBXASSERT(boost::math::isfinite(rayInMe.origin()[index]));
		RBXASSERT(boost::math::isfinite(rayInMe.direction()[index]));

        if ( fabs(rayInMe.origin()[index]) > MC_HUGE_VAL ) {
            surfId = -1;
            return false;
        }
	}

	Grid* grid = rbx_static_cast<MegaClusterInstance*>(myPrim->getOwner())->getVoxelGrid();

	bool doneSearching = false;
	bool foundIntersection = false;

	Vector3 startPos = rayInMe.origin();

		surfId = -1;

		float timeStep[3];
		short direction[3];  //keeps track of whether ray is moving in positive or negative direction along each axis
		float nextTime[3];

		Cell gridTest;
		Vector3int16 testCell;
		float timePassedSoFar;
		testCell = worldToCell_floor(startPos);
		Grid::Region region = getRegionForCellLocation(grid, testCell);
		bool startedInWater = region.hasWaterAt(testCell);

		int axisToStepIn;

		// initialization loop: determine the starting cell in our grid, the time step sizes we will take along each axis, the directions we will step in, 
		//    and the time to first intersection with the nearest grid x-plane, y-plane, and z-plane
		for (int index = 0; index < 3; ++index)
		{
			// DE3399 fix: -1.#IND and NaN will fail equivalence check with self
			if (startPos[index] != startPos[index]) {
				surfId = -1;
				return false;
			}
			// find how long it is between x-plane, y-plane, and z-plane intersections; catch division by zero cases
			if (rayInMe.direction()[index] < MC_RAY_ZERO_SLOPE_TOLERANCE && rayInMe.direction()[index] > -MC_RAY_ZERO_SLOPE_TOLERANCE)
				timeStep[index] = MC_HUGE_VAL;
			else
				timeStep[index] = kCELL_SIZE / std::abs(rayInMe.direction()[index]);

			// find our initial times to next x-plane, y-plane, and z-plane intersections
			if (rayInMe.direction()[index] > 0)
			{
				nextTime[index] = (cellToWorld_smallestCorner(testCell)[index] + kCELL_SIZE - startPos[index]) * timeStep[index] / kCELL_SIZE; 
				direction[index] = 1;
			}
			else
			{
				nextTime[index] = (startPos[index] - cellToWorld_smallestCorner(testCell)[index]) * timeStep[index] / kCELL_SIZE; 
				direction[index] = -1;
			}
		}

		// two dummy values for finding target surface values of non-blocks as if they were blocks (without changing our actual Cf and hitPoint)
		Vector3 dummyHitPoint;
        Vector3 dummySurfaceNormal;
		CoordinateFrame dummyCf;

		while (!doneSearching){
			gridTest = region.voxelAt(testCell);

			if( gridTest.solid.getBlock() != CELL_BLOCK_Empty)
			{
				CellOrientation orientation = gridTest.solid.getOrientation();
				CellBlock type;
				if (treatCellsAsBlocks)
					type = (CellBlock)0;
				else
					type = gridTest.solid.getBlock();

				switch( type )
				{
					case CELL_BLOCK_Solid:
					default:
						foundIntersection = doneSearching = hitLocationOnBlockCell(rayInMe, testCell, localHitPoint, surfaceNormal, surfId, surfaceCf);
						break;
					case CELL_BLOCK_VerticalWedge:
						foundIntersection = doneSearching = hitLocationOnVerticalWedgeCell(rayInMe, testCell, orientation, localHitPoint, surfaceNormal, surfaceCf);
						if (doneSearching) hitLocationOnBlockCell(rayInMe, testCell, dummyHitPoint, dummySurfaceNormal, surfId, dummyCf); // we get the surfaceId of the last cube hit
						break;
					case CELL_BLOCK_HorizontalWedge:
						foundIntersection = doneSearching = hitLocationOnHorizontalWedgeCell(rayInMe, testCell, orientation, localHitPoint, surfaceNormal, surfaceCf);
						if (doneSearching) hitLocationOnBlockCell(rayInMe, testCell, dummyHitPoint, dummySurfaceNormal, surfId, dummyCf); // we get the surfaceId of the last cube hit
						break;
					case CELL_BLOCK_CornerWedge:
						foundIntersection = doneSearching = hitLocationOnCornerWedgeCell(rayInMe, testCell, orientation, localHitPoint, surfaceNormal, surfaceCf);
						if (doneSearching) hitLocationOnBlockCell(rayInMe, testCell, dummyHitPoint, dummySurfaceNormal, surfId, dummyCf); // we get the surfaceId of the last cube hit
						break;
					case CELL_BLOCK_InverseCornerWedge:
						foundIntersection = doneSearching = hitLocationOnInverseCornerWedgeCell(rayInMe, testCell, orientation, localHitPoint, surfaceNormal, surfaceCf);
						if (doneSearching) hitLocationOnBlockCell(rayInMe, testCell, dummyHitPoint, dummySurfaceNormal, surfId, dummyCf); // we get the surfaceId of the last cube hit
						break;
				}
			}

			if (!ignoreWater) {
				bool cellHasWater = region.hasWaterAt(testCell);
				bool waterHit = startedInWater != cellHasWater;
				if (!foundIntersection && waterHit) {
					foundIntersection = doneSearching = hitLocationOnBlockCell(rayInMe, testCell, localHitPoint, surfaceNormal, surfId, surfaceCf);
				}
			}
			
			// advance our ray by 1 block in correct direction (whichever nextTime is smallest is the plane we cross next)
			if (nextTime[0] < nextTime[1])
			{
				if (nextTime[0] < nextTime[2]) axisToStepIn = 0;
				else axisToStepIn = 2;
			}
			else
			{
				if (nextTime[1] < nextTime[2]) axisToStepIn = 1;
				else axisToStepIn = 2;
			}

			timePassedSoFar = nextTime[axisToStepIn]; 
			nextTime[axisToStepIn] += timeStep[axisToStepIn]; 
			testCell[axisToStepIn] += direction[axisToStepIn];

			// see if our ray has expired
			if ( timePassedSoFar > searchRayMax )
				doneSearching = true;

			// see if we're done with this box
			region = getRegionForCellLocation(grid, testCell, &region);
		}

	return foundIntersection;
}

CoordinateFrame MegaClusterPoly::getSurfaceCoordInBody( const size_t surfaceId ) const
{
	CoordinateFrame aCS;
	return aCS;
}

size_t MegaClusterPoly::getFaceFromLegacyNormalId( const NormalId nId ) const
{
	return -1;
}

bool MegaClusterPoly::findTouchingSurfacesConvex( const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId ) const
{
	RBXASSERT(false); // not implemented
    return false;
}

struct FindPlanarTouchesWithGeomPredicate
{
    const MegaClusterPoly* poly;
    const CoordinateFrame* myCf;
    const Geometry* otherGeom;
    const CoordinateFrame* otherCf;

    bool operator()(const Vector3int16& cell) const
    {
        return !poly->hasPlanarTouchWithGeom(cell, *myCf, *otherGeom, *otherCf);
    }
};

bool MegaClusterPoly::findPlanarTouchesWithGeom( const CoordinateFrame& myCf, const Geometry& otherGeom, const CoordinateFrame& otherCf, std::vector<Vector3int16>* cells ) const
{
    findCellsTouchingGeometryWithBuffer(0.2, myCf, otherGeom, otherCf, cells);

    if( cells->size() > 0 )
    {
        FindPlanarTouchesWithGeomPredicate pred = {this, &myCf, &otherGeom, &otherCf };

        cells->erase(std::remove_if(cells->begin(), cells->end(), pred), cells->end());
    }

    return cells->size() > 0;
}

bool MegaClusterPoly::hasPlanarTouchWithGeom( const Vector3int16& cell, const CoordinateFrame& myCf, const Geometry& otherGeom, const CoordinateFrame& otherCf ) const
{
	size_t tempFaceId;
    return findCellIntersectionWithGeom(cell, myCf, otherGeom, otherCf, tempFaceId).size() > 2;
}

std::vector<Vector3> MegaClusterPoly::findCellIntersectionWithGeom( const Vector3int16& cell, const CoordinateFrame& myCf, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId ) const
{
    std::vector<Vector3> result;

	// If the otherGeom is a ball, do not compute intersection
	// TODO: If otherGeom is CSG...
    if( otherGeom.getGeometryType() == Geometry::GEOMETRY_BALL ||
        otherGeom.getGeometryType() == Geometry::GEOMETRY_CYLINDER ||
		otherGeom.getGeometryType() == Geometry::GEOMETRY_TRI_MESH )
        return result;

    const Poly* otherPoly = rbx_static_cast<const Poly*>(&otherGeom);

    Vector3 size(kCELL_SIZE, kCELL_SIZE, kCELL_SIZE);
    POLY::Mesh cellMesh;
    Vector3 cellOffset = kCELL_SIZE * Vector3(cell) + Vector3(Voxel::kHALF_CELL, Voxel::kHALF_CELL, Voxel::kHALF_CELL);

	Grid* grid = rbx_static_cast<MegaClusterInstance*>(myPrim->getOwner())->getVoxelGrid();
    Cell cellData = grid->getCell(cell);

    CellOrientation orientation = cellData.solid.getOrientation();
    CellBlock type = cellData.solid.getBlock();

    switch( type )
    {
        case CELL_BLOCK_Solid:
        default:
            cellMesh.makeCell(size, cellOffset);
            break;
        case CELL_BLOCK_VerticalWedge:
            cellMesh.makeVerticalWedgeCell(size, cellOffset, (int)orientation);
            break;
        case CELL_BLOCK_HorizontalWedge:
            cellMesh.makeHorizontalWedgeCell(size, cellOffset, (int)orientation);
            break;
        case CELL_BLOCK_CornerWedge:
            cellMesh.makeCornerWedgeCell(size, cellOffset, (int)orientation);
            break;
        case CELL_BLOCK_InverseCornerWedge:
            cellMesh.makeInverseCornerWedgeCell(size, cellOffset, (int)orientation);
            break;
    }

    for( unsigned int i = 0; i < otherPoly->getMesh()->numFaces(); i++ )
    {
        std::vector<Vector3> otherPolygon;
        for( unsigned int j = 0; j < otherPoly->getMesh()->getFace(i)->numVertices(); j++ )
        {
            Vector3 otherVertInOther = otherPoly->getMesh()->getFace(i)->getVertexOffset(j);
            Vector3 otherVertInWorld = otherCf.pointToWorldSpace(otherVertInOther);
            otherPolygon.push_back(myCf.pointToObjectSpace(otherVertInWorld));
        }

        for( unsigned int ii = 0; ii < cellMesh.numFaces(); ii++ )
        {
            std::vector<Vector3> cellPolygon;
            for( unsigned int jj = 0; jj < cellMesh.getFace(ii)->numVertices(); jj++ )
                cellPolygon.push_back(cellMesh.getFace(ii)->getVertexOffset(jj));
            
            result = Math::spatialPolygonIntersection(cellPolygon, otherPolygon);
			if( result.size() > 2 ) // valid planar intersection found - do an immediate return of results
			{ 
				otherFaceId = i;
                return result;
			}
        }
    }

    return result;
}

bool MegaClusterPoly::hitLocationOnCornerWedgeCell(const RbxRay& rayInMe, const Vector3int16& testCell, const int& orientation, Vector3& localHitPoint, Vector3& surfaceNormal, CoordinateFrame& hitSurfaceCoord) const
{
    // Depending on orientation, specify faces and check for intersection
    Vector3 t, b0, b1, b2;

    // Rotate depending on orientation
   switch( orientation )
   {
        case 0:
        default:
            t = Vector3(1, 1, -1);
            b0 = Vector3(1, -1, 1);
            b1 = Vector3(1, -1, -1);
            b2 = Vector3(-1, -1, -1);
            break;
        case 1:
            t = Vector3(-1, 1, -1);
            b0 = Vector3(1, -1, -1);
            b1 = Vector3(-1, -1, -1);
            b2 = Vector3(-1, -1, 1);
            break;
        case 2:
            t = Vector3(-1, 1, 1);
            b0 = Vector3(-1, -1, -1);
            b1 = Vector3(-1, -1, 1);
            b2 = Vector3(1, -1, 1);
            break;
        case 3:
            t = Vector3(1, 1, 1);
            b0 = Vector3(-1, -1, 1);
            b1 = Vector3(1, -1, 1);
            b2 = Vector3(1, -1, -1);
            break;
   }

    // check faces of the cell
    Vector3 cellCenterLocal = cellToWorld_smallestCorner(testCell) + Vector3(Voxel::kHALF_CELL, Voxel::kHALF_CELL, Voxel::kHALF_CELL);

    // bottom face (looking at positive z, with y pointing up prior to applying rotation)
    std::vector<Vector3> poly;
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * b0);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * b2);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * b1);
    if( Math::intersectRayConvexPolygon(rayInMe, poly, localHitPoint, true) )
    {
        surfaceNormal = (poly[1]-poly[0]).cross(poly[2]-poly[0]).unit();
        hitSurfaceCoord.rotation = Math::getWellFormedRotForZVector(surfaceNormal);
        hitSurfaceCoord.translation = poly[1];
        return true;
    }
    poly.clear();

    // right face
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * b0);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * b1);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * t);
    if( Math::intersectRayConvexPolygon(rayInMe, poly, localHitPoint, true) )
    {
        surfaceNormal = (poly[1]-poly[0]).cross(poly[2]-poly[0]).unit();
        hitSurfaceCoord.rotation = Math::getWellFormedRotForZVector(surfaceNormal);
        hitSurfaceCoord.translation = poly[1];
        return true;
    }
    poly.clear();

    // back face
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * b1);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * b2);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * t);
    if( Math::intersectRayConvexPolygon(rayInMe, poly, localHitPoint, true) )
    {
        surfaceNormal = (poly[1]-poly[0]).cross(poly[2]-poly[0]).unit();
        hitSurfaceCoord.rotation = Math::getWellFormedRotForZVector(surfaceNormal);
        hitSurfaceCoord.translation = poly[1];
        return true;
    }
    poly.clear();

    // inclined face
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * b0);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * t);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * b2);
    if( Math::intersectRayConvexPolygon(rayInMe, poly, localHitPoint, true) )
    {
        surfaceNormal = (poly[1]-poly[0]).cross(poly[2]-poly[0]).unit();
        hitSurfaceCoord.rotation = Math::getWellFormedRotForZVector(surfaceNormal);
        hitSurfaceCoord.translation = poly[1];
        return true;
    }

    return false;
}

bool MegaClusterPoly::hitLocationOnHorizontalWedgeCell(const RbxRay& rayInMe, const Vector3int16& testCell, const int& orientation, Vector3& localHitPoint, Vector3& surfaceNormal, CoordinateFrame& hitSurfaceCoord) const
{
    // Depending on orientation, specify faces and check for intersection
    Vector3 p0, p1, p2, p3, p4, p5;

    // Rotate depending on orientation
   switch( orientation )
   {
        case 0:
        default:
            p0 = Vector3(1, -1, 1);
            p1 = Vector3(1, -1, -1);
            p2 = Vector3(-1, -1, -1);
            p3 = Vector3(1, 1, 1);
            p4 = Vector3(1, 1, -1);
            p5 = Vector3(-1, 1, -1);
            break;
        case 1:
            p0 = Vector3(1, -1, -1);
            p1 = Vector3(-1, -1, -1);
            p2 = Vector3(-1, -1, 1);
            p3 = Vector3(1, 1, -1);
            p4 = Vector3(-1, 1, -1);
            p5 = Vector3(-1, 1, 1);
            break;
        case 2:
            p0 = Vector3(-1, -1, -1);
            p1 = Vector3(-1, -1, 1);
            p2 = Vector3(1, -1, 1);
            p3 = Vector3(-1, 1, -1);
            p4 = Vector3(-1, 1, 1);
            p5 = Vector3(1, 1, 1);
            break;
        case 3:
            p0 = Vector3(-1, -1, 1);
            p1 = Vector3(1, -1, 1);
            p2 = Vector3(1, -1, -1);
            p3 = Vector3(-1, 1, 1);
            p4 = Vector3(1, 1, 1);
            p5 = Vector3(1, 1, -1);
           break;
   }

    // check faces of the cell
    Vector3 cellCenterLocal = cellToWorld_smallestCorner(testCell) + Vector3(Voxel::kHALF_CELL, Voxel::kHALF_CELL, Voxel::kHALF_CELL);

    // bottom face (looking at positive z, with y pointing up prior to applying rotation)
    std::vector<Vector3> poly;
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p2);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p1);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p0);
    if( Math::intersectRayConvexPolygon(rayInMe, poly, localHitPoint, true) )
    {
        surfaceNormal = (poly[1]-poly[0]).cross(poly[2]-poly[0]).unit();
        hitSurfaceCoord.rotation = Math::getWellFormedRotForZVector(surfaceNormal);
        hitSurfaceCoord.translation = poly[1];
        return true;
    }
    poly.clear();

    // right face
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p0);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p1);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p4);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p3);
    if( Math::intersectRayConvexPolygon(rayInMe, poly, localHitPoint, true) )
    {
        surfaceNormal = (poly[1]-poly[0]).cross(poly[2]-poly[0]).unit();
        hitSurfaceCoord.rotation = Math::getWellFormedRotForZVector(surfaceNormal);
        hitSurfaceCoord.translation = poly[1];
        return true;
    }
    poly.clear();

    // back face
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p1);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p2);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p5);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p4);
    if( Math::intersectRayConvexPolygon(rayInMe, poly, localHitPoint, true) )
    {
        surfaceNormal = (poly[1]-poly[0]).cross(poly[2]-poly[0]).unit();
        hitSurfaceCoord.rotation = Math::getWellFormedRotForZVector(surfaceNormal);
        hitSurfaceCoord.translation = poly[1];
        return true;
    }
    poly.clear();

    // top face
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p3);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p4);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p5);
    if( Math::intersectRayConvexPolygon(rayInMe, poly, localHitPoint, true) )
    {
        surfaceNormal = (poly[1]-poly[0]).cross(poly[2]-poly[0]).unit();
        hitSurfaceCoord.rotation = Math::getWellFormedRotForZVector(surfaceNormal);
        hitSurfaceCoord.translation = poly[1];
        return true;
    }
    poly.clear();

    // inclined face
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p0);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p3);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p5);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p2);
    if( Math::intersectRayConvexPolygon(rayInMe, poly, localHitPoint, true) )
    {
        surfaceNormal = (poly[1]-poly[0]).cross(poly[2]-poly[0]).unit();
        hitSurfaceCoord.rotation = Math::getWellFormedRotForZVector(surfaceNormal);
        hitSurfaceCoord.translation = poly[1];
        return true;
    }

    return false;
}

bool MegaClusterPoly::hitLocationOnVerticalWedgeCell(const RbxRay& rayInMe, const Vector3int16& testCell, const int& orientation, Vector3& localHitPoint, Vector3& surfaceNormal, CoordinateFrame& hitSurfaceCoord) const
{
    // Depending on orientation, specify faces and check for intersection
    Vector3 t0, t1, b0, b1, b2, b3;

    // Rotate depending on orientation
   switch( orientation )
   {
        case 0:
        default:
            t0 = Vector3(1, 1, -1);
            t1 = Vector3(-1, 1, -1);
            b0 = Vector3(1, -1, 1);
            b1 = Vector3(1, -1, -1);
            b2 = Vector3(-1, -1, -1);
            b3 = Vector3(-1, -1, 1);
            break;
        case 1:
            t0 = Vector3(-1, 1, -1);
            t1 = Vector3(-1, 1, 1);
            b0 = Vector3(1, -1, -1);
            b1 = Vector3(-1, -1, -1);
            b2 = Vector3(-1, -1, 1);
            b3 = Vector3(1, -1, 1);
            break;
        case 2:
            t0 = Vector3(-1, 1, 1);
            t1 = Vector3(1, 1, 1);
            b0 = Vector3(-1, -1, -1);
            b1 = Vector3(-1, -1, 1);
            b2 = Vector3(1, -1, 1);
            b3 = Vector3(1, -1, -1);
            break;
        case 3:
            t0 = Vector3(1, 1, 1);
            t1 = Vector3(1, 1, -1);
            b0 = Vector3(-1, -1, 1);
            b1 = Vector3(1, -1, 1);
            b2 = Vector3(1, -1, -1);
            b3 = Vector3(-1, -1, -1);
            break;
   }

    // check faces of the cell
    Vector3 cellCenterLocal = cellToWorld_smallestCorner(testCell) + Vector3(Voxel::kHALF_CELL, Voxel::kHALF_CELL, Voxel::kHALF_CELL);

    // bottom face (looking at positive z, with y pointing up prior to applying rotation)
    std::vector<Vector3> poly;
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * b0);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * b3);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * b2);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * b1);
    if( Math::intersectRayConvexPolygon(rayInMe, poly, localHitPoint, true) )
    {
        surfaceNormal = (poly[1]-poly[0]).cross(poly[2]-poly[0]).unit();
        hitSurfaceCoord.rotation = Math::getWellFormedRotForZVector(surfaceNormal);
        hitSurfaceCoord.translation = poly[1];
        return true;
    }
    poly.clear();

    // right face
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * b0);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * b1);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * t0);
    if( Math::intersectRayConvexPolygon(rayInMe, poly, localHitPoint, true) )
    {
        surfaceNormal = (poly[1]-poly[0]).cross(poly[2]-poly[0]).unit();
        hitSurfaceCoord.rotation = Math::getWellFormedRotForZVector(surfaceNormal);
        hitSurfaceCoord.translation = poly[1];
        return true;
    }
    poly.clear();

    // back face
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * b1);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * b2);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * t1);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * t0);
    if( Math::intersectRayConvexPolygon(rayInMe, poly, localHitPoint, true) )
    {
        surfaceNormal = (poly[1]-poly[0]).cross(poly[2]-poly[0]).unit();
        hitSurfaceCoord.rotation = Math::getWellFormedRotForZVector(surfaceNormal);
        hitSurfaceCoord.translation = poly[1];
        return true;
    }
    poly.clear();

    // left face
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * b2);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * b3);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * t1);
    if( Math::intersectRayConvexPolygon(rayInMe, poly, localHitPoint, true) )
    {
        surfaceNormal = (poly[1]-poly[0]).cross(poly[2]-poly[0]).unit();
        hitSurfaceCoord.rotation = Math::getWellFormedRotForZVector(surfaceNormal);
        hitSurfaceCoord.translation = poly[1];
        return true;
    }
    poly.clear();

    // inclined face
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * b3);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * b0);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * t0);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * t1);
    if( Math::intersectRayConvexPolygon(rayInMe, poly, localHitPoint, true) )
    {
        surfaceNormal = (poly[1]-poly[0]).cross(poly[2]-poly[0]).unit();
        hitSurfaceCoord.rotation = Math::getWellFormedRotForZVector(surfaceNormal);
        hitSurfaceCoord.translation = poly[1];
        return true;
    }

    return false;
}

bool MegaClusterPoly::hitLocationOnInverseCornerWedgeCell(const RbxRay& rayInMe, const Vector3int16& testCell, const int& orientation, Vector3& localHitPoint, Vector3& surfaceNormal, CoordinateFrame& hitSurfaceCoord) const
{
    // Depending on orientation, specify faces and check for intersection
    Vector3 p0, p1, p2, p3, p4, p5, p6;

    // Rotate depending on orientation
   switch( orientation )
   {
        case 0:
        default:
            p0 = Vector3(1, -1, 1);
            p1 = Vector3(1, -1, -1);
            p2 = Vector3(-1, -1, -1);
            p3 = Vector3(-1, -1, 1);
            p4 = Vector3(1, 1, 1);
            p5 = Vector3(1, 1, -1);
            p6 = Vector3(-1, 1, -1);
            break;
        case 1:
            p0 = Vector3(1, -1, -1);
            p1 = Vector3(-1, -1, -1);
            p2 = Vector3(-1, -1, 1);
            p3 = Vector3(1, -1, 1);
            p4 = Vector3(1, 1, -1);
            p5 = Vector3(-1, 1, -1);
            p6 = Vector3(-1, 1, 1);
            break;
        case 2:
            p0 = Vector3(-1, -1, -1);
            p1 = Vector3(-1, -1, 1);
            p2 = Vector3(1, -1, 1);
            p3 = Vector3(1, -1, -1);
            p4 = Vector3(-1, 1, -1);
            p5 = Vector3(-1, 1, 1);
            p6 = Vector3(1, 1, 1);
            break;
        case 3:
            p0 = Vector3(-1, -1, 1);
            p1 = Vector3(1, -1, 1);
            p2 = Vector3(1, -1, -1);
            p3 = Vector3(-1, -1, -1);
            p4 = Vector3(-1, 1, 1);
            p5 = Vector3(1, 1, 1);
            p6 = Vector3(1, 1, -1);
            break;
   }

    // check faces of the cell
    Vector3 cellCenterLocal = cellToWorld_smallestCorner(testCell) + Vector3(Voxel::kHALF_CELL, Voxel::kHALF_CELL, Voxel::kHALF_CELL);

    // bottom face (looking at positive z, with y pointing up prior to applying rotation)
    std::vector<Vector3> poly;
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p0);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p3);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p2);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p1);
    if( Math::intersectRayConvexPolygon(rayInMe, poly, localHitPoint, true) )
    {
        surfaceNormal = (poly[1]-poly[0]).cross(poly[2]-poly[0]).unit();
        hitSurfaceCoord.rotation = Math::getWellFormedRotForZVector(surfaceNormal);
        hitSurfaceCoord.translation = poly[1];
        return true;
    }
    poly.clear();

    // right face
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p0);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p1);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p5);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p4);
    if( Math::intersectRayConvexPolygon(rayInMe, poly, localHitPoint, true) )
    {
        surfaceNormal = (poly[1]-poly[0]).cross(poly[2]-poly[0]).unit();
        hitSurfaceCoord.rotation = Math::getWellFormedRotForZVector(surfaceNormal);
        hitSurfaceCoord.translation = poly[1];
        return true;
    }
    poly.clear();

    // back face
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p1);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p2);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p6);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p5);
    if( Math::intersectRayConvexPolygon(rayInMe, poly, localHitPoint, true) )
    {
        surfaceNormal = (poly[1]-poly[0]).cross(poly[2]-poly[0]).unit();
        hitSurfaceCoord.rotation = Math::getWellFormedRotForZVector(surfaceNormal);
        hitSurfaceCoord.translation = poly[1];
        return true;
    }
    poly.clear();

    // left face
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p6);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p2);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p3);
    if( Math::intersectRayConvexPolygon(rayInMe, poly, localHitPoint, true) )
    {
        surfaceNormal = (poly[1]-poly[0]).cross(poly[2]-poly[0]).unit();
        hitSurfaceCoord.rotation = Math::getWellFormedRotForZVector(surfaceNormal);
        hitSurfaceCoord.translation = poly[1];
        return true;
    }
    poly.clear();

    // front face
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p4);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p3);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p0);
    if( Math::intersectRayConvexPolygon(rayInMe, poly, localHitPoint, true) )
    {
        surfaceNormal = (poly[1]-poly[0]).cross(poly[2]-poly[0]).unit();
        hitSurfaceCoord.rotation = Math::getWellFormedRotForZVector(surfaceNormal);
        hitSurfaceCoord.translation = poly[1];
        return true;
    }
    poly.clear();

    // top face
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p4);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p5);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p6);
    if( Math::intersectRayConvexPolygon(rayInMe, poly, localHitPoint, true) )
    {
        surfaceNormal = (poly[1]-poly[0]).cross(poly[2]-poly[0]).unit();
        hitSurfaceCoord.rotation = Math::getWellFormedRotForZVector(surfaceNormal);
        hitSurfaceCoord.translation = poly[1];
        return true;
    }
    poly.clear();

    // inclined face
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p3);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p4);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * p6);
    if( Math::intersectRayConvexPolygon(rayInMe, poly, localHitPoint, true) )
    {
        surfaceNormal = (poly[1]-poly[0]).cross(poly[2]-poly[0]).unit();
        hitSurfaceCoord.rotation = Math::getWellFormedRotForZVector(surfaceNormal);
        hitSurfaceCoord.translation = poly[1];
        return true;
    }

    return false;
}

bool MegaClusterPoly::hitLocationOnBlockCell(const RbxRay& rayInMe, const Vector3int16& testCell, Vector3& localHitPoint, Vector3& surfaceNormal, int& surfId, CoordinateFrame& hitSurfaceCoord) const
{
    Vector3 t0(1, 1, 1);
    Vector3 t1(1, 1, -1);
    Vector3 t2(-1, 1, -1);
    Vector3 t3(-1, 1, 1);
    Vector3 b0(1, -1, 1);
    Vector3 b1(1, -1, -1);
    Vector3 b2(-1, -1, -1);
    Vector3 b3(-1, -1, 1);

    // check faces of the cell
    Vector3 cellCenterLocal =  cellToWorld_smallestCorner(testCell) + Vector3(Voxel::kHALF_CELL, Voxel::kHALF_CELL, Voxel::kHALF_CELL);

    // bottom face (looking at positive z, with y pointing up prior to applying rotation)
    std::vector<Vector3> poly;
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * b0);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * b3);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * b2);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * b1);
    if( Math::intersectRayConvexPolygon(rayInMe, poly, localHitPoint, true) )
    {
        surfId = 4;
        surfaceNormal = (poly[1]-poly[0]).cross(poly[2]-poly[0]).unit();
        hitSurfaceCoord.rotation = Math::getWellFormedRotForZVector(surfaceNormal);
        hitSurfaceCoord.translation = poly[1];
        return true;
    }
    poly.clear();

    // right face
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * b0);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * b1);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * t1);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * t0);
    if( Math::intersectRayConvexPolygon(rayInMe, poly, localHitPoint, true) )
    {
        surfId = 0;
        surfaceNormal = (poly[1]-poly[0]).cross(poly[2]-poly[0]).unit();
        hitSurfaceCoord.rotation = Math::getWellFormedRotForZVector(surfaceNormal);
        hitSurfaceCoord.translation = poly[1];
        return true;
    }
    poly.clear();

    // back face
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * b1);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * b2);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * t2);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * t1);
    if( Math::intersectRayConvexPolygon(rayInMe, poly, localHitPoint, true) )
    {
        surfId = 5;
        surfaceNormal = (poly[1]-poly[0]).cross(poly[2]-poly[0]).unit();
        hitSurfaceCoord.rotation = Math::getWellFormedRotForZVector(surfaceNormal);
        hitSurfaceCoord.translation = poly[1];
       return true;
    }
    poly.clear();

    // left face
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * b3);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * t3);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * t2);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * b2);
    if( Math::intersectRayConvexPolygon(rayInMe, poly, localHitPoint, true) )
    {
        surfId = 3;
        surfaceNormal = (poly[1]-poly[0]).cross(poly[2]-poly[0]).unit();
        hitSurfaceCoord.rotation = Math::getWellFormedRotForZVector(surfaceNormal);
        hitSurfaceCoord.translation = poly[1];
        return true;
    }
    poly.clear();

    // front face
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * b0);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * t0);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * t3);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * b3);
    if( Math::intersectRayConvexPolygon(rayInMe, poly, localHitPoint, true) )
    {
        surfId = 2;
        surfaceNormal = (poly[1]-poly[0]).cross(poly[2]-poly[0]).unit();
        hitSurfaceCoord.rotation = Math::getWellFormedRotForZVector(surfaceNormal);
        hitSurfaceCoord.translation = poly[1];
        return true;
    }
    poly.clear();

    // top face
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * t0);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * t1);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * t2);
    poly.push_back(cellCenterLocal + Voxel::kHALF_CELL * t3);
    if( Math::intersectRayConvexPolygon(rayInMe, poly, localHitPoint, true) )
    {
        surfId = 1;
        surfaceNormal = (poly[1]-poly[0]).cross(poly[2]-poly[0]).unit();
        hitSurfaceCoord.rotation = Math::getWellFormedRotForZVector(surfaceNormal);
        hitSurfaceCoord.translation = poly[1];
        return true;
    }

    
    return false;
}

void MegaClusterPoly::findCellsTouchingGeometry( const CoordinateFrame& myCf, const Geometry& otherGeom, const CoordinateFrame& otherCf, std::vector<Vector3int16>* found ) const
{
    findCellsTouchingGeometryWithBuffer(0.0, myCf, otherGeom, otherCf, found);
}

void MegaClusterPoly::findCellsTouchingGeometryWithBuffer( const float& buffer, const CoordinateFrame& myCf, const Geometry& otherGeom, const CoordinateFrame& otherCf, std::vector<Vector3int16>* found ) const
{
    // blow out if extents are not touching
    Extents otherExtents(-0.5f * otherGeom.getSize(), 0.5f * otherGeom.getSize());
    otherExtents.scale(1.0f + buffer);

    myTerrainPartition->findCellsTouchingExtents(otherExtents.toWorldSpace(otherCf), found);
}

// we assume for now that the bounding box is axis-aligned to CFrame of MegaCluster
//	also, using spatial hashing, this code could be sped up a lot...
bool MegaClusterPoly::cellsInBoundingBox(const Vector3& min, const Vector3& max)
{
	Grid* grid = rbx_static_cast<MegaClusterInstance*>(myPrim->getOwner())->getVoxelGrid();

	Vector3int16 gridMin(worldToCell_floor(min));
	Vector3int16 gridMax(worldToCell_floor(max));

	const SpatialRegion::Id minChunkId = SpatialRegion::regionContainingVoxel(gridMin);
	const SpatialRegion::Id maxChunkId = SpatialRegion::regionContainingVoxel(gridMax);

	Vector3int16 counter = minChunkId.value();
	for (counter.y = minChunkId.value().y; counter.y <= maxChunkId.value().y; counter.y++) {
	for (counter.z = minChunkId.value().z; counter.z <= maxChunkId.value().z; counter.z++) {
	for (counter.x = minChunkId.value().x; counter.x <= maxChunkId.value().x; counter.x++) {
		const Region3int16 regionExtents = SpatialRegion::inclusiveVoxelExtentsOfRegion(SpatialRegion::Id(counter));

		const Vector3int16 minQueryCoord = regionExtents.getMinPos().max(gridMin);
		const Vector3int16 maxQueryCoord = regionExtents.getMaxPos().min(gridMax);

		Grid::Region region = grid->getRegion(
			minQueryCoord, maxQueryCoord);
		if (region.isGuaranteedAllEmpty()) {
			continue;
		}

		for (Grid::Region::iterator itr = region.begin();
				itr != region.end();
				++itr) {
			if (itr.getCellAtCurrentLocation().solid.getBlock() != CELL_BLOCK_Empty) {
				return true;
			}
		}
	}
	}
	}

	// no hits found
	return false;
}

void MegaClusterPoly::createBulletCellShapes(void)
{
	createBulletCubeCell();
	createBulletVerticalWedgeCell();
	createBulletHorizontalWedgeCell();
	createBulletCornerWedgeCell();
	createBulletInverseCornerWedgeCell();
}

void MegaClusterPoly::createBulletCubeCell(void)
{
	const btScalar shrunkenHalfCell = (float)kHALF_CELL - bulletCollisionMargin;
	bulletCellShapes[CELL_BLOCK_Solid]->addPoint(btVector3(shrunkenHalfCell,  shrunkenHalfCell,  shrunkenHalfCell), false);
	bulletCellShapes[CELL_BLOCK_Solid]->addPoint(btVector3(shrunkenHalfCell,  shrunkenHalfCell, -shrunkenHalfCell), false);
	bulletCellShapes[CELL_BLOCK_Solid]->addPoint(btVector3(shrunkenHalfCell, -shrunkenHalfCell,  shrunkenHalfCell), false);
	bulletCellShapes[CELL_BLOCK_Solid]->addPoint(btVector3(shrunkenHalfCell, -shrunkenHalfCell, -shrunkenHalfCell), false);
	bulletCellShapes[CELL_BLOCK_Solid]->addPoint(btVector3(-shrunkenHalfCell, shrunkenHalfCell,  shrunkenHalfCell), false);
	bulletCellShapes[CELL_BLOCK_Solid]->addPoint(btVector3(-shrunkenHalfCell, shrunkenHalfCell, -shrunkenHalfCell), false);
	bulletCellShapes[CELL_BLOCK_Solid]->addPoint(btVector3(-shrunkenHalfCell, -shrunkenHalfCell, shrunkenHalfCell), false);
	bulletCellShapes[CELL_BLOCK_Solid]->addPoint(btVector3(-shrunkenHalfCell, -shrunkenHalfCell, -shrunkenHalfCell), true);
}

void MegaClusterPoly::createBulletVerticalWedgeCell(void)
{
	const btScalar shrunkenHalfCell = (float)kHALF_CELL - bulletCollisionMargin;
	bulletCellShapes[CELL_BLOCK_VerticalWedge]->addPoint(btVector3(shrunkenHalfCell, -shrunkenHalfCell,  shrunkenHalfCell), false);
	bulletCellShapes[CELL_BLOCK_VerticalWedge]->addPoint(btVector3(shrunkenHalfCell, -shrunkenHalfCell, -shrunkenHalfCell), false);
    bulletCellShapes[CELL_BLOCK_VerticalWedge]->addPoint(btVector3(-shrunkenHalfCell, -shrunkenHalfCell, -shrunkenHalfCell), false);
    bulletCellShapes[CELL_BLOCK_VerticalWedge]->addPoint(btVector3(-shrunkenHalfCell, -shrunkenHalfCell, shrunkenHalfCell), false);
    bulletCellShapes[CELL_BLOCK_VerticalWedge]->addPoint(btVector3(shrunkenHalfCell, shrunkenHalfCell, -shrunkenHalfCell), false);
    bulletCellShapes[CELL_BLOCK_VerticalWedge]->addPoint(btVector3(-shrunkenHalfCell, shrunkenHalfCell, -shrunkenHalfCell), true);
}

void MegaClusterPoly::createBulletHorizontalWedgeCell(void)
{
	const btScalar shrunkenHalfCell = (float)kHALF_CELL - bulletCollisionMargin;
	bulletCellShapes[CELL_BLOCK_HorizontalWedge]->addPoint(btVector3(shrunkenHalfCell,  -shrunkenHalfCell,  shrunkenHalfCell), false);
	bulletCellShapes[CELL_BLOCK_HorizontalWedge]->addPoint(btVector3(shrunkenHalfCell,  -shrunkenHalfCell, -shrunkenHalfCell), false);
	bulletCellShapes[CELL_BLOCK_HorizontalWedge]->addPoint(btVector3(-shrunkenHalfCell, -shrunkenHalfCell,  -shrunkenHalfCell), false);
	bulletCellShapes[CELL_BLOCK_HorizontalWedge]->addPoint(btVector3(shrunkenHalfCell, shrunkenHalfCell, shrunkenHalfCell), false);
	bulletCellShapes[CELL_BLOCK_HorizontalWedge]->addPoint(btVector3(shrunkenHalfCell, shrunkenHalfCell,  -shrunkenHalfCell), false);
	bulletCellShapes[CELL_BLOCK_HorizontalWedge]->addPoint(btVector3(-shrunkenHalfCell, shrunkenHalfCell, -shrunkenHalfCell), true);
}

void MegaClusterPoly::createBulletCornerWedgeCell(void)
{
	const btScalar shrunkenHalfCell = (float)kHALF_CELL - bulletCollisionMargin;
	bulletCellShapes[CELL_BLOCK_CornerWedge]->addPoint(btVector3(shrunkenHalfCell, -shrunkenHalfCell,  shrunkenHalfCell), false);
	bulletCellShapes[CELL_BLOCK_CornerWedge]->addPoint(btVector3(shrunkenHalfCell, -shrunkenHalfCell, -shrunkenHalfCell), false);
	bulletCellShapes[CELL_BLOCK_CornerWedge]->addPoint(btVector3(-shrunkenHalfCell, -shrunkenHalfCell, -shrunkenHalfCell), false);
	bulletCellShapes[CELL_BLOCK_CornerWedge]->addPoint(btVector3(shrunkenHalfCell, shrunkenHalfCell, -shrunkenHalfCell), true);
}

void MegaClusterPoly::createBulletInverseCornerWedgeCell(void)
{
	const btScalar shrunkenHalfCell = (float)kHALF_CELL - bulletCollisionMargin;
	bulletCellShapes[CELL_BLOCK_InverseCornerWedge]->addPoint(btVector3(shrunkenHalfCell, -shrunkenHalfCell,  shrunkenHalfCell), false);
	bulletCellShapes[CELL_BLOCK_InverseCornerWedge]->addPoint(btVector3(shrunkenHalfCell, -shrunkenHalfCell, -shrunkenHalfCell), false);
	bulletCellShapes[CELL_BLOCK_InverseCornerWedge]->addPoint(btVector3(-shrunkenHalfCell, -shrunkenHalfCell,  -shrunkenHalfCell), false);
	bulletCellShapes[CELL_BLOCK_InverseCornerWedge]->addPoint(btVector3(-shrunkenHalfCell, -shrunkenHalfCell,  shrunkenHalfCell), false);
	bulletCellShapes[CELL_BLOCK_InverseCornerWedge]->addPoint(btVector3(shrunkenHalfCell, shrunkenHalfCell, shrunkenHalfCell), false);
	bulletCellShapes[CELL_BLOCK_InverseCornerWedge]->addPoint(btVector3(shrunkenHalfCell, shrunkenHalfCell, -shrunkenHalfCell), false);
	bulletCellShapes[CELL_BLOCK_InverseCornerWedge]->addPoint(btVector3(-shrunkenHalfCell, shrunkenHalfCell, -shrunkenHalfCell), true);
}

btConvexHullShape* MegaClusterPoly::getBulletCellShape(Voxel::CellBlock shape)
{
	RBXASSERT(shape >= 0 && shape < CELL_BLOCK_Empty);

	if (shape >= 0 && shape < CELL_BLOCK_Empty)
		return bulletCellShapes[shape];
	else
		return NULL;
}

} // namespace RBX
