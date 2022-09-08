#pragma once

// #include "V8World/Joint.h"

namespace RBX {

	class Joint;
	class Primitive;

	class JointBuilder
	{
	public:
//		static Joint* makeJoint(Primitive* p0, Primitive* p1, const CoordinateFrame& c0, const CoordinateFrame& c1, Joint::JointType jointType);

		static Joint* canJoin(Primitive* p0, Primitive* p1);
	};

} // namespace

