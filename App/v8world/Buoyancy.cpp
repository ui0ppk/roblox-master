#include "stdafx.h"

#include "v8World/Buoyancy.h"

#include "Util/Math.h"
#include "Voxel/Util.h"
#include "V8DataModel/MegaCluster.h"
#include "V8Kernel/Kernel.h"
#include "V8Kernel/Body.h"
#include "V8Kernel/Constants.h"
#include "V8World/ContactManager.h"
#include "V8World/Primitive.h"
#include "V8World/World.h"
#include "v8world/Assembly.h"

#include "voxel/Grid.h"
#include "voxel2/Grid.h"

namespace RBX
{
	using namespace Voxel;
	///////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////

	float BuoyancyContact::waterViscosity = 0.02;			// This is exposed as prop_WaterViscosity in PhysicsSettings.cpp
	const float BuoyancyContact::waterDensity = 1.00f;

	void BuoyancyContact::removeAllConnectorsFromKernel()
	{
		Kernel* kernel = NULL;
		for (size_t i = 0; i < connectors.size(); ++i) {
			if (connectors[i]->isInKernel()) {
				kernel = kernel ? kernel : getKernel();		// small optimization - getKernel walks the IPipelines
				kernel->removeConnector(connectors[i]);
			}
		}
	}

	void BuoyancyContact::putAllConnectorsInKernel()
	{
		Kernel* kernel = getKernel();;
		for (size_t i = 0; i < connectors.size(); ++i) {
			if (!connectors[i]->isInKernel()) {
				kernel->insertConnector(connectors[i]);
			}
		}
	}

	void BuoyancyContact::deleteConnectors()
	{
		removeAllConnectorsFromKernel();
		for (size_t i = 0; i < connectors.size(); ++i) {
			RBXASSERT(!connectors[i]->isInKernel());
			delete connectors[i];
		}
		connectors.fastClear();
		
		floaterPrim->onBuoyancyChanged( false );
	}

	void BuoyancyContact::deleteAllConnectors()
	{
		deleteConnectors();
	}

	Geometry::GeometryType BuoyancyContact::determineGeometricType( Primitive *primitive )
	{
		if (RBX::PartInstance::fromPrimitive(primitive)->getPartType() == CYLINDER_PART)
			return Geometry::GEOMETRY_CYLINDER;
		if (primitive->getCollideType() == Geometry::COLLIDE_BALL)
			return Geometry::GEOMETRY_BALL;
		if (primitive->getGeometryType() == Geometry::GEOMETRY_WEDGE)
			return Geometry::GEOMETRY_WEDGE;
		if (primitive->getGeometryType() == Geometry::GEOMETRY_CORNERWEDGE)
			return Geometry::GEOMETRY_CORNERWEDGE;
		return Geometry::GEOMETRY_BLOCK;
	}

	void BuoyancyContact::updateBuoyancyFloatingForce()
	{
		fullBuoyancy.x = 0.0f;
		fullBuoyancy.z = 0.0f;
		if (getPrimitive(0)->getWorld()->getUsingNewPhysicalProperties())
		{
			// Buoyancy force does not depend on the object density. Only water density.
			fullBuoyancy.y = - floaterPrim->getGeometry()->getVolume() * Units::kmsAccelerationToRbx( RBX::Constants::getKmsGravity() ) * waterDensity;
		}
		else
		{
			// Buoyant force:			Fb = p_water * grav_constant * Volume_displaced
			// Can be expanded into:	Fb = p_o * g * V / (p_w/p_o)
			// Unfortunately legacy Roblox never knew the correct mass of an object, so what the hell?
			fullBuoyancy.y = - floaterPrim->getConstBody()->getMass() * (Units::kmsAccelerationToRbx( RBX::Constants::getKmsGravity() ) / floaterPrim->getSpecificGravity());
		}
	}

