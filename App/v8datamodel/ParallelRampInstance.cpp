/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/ParallelRampInstance.h"
#include "V8World/ParallelRampPoly.h"
#include "V8World/Primitive.h"

#ifdef _PRISM_PYRAMID_

namespace RBX
{
const char* const sParallelRamp = "ParallelRampPart";

using namespace Reflection;

const char* category_ParallelRamp = "Part ";
static const Vector3 InitialParallelRampPartSize = Vector3(4.0, 4.0, 2.0);

ParallelRampInstance::ParallelRampInstance() : DescribedNonCreatable<ParallelRampInstance, PartInstance, sParallelRamp>(InitialParallelRampPartSize)
{
	setName("ParallelRamp");
	Primitive* myPrim = this->getPartPrimitive();
	myPrim->setGeometryType(Geometry::GEOMETRY_PARALLELRAMP);
	myPrim->setSurfaceType(NORM_X, UNIVERSAL);
	myPrim->setSurfaceType(NORM_X_NEG, UNIVERSAL);
	myPrim->setSurfaceType(NORM_Y, NO_SURFACE);
	myPrim->setSurfaceType(NORM_Y_NEG, NO_SURFACE);

	shouldRenderSetDirty();
}

ParallelRampInstance::~ParallelRampInstance()
{
}


}//namespace

#endif // _PRISM_PYRAMID_