/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Tree/Instance.h"
#include "V8DataModel/JointInstance.h"
#include "GfxBase/IAdornable.h"
#include "Util/NormalId.h"


namespace RBX {

	class PartInstance;
	class MotorJoint;

	extern const char *const sFeature;
	class Feature
		: public DescribedNonCreatable<Feature, Instance, sFeature>
		, public IAdornable
	{
	public:		// ouch - for properties
		enum TopBottom {TOP, CENTER_TB, BOTTOM};
		enum LeftRight {LEFT, CENTER_LR, RIGHT};
		enum InOut {EDGE, INSET, CENTER_IO};
		typedef NormalId FaceId;

		FaceId			faceId;
		TopBottom		topBottom;
		LeftRight		leftRight;
		InOut			inOut;
		
	private:
		// Instance
		/*override*/ bool askSetParent(const Instance* instance) const {return true;}

		// IAdornable
		/*override*/ bool shouldRender3dAdorn() const {return true;}
		/*override*/ void render3dSelect(Adorn* adorn, SelectState selectState);

	protected:
		bool getRenderCoord(CoordinateFrame& c) const;

		enum InOutZ {Z_IN, Z_OUT};

		virtual InOutZ getCoordOrientation() const {return Z_OUT;}

	public:
		Feature();
		~Feature();
	
		FaceId getFaceId() const		{return faceId;}
		void setFaceId(NormalId value);

		TopBottom getTopBottom() const	{return topBottom;}
		void setTopBottom(TopBottom value);

		LeftRight getLeftRight() const	{return leftRight;}
		void setLeftRight(LeftRight value);

		InOut getInOut() const			{return inOut;}
		void setInOut(InOut value);

		CoordinateFrame computeLocalCoordinateFrame() const;
	};


	extern const char *const sMotorFeature;
	class MotorFeature
		: public DescribedCreatable<MotorFeature, Feature, sMotorFeature>
	{
	private:
		// Feature
		/*override*/ void otherFeatureChanged() {}

		// IAdornable
		/*override*/ void render3dAdorn(Adorn* adorn);

	public:
		MotorFeature();

		static bool canJoin(Instance* i0, Instance* i1);
		static void join(Instance* i0, Instance* i1);
	};


	extern const char *const sHole;
	class Hole
		: public DescribedCreatable<Hole, Feature, sHole>
	{
	private:
		// IAdornable
		/*override*/ void render3dAdorn(Adorn* adorn);

		// Feature
		/*override*/ InOutZ getCoordOrientation() const {return Z_IN;}

	public:
		Hole();
	};


	extern const char *const sVelocityMotor;
	class VelocityMotor
		: public DescribedCreatable<VelocityMotor, JointInstance, sVelocityMotor>
	{	
	private:
		typedef DescribedCreatable<VelocityMotor, JointInstance, sVelocityMotor> Super;
		MotorJoint*	motorJoint();
		const MotorJoint* motorJoint() const;

		shared_ptr<Hole> hole;

		rbx::signals::scoped_connection holeAncestorChanged;		
		void onEvent_HoleAncestorChanged();

		void setPart(int i, Feature* feature);

		/*override*/ bool askSetParent(const Instance* instance) const {return true;}

		/*override*/ void onAncestorChanged(const AncestorChanged& event);

	public:
		VelocityMotor();
		~VelocityMotor();

		Hole* getHole() const;	
		void setHole(Hole* value);

		float getMaxVelocity() const;
		void setMaxVelocity(float value);

		float getDesiredAngle() const;
		void setDesiredAngle(float value);

		float getCurrentAngle() const;
		void setCurrentAngle(float value);
	};
} // namespace