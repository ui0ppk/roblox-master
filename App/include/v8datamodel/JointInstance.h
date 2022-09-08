/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Tree/Instance.h"
#include "V8World/Joint.h"
#include "V8World/JointBuilder.h"
#include "GfxBase/IAdornable.h"
#include "V8DataModel/IAnimatableJoint.h"
#include "V8DataModel/PartInstance.h"

/*
	Part* is master, Primitive*'s in the Joint are slaves
*/


namespace RBX {

	class Joint;
	class GlueJoint;
	class WeldJoint;
	class ManualWeldJoint;
	class ManualGlueJoint;
	class JointsService;
	class MotorJoint;
	class Motor6DJoint;

	extern const char *const sJointInstance;
	class JointInstance
		: public DescribedNonCreatable<JointInstance, Instance, sJointInstance>
		, public IAdornable
		, public IJointOwner
	{
	private:
		friend class JointsService;	
		typedef DescribedNonCreatable<JointInstance, Instance, sJointInstance> Super;
		weak_ptr<PartInstance> part[2];

		void setPart(int i, PartInstance* value);

		void setPartNull(int i);
		void setPrimitiveFromPart(int i);

		World* computeWorld();

		void handleWorldChanged();

	protected:
		Joint* joint;

		// Instance
		/*override*/ bool askSetParent(const Instance* instance) const;
		/*override*/ void onAncestorChanged(const AncestorChanged& event);
		
		// IAdornable
		/*override*/ bool shouldRender3dAdorn() const;
		/*override*/ void render3dAdorn(Adorn* adorn);

		JointInstance() {}						// i.e. - if you want to build one, you need a Joint
		JointInstance(Joint* joint);			// Created from Within World - by autojoiner
		~JointInstance();


	public:
		void onCoParentChanged();

		static const Reflection::RefPropDescriptor<JointInstance, PartInstance> prop_Part0;
		static const Reflection::RefPropDescriptor<JointInstance, PartInstance> prop_Part1;

		Joint* getJoint() { return joint; }
		Joint::JointType getJointType() const;

		PartInstance* getPart0();	
		PartInstance* getPart1();	

		PartInstance* getPart0Dangerous() const;	// only for reflection
		PartInstance* getPart1Dangerous() const;	// only for reflection

		void setPart0(PartInstance* value);
		void setPart1(PartInstance* value);	
		
		const CoordinateFrame& getC0() const;
		const CoordinateFrame& getC1() const;
		void setC0(const CoordinateFrame& value);
		void setC1(const CoordinateFrame& value);

		bool isAligned() const;

		bool inEngine();
		
		/*override*/ int getPersistentDataCost() const 
		{
			return Super::getPersistentDataCost() + 4;
		}

		/*override*/ void setName(const std::string& value);

		/*override*/ XmlElement* writeXml(const boost::function<bool(Instance*)>& isInScope, RBX::CreatorRole creatorRole);

	};


	extern const char *const sSnap;
	class Snap
		: public DescribedCreatable<Snap, JointInstance, sSnap>
	{
	public:
		Snap();
		Snap(Joint* joint);
	};

	extern const char *const sWeld;
	class Weld
		: public DescribedCreatable<Weld, JointInstance, sWeld>
	{
	private:
		typedef DescribedCreatable<Weld, JointInstance, sWeld> Super;
		/*override*/ void render3dAdorn(Adorn* adorn);
		const WeldJoint* weldJointConst() const;

	public:
		Weld();
		Weld(Joint* joint);
	};

	extern const char *const sManualSurfaceJointInstance;
	class ManualSurfaceJointInstance
		: public DescribedNonCreatable<ManualSurfaceJointInstance, JointInstance, sManualSurfaceJointInstance>
	{
	private:
		typedef DescribedNonCreatable<ManualSurfaceJointInstance, JointInstance, sManualSurfaceJointInstance> Super;
	protected:
		/*override*/ void render3dAdorn(Adorn* adorn);

	public:
		ManualSurfaceJointInstance(Joint* joint);
		int getSurface0(void) const;
		int getSurface1(void) const;
		void setSurface0(int surfId);
		void setSurface1(int surfId);
	};

	extern const char *const sManualWeld;
	class ManualWeld
		: public DescribedCreatable<ManualWeld, ManualSurfaceJointInstance, sManualWeld>
	{
	private:
		typedef DescribedCreatable<ManualWeld, ManualSurfaceJointInstance, sManualWeld> Super;
		/*override*/ void render3dAdorn(Adorn* adorn);
        /*override*/ bool shouldRender3dAdorn() const { return false; }
		const ManualWeldJoint* manualWeldJointConst() const;

	public:
		ManualWeld();
		ManualWeld(Joint* joint);
	};