	BuoyancyContact::BuoyancyContact( Primitive* p0, Primitive* p1 ) :
		floaterPrim(p1),
		Contact(p0, p1),
		radius(floaterPrim->getRadius()),
		fullSurfaceArea(0.0f),
        voxelGrid(NULL),
        smoothGrid(NULL)
	{
		updateBuoyancyFloatingForce();
		MegaClusterInstance* mci = boost::polymorphic_downcast<MegaClusterInstance*>(p0->getOwner());

		if (mci->isSmooth())
			smoothGrid = mci->getSmoothGrid();
		else
			voxelGrid = mci->getVoxelGrid();
	}

	BuoyancyContact::~BuoyancyContact()
	{
		deleteConnectors();
		RBXASSERT(connectors.size() == 0);
		floaterPrim->onBuoyancyChanged( false );
	}


	void BuoyancyContact::computeExtentsWaterBand( const Extents& extents, float& floatDistance, float& sinkDistance )
	{
		float waterLevel;

		if (hasDistanceSubmergedUnderWater(extents.min(), waterLevel, extents.max())) {
			floatDistance = waterLevel - extents.min().y;
			RBXASSERT(floatDistance >= -1e-5);
			if (worldPosUnderWater(extents.max())) {
				// fully submerged
				sinkDistance = -1.0f;
			} else if (worldPosAboveWater(extents.max(), extents.min().y, waterLevel)) {
				// Partially submerged
				sinkDistance = extents.max().y - waterLevel;
				RBXASSERT(sinkDistance >= -1e-5);
			} else {
				// Degenerate case: no water found below the top point within the extent.
				// Bottom side in the water while top side out of water.
				// Let's test the center and approximate
				if (worldPosUnderWater(extents.bottomCenter()))
					sinkDistance = floatDistance;     // receive 1/2 of the buoyancy
				else
					floatDistance = -1.0f;			  // receive no buoyancy
			}
		} else {
			// No water found above the bottom point
			if (hasDistanceSubmergedUnderWater(extents.max(), waterLevel, extents.max())) {
				floatDistance = waterLevel - extents.min().y;
				RBXASSERT(floatDistance >= -1e-5);
				if (worldPosAboveWater(extents.min(), extents.min().y - floatDistance, waterLevel)) {
					// Partially submerged
					sinkDistance = extents.max().y - waterLevel;
				} else {
					// Degenerate case: No water found below the bottom point within extent.
					// Top side in the water while bottom side out of water.
					// Let's test the center and approximate
					if (worldPosUnderWater(extents.bottomCenter()))
						sinkDistance = floatDistance;     // receive 1/2 of the buoyancy
					else
						floatDistance = -1.0f;			  // receive no buoyancy
				}
			} else {
				// Make sure we are completely out of water
				floatDistance = 0.0f;
				for (unsigned int i = 0; i < 7; ++i) {
					Vector3 corner = extents.getCorner(i);
					if (corner != extents.min() && corner != extents.max() && worldPosUnderWater(corner))
						floatDistance += 1.0f;
				}
				if (floatDistance > 0.0f) {
					// Aha, singular case, some middle corners are in the water.
					// Sample more points to have a smoother approximation				
					if (worldPosUnderWater(extents.center()))
						floatDistance += 1.0f;
					for (unsigned int i = 0; i < 7; ++i) {
						Vector3 midCorner = (extents.getCorner(i) + extents.center()) / 2;
						if (worldPosUnderWater(midCorner))
							floatDistance += 1.0f;
					}
					sinkDistance = 17.0f - floatDistance;
				} else
					floatDistance = -1.0f; // completely out of water
			}
		}
	}

	// Linear Interpolation.
	// Override this to provide more accurate interpolation for a specific shape
	//

