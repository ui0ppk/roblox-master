#pragma once

#include "V8World/Edge.h"
#include "Util/SurfaceType.h"
#include "Util/Face.h"
#include "Util/Extents.h"
#include "Util/SpanningEdge.h"
#include "G3D/Array.h"
#include "boost/intrusive/list.hpp"


namespace RBX {
	
	class Channel;
	class Link;
	class Joint;

	class RBXInterface IJointOwner
	{
	public:
		virtual Joint* getJoint(void) { RBXASSERT(0); return NULL; }
	};

	class StepJointsStage;
	typedef boost::intrusive::list_base_hook< boost::intrusive::tag<StepJointsStage> > StepJointsStageHook;
	class MovingAssemblyStage;
	typedef boost::intrusive::list_base_hook< boost::intrusive::tag<MovingAssemblyStage> > MovingAssemblyStageHook;

	class Joint : public Edge
				, public SpanningEdge
				, public StepJointsStageHook
				, public MovingAssemblyStageHook	// TODO: Can we share the same hooks?
	{
	private:
		typedef Edge Super;
		IJointOwner* jointOwner;

		static bool canBuildJoint(
								Primitive* p0, 
								Primitive* p1, 
								NormalId nId0, 
								NormalId nId1,
								float angleMax,
								float planarMax);

	protected:
		CoordinateFrame jointCoord0;	// in object space
		CoordinateFrame jointCoord1;	// this coord is aligned with Coord0, so it points into the body
	public:
		static bool canBuildJointLoose(Primitive* p0, Primitive* p1, NormalId nId0, NormalId nId1);
		static bool canBuildJointTight(Primitive* p0, Primitive* p1, NormalId nId0, NormalId nId1);

protected:
		///////////////////////////////////////////////////
		// Edge
		//
		/*override*/ EdgeType getEdgeType() const {return Edge::JOINT;}

		Joint();

		Joint(	Primitive* prim0, 
				Primitive* prim1, 
				const CoordinateFrame& _jointCoord0, 
				const CoordinateFrame& _jointCoord1); 

	public:
		~Joint();

		void setJointCoord(int i, const CoordinateFrame& c);

		const CoordinateFrame& getJointCoord(int i) const {
			return (i == 0) ? jointCoord0 : jointCoord1;
		}

		CoordinateFrame getJointWorldCoord(int i);

		void notifyMoved();

		// In precedence order - greatest to least
											// GROUND		KINEMATIC		SPRING		KERNEL
		typedef enum {	ANCHOR_JOINT,		//	X
						WELD_JOINT,			//					X			
						MANUAL_WELD_JOINT,	//					X			
						SNAP_JOINT,			//					X
						MOTOR_1D_JOINT,		//					X
						MOTOR_6D_JOINT,		//					X
						ROTATE_JOINT,		//									X
						ROTATE_P_JOINT,		//									X
						ROTATE_V_JOINT,		//									X
						GLUE_JOINT,			//									X
						MANUAL_GLUE_JOINT,	//									X
						FREE_JOINT,			//	X
						KERNEL_JOINT,		//											X
						NO_JOINT			//		
									} JointType;	
						
		//////////////////////////////////////////////////
		// IJointOwner
		//
		void setJointOwner(IJointOwner* value);
		IJointOwner* getJointOwner() const;

		///////////////////////////////////////////////////
		// Edge Virtuals
		//
		/*override*/ void setPrimitive(int i, Primitive* p);

		///////////////////////////////////////////////////
		// Joint Virtuals
		//
		virtual JointType getJointType() const	{RBXASSERT(0); return Joint::NO_JOINT;}
		virtual bool isBreakable() const {return false;}
		virtual bool isBroken() const {return false;}
		virtual bool joinsFace(Primitive* g, NormalId faceId) const {return false;}
		virtual bool isAligned() {return true;}
		virtual CoordinateFrame align(Primitive* pMove, Primitive* pStay) {RBXASSERT(0); return CoordinateFrame();}

		virtual void setPhysics() {}		// occurs after networking read;

		virtual bool canStepWorld() const {return false;}
		virtual bool canStepUi() const {return false;}

		virtual void stepWorld() {}
		virtual bool stepUi(double distributedGameTime) {return false;}

		//////////////////////////////////////////////////////////
		//
		static bool isJoint(const Edge* e) {return (e->getEdgeType() == Edge::JOINT);}

		static JointType getJointType(const Edge* e) {
			return isJoint(e) ? rbx_static_cast<const Joint*>(e)->getJointType() : Joint::NO_JOINT;
		}

		static bool isGroundJoint(const Edge* e) {			// alternately, created by AutoJoin
			Joint::JointType jt = getJointType(e);
			return ((jt == FREE_JOINT) || (jt == ANCHOR_JOINT));
		}

		static bool isRigidJoint(const Edge* e) {			
			Joint::JointType jt = getJointType(e);
			return ((jt == WELD_JOINT) || (jt == SNAP_JOINT) || (jt == MANUAL_WELD_JOINT));
		}

		static bool isKinematicJoint(const Edge* e) {
			Joint::JointType jt = getJointType(e);
			return ((jt >= WELD_JOINT) && (jt <= MOTOR_6D_JOINT));
		}

		static bool isSpringJoint(const Edge* e) {
			Joint::JointType jt = getJointType(e);
			return ((jt >= ROTATE_JOINT) && (jt <= GLUE_JOINT)) || (jt == MANUAL_GLUE_JOINT);
		}

		static bool isMotorJoint(const Edge* e) {
			Joint::JointType jt = getJointType(e);
			return ((jt >= MOTOR_1D_JOINT) && (jt <= MOTOR_6D_JOINT));
		}

		static bool isKernelJoint(const Edge* e) {
			Joint::JointType jt = getJointType(e);
			return (jt == Joint::KERNEL_JOINT);
		}

		static bool isManualJoint(const Edge* e) {
			Joint::JointType jt = getJointType(e);
			return (jt == Joint::MANUAL_WELD_JOINT || jt == Joint::MANUAL_GLUE_JOINT);
		}

		static bool isSpanningTreeJoint(const Edge* e) {
			return (isKinematicJoint(e) || isSpringJoint(e) || isGroundJoint(e));
		}

		static bool isAutoJoint(const Joint* j) {			// alternately, created by AutoJoin
			return (!isGroundJoint(j) && !isKernelJoint(j) && !isManualJoint(j));
		}

		static Joint* getJoint(Primitive* p, Joint::JointType jointType);
		static const Joint* getConstJoint(const Primitive* p, Joint::JointType jointType);
		static const Joint* findConstJoint(const Primitive* p, Joint::JointType jointType);

		NormalId getNormalId(int i) const {		 
			RBXASSERT((i==0)||(i==1));
			return (i == 0) 
				? Matrix3ToNormalId(jointCoord0.rotation)
				: normalIdOpposite(Matrix3ToNormalId(jointCoord1.rotation));
		}

		virtual Link* resetLink() { RBXASSERT(!"Not Implemented"); return 0; }

		static bool FacesOverlapped( const Primitive* p0, size_t face0Id, const Primitive* p1, size_t face1Id, float adjustPartTolerance = 1.0 );
		static bool FaceVerticesOverlapped( const Primitive* p0, size_t face0Id, const Primitive* p1, size_t face1Id, float adjustPartTolerance );
		static bool FaceEdgesOverlapped( const Primitive* p0, size_t face0Id, const Primitive* p1, size_t face1Id, float adjustPartTolerance );
		static bool findTouchingSurfacesConvex( const Primitive& p0, size_t& face0Id, const Primitive& p1, size_t& face1Id );
		static bool compatibleForGlueAutoJoint( const Primitive& p0, size_t& face0Id, const Primitive& p1, size_t& face1Id );
		static bool compatibleForWeldAutoJoint( const Primitive& p0, size_t& face0Id, const Primitive& p1, size_t& face1Id );
		static bool compatibleForHingeAutoJoint( const Primitive& p0, size_t& face0Id, const Primitive& p1, size_t& face1Id );
		static bool compatibleForStudAutoJoint( const Primitive& p0, size_t& face0Id, const Primitive& p1, size_t& face1Id );
		static bool inCompatibleForAnyJoint( const Primitive& p0, size_t& face0Id, const Primitive& p1, size_t& face1Id );
		static bool positionedForStudAutoJoint( const Primitive& p0, size_t& face0Id, const Primitive& p1, size_t& face1Id );
		static SurfaceType getSurfaceTypeFromNormal( const Primitive& primitive, const NormalId& normalId ); // helper function for correct "isCompatible" function behavior

		/////////////////////////////////////////////////////////////////
		//  SpanningEdge
	private:
		/*override*/ bool isHeavierThan(const SpanningEdge* other) const;
		/*override*/ SpanningNode* otherNode(SpanningNode* n);
		/*override*/ const SpanningNode* otherConstNode(const SpanningNode* n) const;
		/*override*/ SpanningNode* getNode(int i);
		/*override*/ const SpanningNode* getConstNode(int i) const;
	};



	class AnchorJoint : public Joint
	{
	private:
		/*override*/ virtual JointType getJointType() const	{return Joint::ANCHOR_JOINT;}

	public:
		AnchorJoint(Primitive* prim) : Joint(prim, NULL, CoordinateFrame(), CoordinateFrame())
		{}

		static bool isAnchorJoint(const Joint* j) {
			return (j->getJointType() == Joint::ANCHOR_JOINT);
		}
	};

	class FreeJoint : public Joint
	{
	private:
		/*override*/ virtual JointType getJointType() const	{return Joint::FREE_JOINT;}

	public:
		FreeJoint(Primitive* prim) : Joint(prim, NULL, CoordinateFrame(), CoordinateFrame())
		{}

		static bool isFreeJoint(const Joint* j) {
			return (j->getJointType() == Joint::FREE_JOINT);
		}
	};

} // namespace

