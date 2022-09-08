/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8World/Edge.h"
#include "V8World/Feature.h"
#include "V8Kernel/ContactParams.h"
#include "Util/G3DCore.h"
#include "Util/Memory.h"
#include "Util/NormalId.h"
#include "Util/Math.h"
#include "Util/FixedArray.h"
#include "rbx/Debug.h"
#include "rbx/Declarations.h"

#define CONTACT_ARRAY_SIZE 40
#define BULLET_CONTACT_ARRAY_SIZE 40

namespace RBX {

	class Kernel;
	class CollisionStage;
	class ContactConnector;
	class GeoPairConnector;
	class BallBlockConnector;
	class BallBallConnector;
	class Body;
	class Ball;
	class Block;
    class BlockBlockContactData; 

	class RBXBaseClass Contact : public Edge
	{
	private:
		typedef Edge Super;

		friend class CollisionStage;
	

		static bool			ignoreBool;

		// CollisionStage
		int					lastUiContactStep;
		int					steppingIndex;			// For fast removal from the collision stage stepping list
		short				numTouchCycles;

		/////////////////////////////////////////////////////
		//
		// Edge

		/*override*/ void putInKernel(Kernel* _kernel) {
			Super::putInKernel(_kernel);
		}

		/*override*/ void removeFromKernel() {
			RBXASSERT(getKernel());
			deleteAllConnectors();
			Super::removeFromKernel();
		}

		///////////////////////////////////////////////////
		// Edge Virtuals
		//
		/*override*/ virtual EdgeType getEdgeType() const {return Edge::CONTACT;}

	protected:
        ContactParams* contactParams;
		Body* getBody(int i);

		/////////////////////////////////////////////////////
		//
		// ContactPairData management

		void deleteConnector(ContactConnector* c);
		
		virtual void deleteAllConnectors() = 0;		// everyone implements this

		virtual	bool stepContact() = 0;	

	public:
        enum ContactType
		{
            Contact_Simple,
            Contact_Cell,
            Contact_Buoyancy,
		};

		Contact(Primitive* p0, Primitive* p1);

		virtual ~Contact();

		short getNumTouchCycles() {return numTouchCycles;}

		int& steppingIndexFunc() {return steppingIndex;}		// fast removal from stepping list

		// Proximite tests - compute
		typedef bool (Contact::*ProximityTest)(float);

		bool computeIsAdjacentUi(float spaceAllowed);

		virtual bool computeIsCollidingUi(float overlapIgnored); 

		virtual bool computeIsColliding(float overlapIgnored) = 0; 

		static bool isContact(Edge* e) {return (e->getEdgeType() == Edge::CONTACT);}

		/////////////////////////////////////////////////////
		//
		// From The Contact Manager
		virtual void onPrimitiveContactParametersChanged();

		bool step(int uiStepId);

		virtual int numConnectors() const = 0;
		
		virtual ContactConnector* getConnector(int i) = 0;

		void primitiveMovedExternally();

        virtual void generateDataForMovingAssemblyStage(void);

		virtual void invalidateContactCache();

        ContactParams* getContactParams(void) { return contactParams; }

		bool isInContact() { return lastUiContactStep > 0; }
        
		virtual ContactType getContactType() const { return Contact_Simple; }
	};

	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////

	class BallBallContact 
		: public Contact 
		, public Allocator<BallBallContact>
	{
	private:
		BallBallConnector* ballBallConnector;

		Ball* ball(int i);

		/*override*/ void deleteAllConnectors();
		/*override*/ bool computeIsColliding(float overlapIgnored); 
		/*override*/ bool stepContact();

		/*override*/  int numConnectors() const {return ballBallConnector ? 1 : 0;}
		/*override*/  ContactConnector* getConnector(int i);

	public:
		BallBallContact(Primitive* p0, Primitive* p1)
			: Contact(p0, p1)
			, ballBallConnector(NULL)
		{}

		~BallBallContact() {RBXASSERT(!ballBallConnector);}

        void generateDataForMovingAssemblyStage(void); /*override*/
	};

	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////
	class BallBlockContact 
		: public Contact 
		, public Allocator<BallBlockContact>
	{
	private:
		BallBlockConnector* ballBlockConnector;

		Primitive* ballPrim();
		Primitive* blockPrim();

		Ball* ball();
		Block* block();

		bool computeIsColliding(
			int& onBorder, 
			Vector3int16& clip, 
			Vector3& projectionInBlock, 
			float overlapIgnored);	

		/*override*/ void deleteAllConnectors();
		/*override*/ bool computeIsColliding(float overlapIgnored); 
		/*override*/ bool stepContact();

		/*override*/  int numConnectors() const {return ballBlockConnector ? 1 : 0;}
		/*override*/  ContactConnector* getConnector(int i);

	public:
		BallBlockContact(Primitive* p0, Primitive* p1)
			: Contact(p0, p1)
			, ballBlockConnector(NULL)
		{}

		~BallBlockContact() {RBXASSERT(!ballBlockConnector);}

        void generateDataForMovingAssemblyStage(void); /*override*/
	};

	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////