	void BuoyancyContact::updateSubmergeRatio()
	{
		for (unsigned int i = 0; i < connectors.size(); ++i) {
			float floatDistance, sinkDistance;
			connectors[i]->getWaterBand(floatDistance, sinkDistance);
			if (floatDistance <= 0.0f)
				connectors[i]->setSubMergeRatio(0.0f);	// outside water
			else if (sinkDistance <= 0.0f)
				connectors[i]->setSubMergeRatio(1.0f);	// fully submerged
			else 
				connectors[i]->setSubMergeRatio(floatDistance / (floatDistance + sinkDistance));
		}
	}

	Vector3 BuoyancyContact::getWaterVelocity( int i )
	{
		return cellVelocity( connectors[i]->getWorldPosition() );
	}

	void BuoyancyContact::onPrimitiveContactParametersChanged()
	{
		if (floaterPrim->getWorld() && floaterPrim->getWorld()->getUsingNewPhysicalProperties())
		{
			floaterPrim->getWorld()->ticklePrimitive(floaterPrim, true);
		}
	}

	void BuoyancyContact::updateConnectors()
	{

		const Vector3 partialBuoyancy = fullBuoyancy / connectors.size();
		float viscosity_K = -waterViscosity / RBX::Constants::kernelDt() / connectors.size();

		for (unsigned int i = 0; i < connectors.size(); ++i) {
			float submergeRatio = connectors[i]->getSubMergeRatio();

			if (submergeRatio > 0.0f) {
				const Vector3 relativeVelocity = floaterPrim->getPV().velocity.linear - getWaterVelocity(i);

				if ( relativeVelocity.isZero() ) // to avoid division by zero
					connectors[i]->setForce( partialBuoyancy * submergeRatio );
				else
				{
					const Vector3 relativeVelocityInObject = floaterPrim->getCoordinateFrame().vectorToObjectSpace(relativeVelocity);
					// Scale the viscosity by the cross sections in axis aligned directions
					Vector3 viscosityInObject = relativeVelocityInObject * getCrossSections(i, relativeVelocityInObject);
					Vector3 linearViscosity = viscosity_K * floaterPrim->getCoordinateFrame().vectorToWorldSpace(viscosityInObject);
					connectors[i]->setForce( ( partialBuoyancy + linearViscosity ) * submergeRatio );
				}
			} else
				connectors[i]->setForce( Vector3::zero() );
		}
	}


	bool BuoyancyContact::stepContact()
	{
		if (computeIsColliding(0.0)) {
			if (inKernel()) {
				if (connectors.size() == 0) {
					createConnectors();
					putAllConnectorsInKernel();
				}
				updateWaterBand();
				updateSubmergeRatio();
				updateConnectors();
			}
			floaterPrim->onBuoyancyChanged( true );
			return true;
		}
		else {
			deleteAllConnectors();
			return false;
		}
	}

	bool BuoyancyContact::cellHasWater( Vector3int16 pos )
	{
        if (smoothGrid)
			return smoothGrid->getCell(pos.x, pos.y, pos.z).getMaterial() == Voxel2::Cell::Material_Water;
		else
            return !voxelGrid->getWaterCell(pos).isEmpty();
	}

	bool BuoyancyContact::worldPosUnderWater( const Vector3& pos )
	{
		Vector3int16 internalPos(worldToCell_floor(pos));
		return cellHasWater(internalPos);
	}

	// Search water level above the specified world pos if under water
	bool BuoyancyContact::hasDistanceSubmergedUnderWater( const Vector3& worldpos, float& waterLevel, const Vector3& worldMaxSearch )
	{
		Vector3int16 pos(worldToCell_floor(worldpos));
		if (!cellHasWater(pos))
			return false;

		Vector3int16 endPos(worldToCell_floor(worldMaxSearch));
		pos.y++;
		// TODO: Optimization: Lazy eval material
		while (pos.y <= endPos.y && cellHasWater(pos))
			pos.y++;

		waterLevel = cellToWorld_smallestCorner(pos).y;
		return true;
	}

