/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/ParametricPartInstance.h"
#include "V8World/Primitive.h"
#include "V8World/Poly.h"

namespace RBX
{
namespace PART
{

const char* const sWedge = "WedgePart";

ParametricPartInstance::ParametricPartInstance()
{
}

ParametricPartInstance::~ParametricPartInstance()
{
}

Wedge::Wedge()
{
	setName("Wedge");
	Primitive* myPrim = this->getPartPrimitive();
	myPrim->setGeometryType(Geometry::GEOMETRY_WEDGE);
	myPrim->setSurfaceType(NORM_Y, NO_SURFACE);
}

Wedge::~Wedge()
{
}


}} //namespace