	class BlockBlockContact 
		: public Contact
		, public Allocator<BlockBlockContact>
	{
	private:
        static int pairMatches;
		static int pairMisses;
		static int featureMatches;
		static int featureMisses;

        typedef RBX::FixedArray<GeoPairConnector*, 8> ConnectorArray;

        friend class BlockBlockContactData;
        BlockBlockContactData* myData;

        Block* block(int i);

		GeoPairConnector* findGeoPairConnector(
			Body* b0, 
			Body* b1, 
			GeoPairType _pairType, 
			int param0, 
			int param1);

		void loadGeoPairEdgeEdge(
			int b0, 
			int b1, 
			int edge0, 
			int edge1);

		void loadGeoPairPointPlane(
			int pointBody, 
			int planeBody, int pointID, 
			NormalId pointFaceID, 
			NormalId planeFaceID);

		// plane contact - first compute the feature0, feature1
		bool getBestPlaneEdge(float overlapIgnored, bool& planeContact);

		int intersectRectQuad(Vector2& planeRect, Vector2 (&otherQuad)[4]);

		bool computeIsColliding(float overlapIgnored, bool& planeContact); 

		////////////////////////////////////////////////////
		// Contact
		/*override*/ void deleteAllConnectors(void);
		/*override*/ bool computeIsColliding(float overlapIgnored); 
		/*override*/ bool stepContact();

		/*override*/  int numConnectors() const;
		/*override*/  ContactConnector* getConnector(int i);

	public:
		BlockBlockContact(Primitive* p0, Primitive* p1);
        ~BlockBlockContact();

		static float pairHitRatio();
		static float featureHitRatio();

        void generateDataForMovingAssemblyStage(void); /*override*/

    private:
		inline static void boxProjection(const Vector3& normal0, const Matrix3& R1, const Vector3& extent1, float& projectedExtent) 
		{
			Vector3 temp = Math::vectorToObjectSpace(normal0, R1);
			projectedExtent =		std::abs(	extent1[0] * temp[0]	)
								+	std::abs(	extent1[1] * temp[1]	)
								+	std::abs(	extent1[2] * temp[2]	);
		}

		// if length < 0, no overlap, and no block/block contact
		// proj0 >0 on entry
		// proj1 >0 on entry

		inline static bool updateBestAxis(float proj0, float p0p1, float proj1, float& _overlap, float overlapIgnored) 
		{
			// no overlap along this axis - bail, not in contact
			_overlap =  proj0 + proj1 - std::abs(p0p1);
			return (_overlap > overlapIgnored);
		}

		bool geoFeaturesOverlap(
								int pointBody,
								int planeBody, 
								int pointID, 
								NormalId pointFaceID, 
								NormalId planeFaceID);
	};

    class BlockBlockContactData
    {
	friend class BlockBlockContact;

	private:
		BlockBlockContact::ConnectorArray connectors[2];
		int connectorsIndex;

		// for hysteresis:
		int				witnessId;
		int				separatingAxisId;

		int				feature[2];	// -1 no feature, 0..5 plane, 6..8 edge Normal, 
		int				bPlane;
		int				bOther;
		RBX::NormalId	planeID;
		RBX::NormalId	otherPlaneID;

        BlockBlockContact* myOwner;

    public:
        BlockBlockContactData(BlockBlockContact* owner);
        ~BlockBlockContactData() {}

		int numConnectors() const { return connectors[connectorsIndex].size(); }
        ContactConnector* getConnector( int i );
        void clearConnectors( void );
        GeoPairConnector* findGeoPairConnector(	Body* b0, Body* b1, GeoPairType _pairType, int param0, int param1 );
        bool stepContact();
        void loadGeoPairEdgeEdgePlane( int edgeBody, int planeBody, int edge0, int edge1 );
        bool getBestPlaneEdge(float overlapIgnored, bool& planeContact);
        int computePlaneContact(void);
        int intersectRectQuad(Vector2& planeRect, Vector2 (&otherQuad)[4]);
    };

} // namespace