	// Search water level below the specified world pos if above water
	bool BuoyancyContact::worldPosAboveWater( const Vector3& worldpos, int minY, float& waterLevel )
	{
		Vector3int16 pos(worldToCell_floor(worldpos));
		// Skip the current cell the worldpos is located
		do {
			waterLevel = cellToWorld_smallestCorner(pos).y;
			pos.y--;
		} while (!cellHasWater(pos) && waterLevel >= minY);

		if (waterLevel >= minY)
			return true;			// found water below the position

		return false;		// no water below the position and above minY
	}

	Vector3 BuoyancyContact::cellVelocity( const Vector3& worldpos )
	{
        if (!voxelGrid)
			return Vector3::zero();

		Vector3int16 pos(worldToCell_floor(worldpos));
		Cell cell = voxelGrid->getWaterCell(pos);

		if (cell.isEmpty())
			return Vector3::zero();
	
		int magnitude = 16 * cell.water.getForce();
		Vector3 velocity(0.0f, 0.0f, 0.0f);
		velocity[cell.water.getDirection() >> 1] = (cell.water.getDirection() & 0x1) ? magnitude : -magnitude;
		return velocity;
	}

	// extremely expedited "broadphase" check
	bool BuoyancyContact::isTouchingWater( Primitive* prim )
	{
		Extents extents = prim->getExtentsWorld();
		// check center
		if (worldPosUnderWater(extents.center()))
			return true;

		// check circumscribing corners
		for (int i = 0; i < 8; ++i)
			if (worldPosUnderWater(extents.getCorner(i)))
				return true;

		return false;
	}

	bool BuoyancyContact::computeIsColliding( float ) 
	{
		return isTouchingWater(floaterPrim);
	}

	bool BuoyancyContact::computeIsCollidingUi( float )
	{
		// override to always return false so can build underwater; shouldn't affect HumanoidState code
		getPrimitive(0)->getFastFuzzyExtents();			// updates - outside of world loop
		getPrimitive(1)->getFastFuzzyExtents();
		return false;
	}

