/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Kernel/BodyPvSetter.h"
#include "V8World/Geometry.h"
#include "V8World/Edge.h"
#include "V8World/SurfaceData.h"
#include "V8World/IMoving.h"
#include "V8World/BasicSpatialHashPrimitive.h"
#include "Util/SurfaceType.h"
#include "Util/Extents.h"
#include "Util/Face.h"
#include "Util/NormalId.h"
#include "Util/ComputeProp.h"
#include "Util/G3DCore.h"
#include "Util/Guid.h"
#include "Util/IndexArray.h"
#include "Util/SpanningNode.h"
#include "Util/SystemAddress.h"
#include "Util/CompactEnum.h"
#include "util/PhysicalProperties.h"
#include "util/PartMaterial.h"
#include <string>
#include <boost/flyweight.hpp>

namespace RBX {
	enum NetworkOwnership
	{
		NetworkOwnership_Auto = 0, 
		NetworkOwnership_Manual = 1 
	};

	class Body;
	class Clump;
	class Assembly;
	class Mechanism;
	class World;
	template<class P, class C, class CM, int M> class SpatialHash;
	class Contact;
	class Joint;
	struct RootPrimitiveOwnershipData;

	class EdgeList
	{
	private:
		struct Entry
		{
			Edge* edge;
			Primitive* other;
		};

		Primitive* owner;
		std::vector<Entry> list;

	public:
		EdgeList(Primitive* owner) : owner(owner)
		{}

		~EdgeList() {
			RBXASSERT(list.size() == 0);
		}

		int size() const			{return list.size();}

		Edge* getEdge(int i) const
		{
			RBXASSERT_VERY_FAST(unsigned(i) < list.size());
			RBXASSERT_VERY_FAST(list[i].edge->otherPrimitive(owner) == list[i].other);
			return list[i].edge;
		}

		Primitive* getOther(int i) const
		{
			RBXASSERT_VERY_FAST(unsigned(i) < list.size());
			RBXASSERT_VERY_FAST(list[i].edge->otherPrimitive(owner) == list[i].other);
			return list[i].other;
		}

		Edge* getFirst() const		{return (list.size() > 0) ? list[0].edge : NULL;}

		Edge* getNext(const Primitive* p, Edge* e) const;

		void insertEdge(Edge* e);
		void removeEdge(Edge* e);
	};