	extern const char *const sManualGlue;
	class ManualGlue
		: public DescribedCreatable<ManualGlue, ManualSurfaceJointInstance, sManualGlue>
	{
	private:
		typedef DescribedCreatable<ManualGlue, ManualSurfaceJointInstance, sManualGlue> Super;
		/*override*/ void render3dAdorn(Adorn* adorn);
        /*override*/ bool shouldRender3dAdorn() const { return true; }
		const ManualGlueJoint* manualGlueJointConst() const;

	public:
		ManualGlue();
		ManualGlue(Joint* joint);
	};

	///////////////////////////////////////////////////////
	//

	extern const char *const sGlue;
	class Glue
		: public DescribedCreatable<Glue, JointInstance, sGlue>
	{
	private:
		GlueJoint* glueJoint();
		const GlueJoint* glueJointConst() const;

	public:
		Glue();
		Glue(Joint* joint);

		const Vector3& getF0() const;
		const Vector3& getF1() const;
		const Vector3& getF2() const;
		const Vector3& getF3() const;
		void setF0(const Vector3& value);
		void setF1(const Vector3& value);
		void setF2(const Vector3& value);
		void setF3(const Vector3& value);
	};

	///////////////////////////////////////////////////////
	//

	extern const char *const sRotate;
	class Rotate
		: public DescribedCreatable<Rotate, JointInstance, sRotate>
	{
	public:
		Rotate();
		Rotate(Joint* joint);
	};

	///////////////////////////////////////////////////////
	//

	extern const char *const sDynamicRotate;
	class DynamicRotate	: public DescribedNonCreatable<DynamicRotate, JointInstance, sDynamicRotate>
	{
	protected:
		DynamicRotate();
		DynamicRotate(Joint* joint);

	public:
		float getBaseAngle() const;
		void setBaseAngle(float value);
	};


	extern const char *const sRotateP;
	class RotateP
		: public DescribedCreatable<RotateP, DynamicRotate, sRotateP>
	{
	public:
		RotateP();
		RotateP(Joint* joint);
	};

	extern const char *const sRotateV;
	class RotateV
		: public DescribedCreatable<RotateV, DynamicRotate, sRotateV>
	{
	public:
		RotateV();
		RotateV(Joint* joint);
	};

	///////////////////////////////////////////////////////
	//

	extern const char *const sMotor;
	class Motor
		: public DescribedCreatable<Motor, JointInstance, sMotor>
		, public IAnimatableJoint
	{
	private:
		MotorJoint* getMotorJoint();
		const MotorJoint* getMotorJoint() const;
		void renderJointCylinder(Adorn* adorn, int i);
	protected:
		Motor(Joint* joint, int); // special constructor for Motor6D
	public:
		Motor();
		Motor(Joint* joint);

		virtual float getMaxVelocity() const;
		virtual void setMaxVelocity(float value);

		virtual float getDesiredAngle() const;
		virtual void setDesiredAngle(float value);
		virtual void setDesiredAngleUi(float value);		// non replicating version

		virtual float getCurrentAngle() const;
		virtual void setCurrentAngleUi(float value);		// current angle doesn't replicate - it is a command.  Physics for the joint angle replicates
		
		/*IAnimatableJoint*/
		/*implement*/ const std::string& getParentName();
		/*implement*/ const std::string& getPartName();
		/*implement*/ void applyPose(const CachedPose& cframe);
        /*implement*/ virtual void setIsAnimatedJoint(bool val);
	};

	extern const char *const sMotor6D;
	class Motor6D
		: public DescribedCreatable<Motor6D, Motor, sMotor6D> // derive from Motor, so that and script doing a ::IsA(Motor) should still work.
	{
	private:
		Motor6DJoint* getMotorJoint();
		const Motor6DJoint* getMotorJoint() const;
		void renderJointCylinder(Adorn* adorn, int i);
	public:
		Motor6D();
		Motor6D(Joint* joint);

		/*override*/ float getMaxVelocity() const;
		/*override*/ void setMaxVelocity(float value);

		/*override*/ float getDesiredAngle() const;
		/*override*/ void setDesiredAngle(float value);
		/*override*/ void setDesiredAngleUi(float value);		// non replicating version

		/*override*/ float getCurrentAngle() const;
		/*override*/ void setCurrentAngleUi(float value);		// current angle doesn't replicate - it is a command.  Physics for the joint angle replicates
		
		/*IAnimatableJoint*/
		/*override*/ void applyPose(const CachedPose& cframe);
	};

} // namespace