	BuoyancyContact* BuoyancyContact::create( Primitive* p0, Primitive *p1 )
	{
		Geometry::GeometryType geoType = BuoyancyContact::determineGeometricType(p1);
		BuoyancyContact* contact = NULL;
		
		switch( geoType )
		{
			case Geometry::GEOMETRY_BALL:
				contact = new BuoyancyBallContact(p0, p1);
				break;
			case Geometry::GEOMETRY_CYLINDER:
				contact = new BuoyancyCylinderContact(p0, p1);
				break;
			case Geometry::GEOMETRY_BLOCK:
				contact = new BuoyancyBoxContact(p0, p1);
				break;
			case Geometry::GEOMETRY_WEDGE:
				contact = new BuoyancyWedgeContact(p0, p1);
				break;
			case Geometry::GEOMETRY_CORNERWEDGE:
				contact = new BuoyancyCornerWedgeContact(p0, p1);
				break;
			default:
				RBXASSERT(contact);
		}

		contact->initializeCrossSections();

		return contact;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////

	void BuoyancyBallContact::initializeCrossSections()
	{
		radius = floaterPrim->getSize().x / 2.0f;
		// Partial submerged cross section will be interpolated as submerged volume
		crossSectionArea = Math::pif() * radius * radius;

		fullSurfaceArea = 4.0f * crossSectionArea;
	}

	void BuoyancyBallContact::getSurfaceAreaInDirection(const Vector3& relativeVelocity, float& crossArea, float& tangentArea)
	{
		crossArea = crossSectionArea;
		tangentArea = 2.0f * crossSectionArea;
	}

	Vector3 BuoyancyBallContact::getCrossSections(int, const Vector3&)
	{
		return Vector3(crossSectionArea, crossSectionArea, crossSectionArea) / 8.0f;
	}

	bool BuoyancyBallContact::computeIsColliding( float overlapIgnored )
	{
		Vector3 center = floaterPrim->getPV().position.translation;
		if (worldPosUnderWater(center))
			return true;

		Extents extents = floaterPrim->getExtentsLocal();
		for (int faceId = NORM_X; faceId <= NORM_Z_NEG; ++faceId)
			if (worldPosUnderWater(center + extents.faceCenter(static_cast<NormalId>(faceId))))
				return true;
		return false;
	}

	void BuoyancyBallContact::createConnectors()
	{
		connectors.push_back(new BuoyancyConnector(getBody(0), getBody(1), Vector3::zero()));
	}

	void BuoyancyBallContact::updateWaterBand()
	{
		RBXASSERT(connectors.size() == 1);  // Ball has only one connector	

		float floatDistance, sinkDistance, waterLevel;
		Vector3 center = floaterPrim->getPV().position.translation;
		Vector3 bottom = center + Vector3(0.0f, -radius, 0.0f);
		Vector3 top = center + Vector3(0.0f, radius, 0.0f);
	
		if (hasDistanceSubmergedUnderWater(bottom, waterLevel, top)) {
			floatDistance = waterLevel - bottom.y;
			RBXASSERT(floatDistance >= -1e-5);
			if (worldPosUnderWater(top)) {
				// fully submerged
				sinkDistance = -1.0f;
			} else {
				bool aboveWater = worldPosAboveWater(top, bottom.y, waterLevel);
				RBXASSERT(aboveWater);
				// Partially submerged
				sinkDistance = top.y - waterLevel;
				RBXASSERT(sinkDistance >= -1e-5);
			}
		} else {
			// No water found above the bottom point
			if (worldPosAboveWater(top, bottom.y, waterLevel)) {
				floatDistance = top.y - waterLevel;
				RBXASSERT(floatDistance >= -1e-5);
				sinkDistance = waterLevel - bottom.y;
				RBXASSERT(sinkDistance >= -1e-5);
			} else {
				// De-generate case : No water found at the top and bottom
				// Just sample the 4 sides and give a rough estimate
				float leftHasWater = worldPosUnderWater(center + Vector3(-radius, 0.0f, 0.0f)) ? 1.0f : 0.0f;
				float rightHasWater = worldPosUnderWater(center + Vector3(radius, 0.0f, 0.0f)) ? 1.0f : 0.0f;
				float frontHasWater = worldPosUnderWater(center + Vector3(0.0f, 0.0f, -radius)) ? 1.0f : 0.0f;
				float backHasWater = worldPosUnderWater(center + Vector3(0.0f, 0.0f, radius)) ? 1.0f : 0.0f;
				floatDistance = leftHasWater + rightHasWater + frontHasWater + backHasWater;
				sinkDistance = 4.0f - floatDistance;
			}
		}

		connectors[0]->setWaterBand(floatDistance, sinkDistance);
	}

	// override to interpolate over volume accurately
	//    integration over water depth yields volume fraction as (2 + 3k - k^3)/4 where k is normalized water depth over [-1, 1]
	void BuoyancyBallContact::updateSubmergeRatio()
	{
		RBXASSERT(connectors.size() == 1);
		float floatDistance, sinkDistance;
		connectors[0]->getWaterBand(floatDistance, sinkDistance);
		if (floatDistance <= 0.0f)
			connectors[0]->setSubMergeRatio(0.0f);	// out of water
		else if (sinkDistance <= 0.0f)
			connectors[0]->setSubMergeRatio(1.0f);	// fully submerged
		else {
			float normalizedDepth = floatDistance / ( floatDistance + sinkDistance );
			connectors[0]->setSubMergeRatio( normalizedDepth * normalizedDepth * (3 - 2 * normalizedDepth) );
		}
	}

	RBX::Vector3 BuoyancyBallContact::getWaterVelocity( int )
	{
		Vector3 center = floaterPrim->getPV().position.translation;
		Vector3 velocity = cellVelocity( center );
		if (velocity != Vector3::zero())
			return velocity;

		// Check center of 6 faces of the bounding box
		Extents extents = floaterPrim->getExtentsLocal();
		for (int faceId = NORM_X; faceId <= NORM_Z_NEG; ++faceId) {
			velocity = cellVelocity( center + extents.faceCenter(static_cast<NormalId>(faceId)) );
			if (velocity != Vector3::zero())
				return velocity;
		}
		return velocity;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////

	void BuoyancyBoxContact::initializeCrossSections()
	{
		// Cross section and tangent surface area depend on the direction of relative water velocity: 
		//   for full submergence, just transform relative velocity direction into box CFrame, and 
		//						  dot product it with the pre-computed axis aligned cross section vector
		//   for partial submergence,  linear interpolation on the 3 surface areas (faces) would probably be more than enough
		Vector3 boxSize = floaterPrim->getSize();
		crossSectionSurfaceAreas.x = boxSize.y * boxSize.z;
		crossSectionSurfaceAreas.y = boxSize.x * boxSize.z;
		crossSectionSurfaceAreas.z = boxSize.x * boxSize.y;
		tangentSurfaceAreas.x = 2.0f * ( crossSectionSurfaceAreas.y + crossSectionSurfaceAreas.z );
		tangentSurfaceAreas.y = 2.0f * ( crossSectionSurfaceAreas.x + crossSectionSurfaceAreas.z );
		tangentSurfaceAreas.z = 2.0f * ( crossSectionSurfaceAreas.x + crossSectionSurfaceAreas.y );

		fullSurfaceArea = crossSectionSurfaceAreas.sum() * 2.0f;
	}

	Vector3 BuoyancyBoxContact::getCrossSections(int i, const Vector3& velocity)
	{
		const Extents extents = floaterPrim->getExtentsLocal();
		Vector3int16 index = extents.getCornerIndex(i);
		// Polarity: 0 - negative axis direction points outwards
		//           1 - positive axis direction points outwards
		// index.x : 0,0,0,0,1,1,1,1
		// index.y : 0,0,1,1,0,0,1,1
		// index.z : 0,1,0,1,0,1,0,1
		Vector3 crossAreas[2];
		crossAreas[0] = crossSectionSurfaceAreas / 4.0f;
		crossAreas[1] = Vector3::zero();
		Vector3int16 sign(velocity.x > 0, velocity.y > 0, velocity.z > 0);;
		return Vector3( crossAreas[sign.x ^ index.x].x,
					    crossAreas[sign.y ^ index.y].y,
					    crossAreas[sign.z ^ index.z].z );
	}

	void BuoyancyBoxContact::getSurfaceAreaInDirection(const Vector3& relativeVelocityDir, float& crossArea, float& tangentArea)
	{
		Vector3 componentWeights = relativeVelocityDir * relativeVelocityDir;
		RBXASSERT(Math::fuzzyEq(componentWeights.x + componentWeights.y + componentWeights.z, 1.0f, 1.0e-4f));

		crossArea = crossSectionSurfaceAreas.dot(componentWeights);
		tangentArea = tangentSurfaceAreas.dot(componentWeights);
	}

	void BuoyancyBoxContact::createConnectors()
	{
		// 8 corners
		const Extents extents = floaterPrim->getExtentsLocal();
		for (unsigned int i = 0; i < 8; ++i)
			connectors.push_back(new BuoyancyConnector(getBody(0), getBody(1), extents.getCorner(i) / 2.0f));
	}

	void BuoyancyBoxContact::updateWaterBand()
	{
		// A box is divided into 8 voxels, one for each connector.
		// Each voxel computes its own submersion ratio.
		RBXASSERT(connectors.size() == 8);

		for (unsigned int i = 0; i < connectors.size(); ++i) {
			float floatDistance, sinkDistnace;		
			Extents primExtents = floaterPrim->getExtentsLocal();
			Extents voxelExtents = Extents::vv(Vector3::zero(), primExtents.getCorner(i));
			Extents voxelWorldExtents = voxelExtents.toWorldSpace(floaterPrim->getCoordinateFrame());
			computeExtentsWaterBand(voxelWorldExtents, floatDistance, sinkDistnace);
			connectors[i]->setWaterBand(floatDistance, sinkDistnace);
		}
	}

	BuoyancyBoxContact::BuoyancyBoxContact( Primitive* p0, Primitive* p1 ) : BuoyancyContact(p0, p1)
	{
		crossSectionSurfaceAreas = tangentSurfaceAreas = Vector3::zero();
	}

	///////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////

	void BuoyancyCylinderContact::initializeCrossSections()
	{
		BuoyancyBoxContact::initializeCrossSections();

		// same as box, except caps are circles instead of rectangles
		// Note: This is a good approximation which doesn't use sqrt, but to be technically accurate, we would need our
		//       getSurfaceAreaInDirection to return 
		//			  surfaceAreas[0]*direction.x + surfaceAreas[1]*(sqrt(direction.y^2 + direction.z^2))
		//       instead of the current effective return of
		//			  surfaceAreas[0]*direction.x + surfaceAreas[1]*(direction.y + direction.z)
		float cylinderRadius = floaterPrim->getSize().y / 2.0f;
		crossSectionSurfaceAreas.x = Math::pif() * cylinderRadius * cylinderRadius;
	}

	// High-fidelity Cylinder Interpolator:
	//   Blend (based on how "vertical" cylinder is) between the linear BuoyancyBoxContact::updateSubmergeRatio()
	//   and a spherical interpolator for cylinder in horizontal position as the following:
	//   v = 1/2 + (arcsin(x) + x*sqrt(1 - x^2))/pi, where v is the fraction of the volume and x is the fractional
	//   height of cylinder under water over the interval [-1, 1].
	void RBX::BuoyancyCylinderContact::updateSubmergeRatio()
	{
		// Just do a linear interpolation for vertical case
		BuoyancyBoxContact::updateSubmergeRatio();

		RBXASSERT(connectors.size() == 8);
		// we average the 4 ratios, and then map them to [-1, 1] from [0, 1]
		float startPointRatio = (connectors[0]->getSubMergeRatio() + connectors[1]->getSubMergeRatio() +
								 connectors[2]->getSubMergeRatio() + connectors[3]->getSubMergeRatio()) * 0.5f - 1.0f;
		float endPointRatio = (connectors[4]->getSubMergeRatio() + connectors[5]->getSubMergeRatio() +
							   connectors[6]->getSubMergeRatio() + connectors[7]->getSubMergeRatio()) * 0.5f - 1.0f;

		// the length of the cylinder runs along its local x-axis
		static const Vector3 upVector(0.0f, 1.0f, 0.0f);
		float verticality = fabs((floaterPrim->getCoordinateFrame().rightVector().unit()).dot(upVector));

		// we calculate the horizontal cylinder ratios here
		static const float pi_inverse = 1.0f / 3.1415926535f;
		startPointRatio = 0.5f + ( asinf(startPointRatio) + startPointRatio * sqrt(1.0f - startPointRatio*startPointRatio) ) * pi_inverse;
		endPointRatio = 0.5f + ( asinf(endPointRatio) + endPointRatio * sqrt(1.0f - endPointRatio*endPointRatio) ) * pi_inverse;

		// we use verticality to weight the average between the original submergeRatio and the horizontal-cylinder ratios
		for (int i = 0; i < 4; i++)
			connectors[i]->setSubMergeRatio( verticality * (connectors[i]->getSubMergeRatio()) + (1.0f - verticality) * (startPointRatio) );

		for (int i = 4; i < 8; i++)
			connectors[i]->setSubMergeRatio( verticality * (connectors[i]->getSubMergeRatio()) + (1.0f - verticality) * (endPointRatio) );
	}

	///////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////

	void BuoyancyWedgeContact::initializeCrossSections()
	{
		Vector3 boxSize = floaterPrim->getSize();
		crossSectionSurfaceAreas.x = boxSize.y * boxSize.z / 2.0f;
		crossSectionSurfaceAreas.y = boxSize.x * boxSize.z;
		crossSectionSurfaceAreas.z = boxSize.x * boxSize.y;

		float slopingSurfaceArea = boxSize.x * sqrtf(boxSize.y*boxSize.y + boxSize.z*boxSize.z);
		
		tangentSurfaceAreas.x = crossSectionSurfaceAreas.y + crossSectionSurfaceAreas.z + slopingSurfaceArea;
		tangentSurfaceAreas.y = 2.0f * ( crossSectionSurfaceAreas.x + crossSectionSurfaceAreas.z );
		tangentSurfaceAreas.z = 2.0f * ( crossSectionSurfaceAreas.x + crossSectionSurfaceAreas.y );

		fullSurfaceArea = 2.0f * crossSectionSurfaceAreas.x + tangentSurfaceAreas.x;
	}

	// apply weights to voxels for approximation of wedge shape volumes
	void BuoyancyWedgeContact::updateSubmergeRatio()
	{
		BuoyancyBoxContact::updateSubmergeRatio();
		// GetMass() still returns a mass as if it were a box, so voxel weights must sum to number of voxels (8 here)
		static const float WedgeVoxelWeightArray[8] = {1.0f, 2.0f, 0.0f, 1.0f, 1.0f, 2.0f, 0.0f, 1.0f};
		RBXASSERT(connectors.size() == 8);
		for (unsigned int i = 0; i < connectors.size(); ++i)
			connectors[i]->setSubMergeRatio( WedgeVoxelWeightArray[i] * connectors[i]->getSubMergeRatio() );
	}

	///////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////

	void BuoyancyCornerWedgeContact::initializeCrossSections()
	{
		Vector3 boxSize = floaterPrim->getSize();
		crossSectionSurfaceAreas.x = boxSize.y * boxSize.z / 2.0f;
		crossSectionSurfaceAreas.y = boxSize.x * boxSize.z;
		crossSectionSurfaceAreas.z = boxSize.x * boxSize.y / 2.0f;

		tangentSurfaceAreas.x = 2.0f * ( crossSectionSurfaceAreas.y + crossSectionSurfaceAreas.z );
		tangentSurfaceAreas.y = 2.0f * ( crossSectionSurfaceAreas.x + crossSectionSurfaceAreas.z );
		tangentSurfaceAreas.z = 2.0f * ( crossSectionSurfaceAreas.x + crossSectionSurfaceAreas.y );

		float slopingSurfaceArea1 = boxSize.x * sqrtf(boxSize.y*boxSize.y + boxSize.z*boxSize.z);
		float slopingSurfaceArea2 = boxSize.z * sqrtf(boxSize.y*boxSize.y + boxSize.x*boxSize.x);
		fullSurfaceArea = crossSectionSurfaceAreas.sum() +  slopingSurfaceArea1 + slopingSurfaceArea2;
	}

	void BuoyancyCornerWedgeContact::updateSubmergeRatio()
	{
		BuoyancyBoxContact::updateSubmergeRatio();
		static const float CornerWedgeVoxelWeightArray[8] = {1.5f, 1.0f, 0.0f, 0.0f, 3.0f, 1.5f, 1.0f, 0.0f};
		RBXASSERT(connectors.size() == 8);
		for (unsigned int i = 0; i < connectors.size(); ++i)
			connectors[i]->setSubMergeRatio( CornerWedgeVoxelWeightArray[i] * connectors[i]->getSubMergeRatio() );
	}

}