	class Primitive : public IPipelined
					, public SpanningNode
					, public BodyPvSetter
					, public BasicSpatialHashPrimitive
	{
		template<class P, class C, class CM, int M> friend class SpatialHash;


	public:
		static bool allowSleep;

		typedef enum {DYNAMICS_ENGINE, HUMANOID_ENGINE} EngineType;
		typedef enum {DEFAULT_SIZE, TORSO_SIZE, ROOT_SIZE, SEAT_SIZE} SizeMultiplier;	// overweights torsos and seats to make them roots of the spanning tree

		// bullet related
		void updateBulletCollisionObject(void);

		// Moved to public for CSG
		Vector3 clipToSafeSize(const Vector3& newSize);

	// WORLD access here
	public:
		int& worldIndexFunc() {return worldIndex;}

	private:
		void onChangedInKernel();

		// For fuzzyExtents - when updated
		static int fuzzyExtentsReset() {return -2;}	// out of synch with body -1;

		Extents computeFuzzyExtents();
		void setFixed(bool newAnchoredProperty, bool newDragging);

		float computeJointK();

		Geometry* newGeometry(Geometry::GeometryType geometryType);

	public:
		Primitive(Geometry::GeometryType geometryType);
		virtual ~Primitive();

		const Guid& getGuid() const;
		void setGuid(const Guid& value);

		unsigned int getSizeMultiplier() const;
		void setSizeMultiplier(SizeMultiplier value);

		void calculateSortSize();
		unsigned int getSortSize();

		World* getWorld() const						{return world;}
		void setWorld(World* _world)				{world = _world;}

		// Clump
		Clump* getClump();					
		const Clump* getConstClump() const;					
		Assembly* getAssembly();
		const Assembly* getConstAssembly() const;
		Mechanism* getMechanism();
		const Mechanism* getConstMechanism() const;

		// Geometry
		Geometry* getGeometry()						{return geometry;}
		const Geometry* getConstGeometry() const	{return geometry;}

		void resetGeometryType(Geometry::GeometryType geometryType);
		void setGeometryType(Geometry::GeometryType geometryType);
		Geometry::GeometryType getGeometryType() const;
		Geometry::CollideType getCollideType() const;

		// Body
		Body* getBody()								{return body;}
		const Body* getConstBody() const			{return body;}

		// Class PartInstance
		void setOwner(IMoving* set);
		IMoving* getOwner() const					{return myOwner;}

		// Access Mechanism Root
		Primitive* getMechRoot();

		// Access Root Moving Primitive
		Primitive* getRootMovingPrimitive();

		// Find if current primitive is Ancestor of Primitive
		bool isAncestorOf(Primitive* prim);

		// Network
		const RBX::SystemAddress getNetworkOwner() const {return networkOwner;}
		void setNetworkOwner(const RBX::SystemAddress value) {networkOwner = value;}

		const NetworkOwnership getNetworkOwnershipRuleInternal() const { return networkOwnershipRule; }
		void setNetworkOwnershipRuleInternal(NetworkOwnership value) { networkOwnershipRule = value; }

		bool getNetworkIsSleeping() const				{return networkIsSleeping;}
		void setNetworkIsSleeping(bool value, Time wakeupNow);

		///////////////////////////////////////////////
		//
		static void onNewOverlap(Primitive* p0, Primitive* p1);
		static void onStopOverlap(Primitive* p0, Primitive* p1);

		///////////////////////////////////////////////
		// Properties
		//
		// Position
		const PV& getPV() const;
		const CoordinateFrame& getCoordinateFrame() const;
		const CoordinateFrame& getCoordinateFrameUnsafe(); // Faster: Thread must hold writer lock.

		void setCoordinateFrame(const CoordinateFrame& cFrame);
		void setPV(const PV& newPv);

		// Velocity
		void setVelocity(const Velocity& vel);
		void zeroVelocity(); // doesn't tickle primitive

		// Mass
		void setMassInertia(float mass);

		// Density/Specific Gravity
		void setSpecificGravity(float value);
		float getSpecificGravity() const			{ return specificGravity; }

		// Dragging
		void setDragging(bool value);
		bool getDragging() const						{return dragging;}

		// Anchored
		void setAnchoredProperty(bool value);
		bool getAnchoredProperty() const				{return anchoredProperty;}

		void updateMassValues(bool physicalPropertiesEnabled);
		float getCalculateMass(bool physicalPropertiesEnabled);

		// EngineType
		void setEngineType(EngineType value);
        EngineType getEngineType() const			{return engineType;}

		// Fixed
		bool requestFixed() const						{return (dragging || anchoredProperty);}
	
		// Collide
		void setPreventCollide(bool _preventCollide);
		bool getPreventCollide() const					{return preventCollide;}

		bool getCanCollide() const						{return !dragging && !preventCollide;}

		// CanThrottle
		void setCanThrottle(bool value);
		bool getCanThrottle() const;

		// PartMaterial
		void setPartMaterial(PartMaterial _material);
		PartMaterial getPartMaterial() const			{ return material; }

		// Friction
		void setFriction(float _friction);
		float getFriction()	const						{return friction;}

		// Elasticity
		void setElasticity(float elasticity);
		float getElasticity() const						{return elasticity;}

		void setPhysicalProperties(const PhysicalProperties& _physProp);
		const PhysicalProperties& getPhysicalProperties() const { return customPhysicalProperties; }

		// Buouyancy
		void onBuoyancyChanged( bool value );

		// Parameters
		void setGeometryParameter(const std::string& parameter, int value);
		int getGeometryParameter(const std::string& parameter) const;

		// Size and Extents - local
		void setSize(const G3D::Vector3& size);
		const Vector3& getSize() const			{return geometry->getSize();}

		virtual float getRadius() const			{return geometry->getRadius();}

		float getPlanarSize() const				{return Math::planarSize(getSize());}

		Extents getExtentsLocal() const {
			Vector3 halfSize = geometry->getSize() * 0.5;
			return Extents(-halfSize, halfSize);
		}

		// World
		Extents getExtentsWorld() const {
			Extents local = getExtentsLocal();
			return local.toWorldSpace(getCoordinateFrame()); 
		}

		const Extents& getFastFuzzyExtentsNoCompute()  {
			RBXASSERT_VERY_FAST(computeFuzzyExtents() == fuzzyExtents);
			return fuzzyExtents;
		}

		const Extents& getFastFuzzyExtents();

		static float squaredDistance(const Primitive& p0, const Primitive& p1)
		{
			return (p0.getCoordinateFrame().translation - p1.getCoordinateFrame().translation).squaredMagnitude();
		}

		static bool aaBoxCollide(Primitive& p0, Primitive& p1)
		{
			return (	Extents::overlapsOrTouches(	p0.getFastFuzzyExtents(), 
													p1.getFastFuzzyExtents())	);
		}
		///////////////////////////////////////////////////////////////

		bool hitTest(const RbxRay& worldRay, Vector3& worldHitPoint, Vector3& surfaceNormal);

		Face getFaceInObject(NormalId objectFace) const;
		Face getFaceInWorld(NormalId objectFace);

		CoordinateFrame getFaceCoordInObject(NormalId objectFace) const;

		void setSurfaceType(NormalId id, SurfaceType newSurfaceType);
		SurfaceType getSurfaceType(NormalId id) const {return surfaceType[id];}

		void setSurfaceData(NormalId id, const SurfaceData& newSurfaceData);
		SurfaceData getSurfaceData(NormalId id) {
			return surfaceData ? surfaceData[id] : SurfaceData::empty();
		}
		const SurfaceData& getConstSurfaceData(NormalId id) const {
			return surfaceData ? surfaceData[id] : SurfaceData::empty();
		}

		bool isGeometryOrthogonal( void ) const;
        
        bool computeIsGrounded( void ) const;
        

		// JointK and Friction and Elasticity
		float getJointK();

		/////////////////////////////////////
		// Global Primitive Stuff
		static float	defaultElasticity()		{return 0.75;}
		static float	defaultFriction()		{return 0.0;}

	private:
		class RigidJoint* getFirstRigidAt(Joint* start);

	public:
		///////////////////////////////////////////////////////////////
		//
		// Creating and breaking joints

		static void insertEdge(Edge* e);
		static void removeEdge(Edge* e);

		bool hasAutoJoints() const;

		bool hasEdge()						{return ((joints.size() >0) || (contacts.size() > 0));}
		int getNumEdges() const				{return joints.size() + contacts.size();}
		Edge* getFirstEdge() const;
		Edge* getNextEdge(Edge* e) const; 

		int getNumJoints() const			{return joints.size();}
		Joint* getFirstJoint();
		Joint* getNextJoint(Joint* prev);
		const Joint* getConstFirstJoint() const;
		const Joint* getConstNextJoint(const Joint* prev) const;
		Joint* getJoint(int id);
		const Joint* getConstJoint(int id) const;
		Primitive* getJointOther(int id)	{return joints.getOther(id);}

		int getNumContacts() const			{return contacts.size();}
		
		Contact* getFirstContact();
		static const bool hasGetFirstContact = true; // this is to simulate __if_exists(getFirstContact) EL
		
		Contact* getNextContact(Contact* prev);
		Contact* getContact(int id);
		Primitive* getContactOther(int id)	{return contacts.getOther(id);}

		RigidJoint* getFirstRigid();
		RigidJoint* getNextRigid(RigidJoint* prev);

		static Joint* getJoint(Primitive* p0, Primitive* p1, int index = 0);
		static Contact* getContact(Primitive* p0, Primitive* p1);

		static Primitive* downstreamPrimitive(Joint* j);

		/////////////////////////////////////////////////////////////////
		//  SpanningNode
	private:
		SpanningEdge* nextSpanningEdgeFromJoint(Joint* j);

		/*override*/ SpanningEdge* getFirstSpanningEdge();
		/*override*/ SpanningEdge* getNextSpanningEdge(SpanningEdge* edge);

	private:
		World*					world;
		Geometry*				geometry;
		Body*					body;
		IMoving*				myOwner;			// forward declared outside of engine

		EdgeList				contacts;
		EdgeList				joints;

		SystemAddress			networkOwner;

		Guid					guid;				// used for tree stuff

		unsigned int			sortSize;			// cached size value used for joint sorting	

		int						worldIndex;			// For fast removal from the world primitives list

		Extents					fuzzyExtents;
		unsigned int			fuzzyExtentsStateId;

		float                   specificGravity;
		float					jointK;

		float					friction;
		float					elasticity;

		boost::flyweight<PhysicalProperties> customPhysicalProperties;

		CompactEnum<PartMaterial, uint16_t> material;

		// FIXED == (anchored || dragging);
		bool					dragging;						// replicated
		bool					anchoredProperty;				// replicated
		bool					preventCollide;					// if dragging -> no collide

		bool					networkIsSleeping;
		bool					jointKDirty;

		CompactEnum<NetworkOwnership, uint8_t> networkOwnershipRule;

		CompactEnum<EngineType, uint8_t> engineType;
		CompactEnum<SizeMultiplier, uint8_t> sizeMultiplier;		// tree stuff - overrides size/guid

		CompactEnum<SurfaceType, uint8_t> surfaceType[6];		// for joints....
		SurfaceData* surfaceData;
	};

}
