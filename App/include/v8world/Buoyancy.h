#pragma once

#include "Voxel/Cell.h"
#include "v8kernel/BuoyancyConnector.h"
#include "v8World/Geometry.h"
#include "v8World/Contact.h"

/*
	The Buoyancy feature manages all aspects of parts' interaction with water when they have come
	into contact with water.

	The Buoyancy contact is implemented as a standard contact type managed by ContactManager.

	Each BuoyancyContact manages a few BuoyancyConnectors that represent the buoyancy and water
	viscosity forces applied on the part.

	Box Buoyancy is divided into 8 voxels, each voxel contributes one connector that represents
	the buoyancy and viscosity force applied on that voxel shape.
*/

namespace RBX {

	namespace Voxel { class Grid; }
	namespace Voxel2 { class Grid; }

	class BuoyancyContact : public Contact
	{
	public:
		static const int MAX_CONNECTORS = 8;
		static float waterViscosity;
		static const float waterDensity;

		typedef RBX::FixedArray<BuoyancyConnector*, MAX_CONNECTORS> ConnectorArray;

		static Geometry::GeometryType determineGeometricType( Primitive *prim );
		static BuoyancyContact* create( Primitive* p0, Primitive *p1 );
		
		BuoyancyContact( Primitive* p0, Primitive* p1 );
		~BuoyancyContact();

		virtual Geometry::GeometryType getType() = 0;

		ContactType getContactType() const override { return Contact_Buoyancy; }
	
		void onPrimitiveContactParametersChanged() override;

	private:
		void deleteConnectors();

		void updateBuoyancyFloatingForce();

	protected:
		ConnectorArray connectors;
		Primitive* floaterPrim;

        Voxel::Grid* voxelGrid;
        Voxel2::Grid* smoothGrid;

		float radius;
		float fullSurfaceArea;
		Vector3 fullBuoyancy;

		bool worldPosUnderWater( const Vector3& pos );
		bool isTouchingWater( Primitive* prim );

		Voxel::Cell getWaterCell( Vector3int16 pos );
		bool cellHasWater( Vector3int16 pos );

		bool hasDistanceSubmergedUnderWater( const Vector3& worldpos, float& waterLevel, const Vector3& searchEnd );
		bool worldPosAboveWater( const Vector3& worldpos, int minY, float& waterLevel );
		Vector3 cellVelocity( const Vector3& worldpos );

		void removeAllConnectorsFromKernel();
		void putAllConnectorsInKernel();
		void computeExtentsWaterBand( const Extents& extents, float& floatDistance, float& sinkDistance );
		void updateConnectors();

		// Contact API
		void deleteAllConnectors() override;
		int numConnectors() const override					{ return connectors.size(); }
		ContactConnector* getConnector( int i ) override	{ return connectors[i]; }
		bool stepContact() override;
		bool computeIsColliding(float overlapIgnored) override;
		bool computeIsCollidingUi(float overlapIgnored) override; // override to always return false so can build underwater; shouldn't affect HumanoidState code

		// Buoyancy Shape API 
		virtual Vector3 getWaterVelocity(int i);
		virtual void createConnectors() = 0;
		virtual void updateWaterBand() = 0;
		virtual void updateSubmergeRatio();
		virtual void getSurfaceAreaInDirection(const Vector3& relativeVelocity, float& crossArea, float& tangentArea) = 0;
		virtual void initializeCrossSections() = 0;
		virtual Vector3 getCrossSections(int i, const Vector3& velocity) = 0;
	};

	///////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////

	class BuoyancyBallContact : public BuoyancyContact
	{

	protected:
		float crossSectionArea;

		bool computeIsColliding(float overlapIgnored);
		void createConnectors();
		Vector3 getWaterVelocity(int i);
		void updateWaterBand();
		void updateSubmergeRatio();
		void getSurfaceAreaInDirection(const Vector3& relativeVelocity, float& crossArea, float& tangentArea);
		void initializeCrossSections();
		virtual Vector3 getCrossSections(int, const Vector3&);

	public:
		BuoyancyBallContact( Primitive* p0, Primitive* p1 ) : BuoyancyContact(p0, p1) {}
		Geometry::GeometryType getType() { return Geometry::GEOMETRY_BALL; }
	};

	///////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////

	class BuoyancyBoxContact : public BuoyancyContact
	{

	protected:
		Vector3 crossSectionSurfaceAreas;
		Vector3 tangentSurfaceAreas;

		void createConnectors();
		void updateWaterBand();
		virtual void getSurfaceAreaInDirection(const Vector3& relativeVelocity, float& crossSectionArea, float& tangentSurfaceAread);
		virtual void initializeCrossSections();
		Vector3 getCrossSections( int i, const Vector3& velocity );
	public:
		BuoyancyBoxContact( Primitive* p0, Primitive* p1 );
		Geometry::GeometryType getType() { return Geometry::GEOMETRY_BLOCK; }
	};

	///////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////

	class BuoyancyCylinderContact : public BuoyancyBoxContact
	{
	protected:
		void updateSubmergeRatio();
		void initializeCrossSections();

	public:
		BuoyancyCylinderContact( Primitive* p0, Primitive* p1 ) : BuoyancyBoxContact(p0, p1) {}
		Geometry::GeometryType getType() { return Geometry::GEOMETRY_CYLINDER; }
	};

	///////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////

	class BuoyancyWedgeContact : public BuoyancyBoxContact
	{

	protected:
		void updateSubmergeRatio();
		void initializeCrossSections();

	public:
		BuoyancyWedgeContact( Primitive* p0, Primitive* p1 ) : BuoyancyBoxContact(p0, p1) {}
		Geometry::GeometryType getType() { return Geometry::GEOMETRY_WEDGE; }
	};

	///////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////

	class BuoyancyCornerWedgeContact : public BuoyancyBoxContact
	{
	protected:
		void updateSubmergeRatio();
		void initializeCrossSections();

	public:
		BuoyancyCornerWedgeContact( Primitive* p0, Primitive* p1 ) : BuoyancyBoxContact(p0, p1) {}
		Geometry::GeometryType getType() { return Geometry::GEOMETRY_CORNERWEDGE; }
	};
}
