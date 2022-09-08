/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/RightAngleRampInstance.h"
#include "V8World/RightAngleRampPoly.h"
#include "V8World/Primitive.h"

#ifdef _PRISM_PYRAMID_

namespace RBX
{
const char* const  sRightAngleRamp = "RightAngleRampPart";

using namespace Reflection;

const char* category_RightAngleRamp = "Part ";
static const Vector3 InitialRightAngleRampPartSize = Vector3(4.0, 4.0, 2.0);

RightAngleRampInstance::RightAngleRampInstance() : DescribedNonCreatable<RightAngleRampInstance, PartInstance, sRightAngleRamp>(InitialRightAngleRampPartSize)
{
	setName("RightAngleRamp");
	Primitive* myPrim = this->getPartPrimitive();
	myPrim->setGeometryType(Geometry::GEOMETRY_RIGHTANGLERAMP);
	myPrim->setSurfaceType(NORM_X_NEG, UNIVERSAL);
	myPrim->setSurfaceType(NORM_Y_NEG, UNIVERSAL);
	myPrim->setSurfaceType(NORM_Y, NO_SURFACE);

	shouldRenderSetDirty();
}

RightAngleRampInstance::~RightAngleRampInstance()
{
}


}//namespace

#endif // _PRISM_PYRAMID_
