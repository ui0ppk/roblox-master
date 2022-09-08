/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/G3DCore.h"
#include "rbx/Declarations.h"
#include "Util/Memory.h"
#include "Util/Math.h"

namespace RBX {

	class Body;

	class RBXBaseClass Link
	{
		friend class Body;

	protected:
		Body*						body;			// body I'm affilliated with (child)
		CoordinateFrame				parentCoord;
		CoordinateFrame				childCoord;
		CoordinateFrame				childCoordInverse;

		CoordinateFrame				childInParent;
		unsigned int							stateIndex;

		virtual void computeChildInParent(CoordinateFrame& answer) const = 0;
		
		void dirty();

		void setBody(Body* b)	{body = b;}

	public:
		Link();

		~Link();

		const CoordinateFrame& getChildInParent();

		Body* getBody() const {return body;}

		void reset(
			const CoordinateFrame& parentC,
			const CoordinateFrame& childC);
	};


	class RevoluteLink 
		: public Link
		, public Allocator<RevoluteLink>
	{
	private:
		float					jointAngle;

		/*override*/ void computeChildInParent(CoordinateFrame& answer) const;

	public:
		RevoluteLink() : jointAngle(0.0f) 
		{
		}

		void setJointAngle(float value) {
			jointAngle = value;
			dirty();
		}
	};

	class D6Link 
		: public Link
		, public Allocator<D6Link>
	{
	private:
		CoordinateFrame					offsetCFrame;

		/*override*/ void computeChildInParent(CoordinateFrame& answer) const;

	public:
		void setJointOffsetCFrame(const CoordinateFrame& value) {
			offsetCFrame = value;
			RBXASSERT(!Math::hasNanOrInf(value));
			dirty();
		}
	};

} // namespace

