/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/PyramidInstance.h"
#include "V8World/PyramidPoly.h"
#include "V8World/Primitive.h"

#ifdef _PRISM_PYRAMID_

namespace RBX
{
const char* const  sPyramid = "PyramidPart";

using namespace Reflection;

const char* category_Pyramid = "Part ";
const EnumPropDescriptor<PyramidInstance, PyramidInstance::NumSidesEnum> PyramidInstance::prop_sidesXML("sides", category_Pyramid, &PyramidInstance::GetNumSides, &PyramidInstance::SetNumSides, PropertyDescriptor::STREAMING);
static const EnumPropDescriptor<PyramidInstance, PyramidInstance::NumSidesEnum> prop_sidesUI("Sides", category_Pyramid, &PyramidInstance::GetNumSides, &PyramidInstance::SetNumSides, PropertyDescriptor::UI);
static const Vector3 InitialPyramidPartSize = Vector3(4.0, 4.0, 4.0);

// Disable slice control for now.
//const PropDescriptor<PyramidInstance, int> PyramidInstance::prop_slices("Slices", category_Pyramid, &PyramidInstance::GetNumSlices, &PyramidInstance::SetNumSlices);


PyramidInstance::PyramidInstance() : DescribedNonCreatable<PyramidInstance, PartInstance, sPyramid>(InitialPyramidPartSize)
{
	setName("Pyramid");
	Primitive* myPrim = this->getPartPrimitive();
	myPrim->setGeometryType(Geometry::GEOMETRY_PYRAMID);
	myPrim->setGeometryParameter("NumSides", 8);
	myPrim->setGeometryParameter("NumSlices", 8);
	myPrim->setSurfaceType(NORM_Y, NO_SURFACE);
}

PyramidInstance::~PyramidInstance()
{
}


PyramidInstance::NumSidesEnum PyramidInstance::GetNumSides() const
{
	int num = getConstPartPrimitive()->getGeometryParameter("NumSides");
	return static_cast<NumSidesEnum>(num);
}

int PyramidInstance::GetNumSlices() const
{
	return getConstPartPrimitive()->getGeometryParameter("NumSides");
}


void PyramidInstance::SetNumSides(PyramidInstance::NumSidesEnum num)
{
	// disable slices control from UI.  set slices = sides
	int NumSides = num; 
	raisePropertyChanged(prop_sidesXML); 
	raisePropertyChanged(prop_sidesUI);

	getPartPrimitive()->setGeometryParameter("NumSides", NumSides);
	getPartPrimitive()->setGeometryParameter("NumSlices", NumSides);

	shouldRenderSetDirty();
}

void PyramidInstance::SetNumSlices(int num)
{
	getPartPrimitive()->setGeometryParameter("NumSlices", num);

	shouldRenderSetDirty();
}


void PyramidInstance::setPartSizeXml(const Vector3& rbxSize)
{
	Vector3 newSize = rbxSize;
	Vector3 currentSize = getPartSizeXml();

	if( rbxSize.x != currentSize.x )
		newSize.x = newSize.z = rbxSize.x;
	else if( rbxSize.z != currentSize.z )
		newSize.z = newSize.x = rbxSize.z;

	if(newSize != getPartSizeXml())
	{
		Super::setPartSizeXml(newSize);
	}
	else
	{
		refreshPartSizeUi();
	}
}

}//namespace

#endif // _PRISM_PYRAMID_
