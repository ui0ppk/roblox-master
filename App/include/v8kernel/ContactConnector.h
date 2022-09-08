/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Kernel/Connector.h"
#include "V8Kernel/ContactParams.h"
#include "V8Kernel/Pair.h"
#include "V8Kernel/Body.h"
#include "v8kernel/SimBody.h"
#include "v8kernel/Constants.h"
#include "Util/NormalID.h"
#include "rbx/Debug.h"
#include "Util/Memory.h"


namespace RBX {

	//////////////////////////////////////////////////////////////////////////
	class ContactConnector : public Connector
	{
	private:
		static int		inContactHit;
		static int		outOfContactHit;

		int				age;
		// Cache for computeImpulse
		Matrix3 deltaVelPerUnitImpulse;
		Matrix3 impulsePerUnitDeltaVel;
		float	inverseMass;
		float	penetrationVelocity;
		float	reboundVelocity;
		bool	impulseComputed;

	protected:
		GeoPair			geoPair;
		ContactParams	contactParams;
		PairParams		oldContactPoint;
		PairParams		contactPoint;

		// state variables
		float			firstApproach;
		float			threshold;

		// delay variables
		float			forceMagLast;	// contact only variable
		Vector3			frictionOffset;

		/*override*/ virtual KernelType getConnectorKernelType() const { return Connector::CONTACT; }

	public:
		virtual void updateContactPoint();

		static float overlapGoal() {return 0.01f;}		// standard goal seek for overlapping objects

		/*override*/ Body* getBody(BodyIndex id)	{return (id == body0) ? geoPair.body0 : geoPair.body1;}
		void setBody(int id, Body* b) {
			if (id == 0) {
				geoPair.body0 = b;
			}
			else {
				geoPair.body1 = b;
			}
		}

		ContactConnector(Body* b0, Body* b1, const ContactParams& contactParams) 
			: contactParams(contactParams), inverseMass(0.0f), impulseComputed(false)
		{
			geoPair.body0 = b0;
			geoPair.body1 = b1;
			reset();
		}

		void reset() {			// cleans up state variables for buffered version
			firstApproach = 0.0;
			threshold = 0.0;
			forceMagLast = 0.0;
			age = 0;
			penetrationVelocity = 0.0;
			reboundVelocity = 0.0;
		}

		inline void clearImpulseComputed() { impulseComputed = false; }

		bool isIntersecting() {
			RBXASSERT(geoPair.geoPairType == POINT_PLANE_PAIR);
			return (contactPoint.length < -overlapGoal());
		}

		// Reorder the SimBody(s) so that simBody0 is always in kernel and adjust contact point data accordingly
		bool getReordedSimBody(SimBody*& simBody0, SimBody*& simBody1, Body*& bodyNotInKernel, PairParams& params);
		bool getReordedSimBody(SimBody*& simBody0, SimBody*& simBody1, PairParams& params);

		// Compute the relative velocities between the two bodies
		bool getSimBodyAndContactVelocity(SimBody*& simBody0, SimBody*& simBody1, PairParams& params, 
										  float& normalVel, Vector3& perpVel);
		float computeRelativeVelocity(const PairParams &params, Vector3* deltaVnormal, Vector3* perpVel);
		float computeRelativeVelocity();

		void applyContactPointForSymmetryDetection(SimBody* simBody0, SimBody* simBody1,
												   const PairParams& params, float direction);

        const ContactParams& getContactParams() const { return contactParams; }
		void setContactParams(const ContactParams& params) { contactParams = params; }

		/////////// Called by kernel //////////////////////////////
		/* override*/ virtual void computeForce(bool throttling);
		/* override*/ virtual bool computeImpulse(float& residualVelocity);
		/* override*/ bool canThrottle() const;

		// Debug
		static float percentActive();

		float computeOverlap() {			// positive == bigger overlap
			updateContactPoint();
			return -contactPoint.length;
		}

        inline PairParams& getContactPoint() { return contactPoint; }
		inline const PairParams& getContactPoint() const { return contactPoint; }

		void getLengthNormalPosition(Vector3& position, Vector3& normal, float& length) {
			position = contactPoint.position;
			normal = contactPoint.normal;
			length = contactPoint.length;
		}

		inline bool isRestingContact() { return age > 4; }
	};

	class GeoPairConnector 
		: public ContactConnector
		, public Allocator<GeoPairConnector>
	{
	public:
		GeoPairConnector(Body* b0, Body* b1, const ContactParams& contactParams) : ContactConnector(b0, b1, contactParams)
		{}

		//////////////////////////////////////////////////////////////////////////////////

		/*override*/ void updateContactPoint()
		{
			geoPair.computeLengthNormalPosition(contactPoint);
			ContactConnector::updateContactPoint();
		}

		void setPointPlane(const Vector3* oPoint, const Vector3* oPlane, 
			int pointId, NormalId planeId) 
		{
			geoPair.setPointPlane(oPoint, oPlane, pointId, planeId);
		}

		void setEdgeEdgePlane(const Vector3* e0, const Vector3* e1, 
			NormalId n0, NormalId n1, NormalId planeId, float edgeLength0, float edgeLength1) 
		{
			geoPair.setEdgeEdgePlane(e0, e1, n0, n1, planeId, edgeLength0, edgeLength1);
		}

		void setEdgeEdge(const Vector3* e0, const Vector3* e1, NormalId n0, NormalId n1) 
		{
			geoPair.setEdgeEdge(e0, e1, n0, n1);
		}
	
		bool match(Body* b0, Body* b1, GeoPairType pairType, int param0, int param1) 
		{
			return geoPair.match(b0, b1, pairType, param0, param1);
		}
	};
	
	////////////////////////////////////////////////////////////////////////////////

	class BallBallConnector : public ContactConnector
							, public Allocator<BallBallConnector>
	{
	private:
		float radius0;
		float radiusSum;

	public:
		BallBallConnector(Body* b0, Body* b1, const ContactParams& contactParams) 
			: ContactConnector(b0, b1, contactParams)
		{}

		/*override*/ void updateContactPoint();
		void setRadius(float r0, float r1) {
			radius0 = r0;
			radiusSum = r0 + r1;
		}
	};


	////////////////////////////////////////////////////////////////////////////////

	class BallBlockConnector :	public ContactConnector, 
								public Allocator<BallBlockConnector>
	{
	private:
		float radius0;		// ball
		Vector3 offset1;
		NormalId normalId1;
		GeoPairType geoPairType;

		void computeBallPoint(PairParams& params);	
		void computeBallEdge(PairParams& params);	
		void computeBallPlane(PairParams& params);	

	public:
		BallBlockConnector(Body* b0, Body* b1, const ContactParams& contactParams) 
			: ContactConnector(b0, b1, contactParams)
		{}

		/*override*/ void updateContactPoint();
		void setBallBlock(float _radius0, const Vector3* _offset1, RBX::NormalId _normalID, GeoPairType _geoPairType) {
			offset1 = *_offset1;
			radius0 = _radius0;
			normalId1 = _normalID;
			geoPairType = _geoPairType;
		}
	};

} // namespace