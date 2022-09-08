/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/ExtrudedPartInstance.h"
#include "V8World/Primitive.h"

namespace RBX
{
const char* const sExtrudedPart = "TrussPart";

using namespace Reflection;

const char* category_ExtrudedPart = "Part ";
const EnumPropDescriptor<ExtrudedPartInstance, ExtrudedPartInstance::VisualTrussStyle> ExtrudedPartInstance::prop_styleXml("style", category_ExtrudedPart, &ExtrudedPartInstance::getVisualTrussStyle, &ExtrudedPartInstance::setVisualTrussStyle, PropertyDescriptor::STREAMING);

static const EnumPropDescriptor<ExtrudedPartInstance, ExtrudedPartInstance::VisualTrussStyle> prop_styleUi("Style", category_ExtrudedPart , &ExtrudedPartInstance::getVisualTrussStyle, &ExtrudedPartInstance::setVisualTrussStyle, PropertyDescriptor::UI);


static const int MinExtrudedDimensionSize = 2;
static const Vector3 MinExtrudedPartSize = Vector3(float(MinExtrudedDimensionSize), float(MinExtrudedDimensionSize), float(MinExtrudedDimensionSize));

static const int MaxExtrudedDimensionSize = 64;
static const Vector3 MaxExtrudedPartSize = Vector3(float(MaxExtrudedDimensionSize ), float(MaxExtrudedDimensionSize ), float(MaxExtrudedDimensionSize ));

ExtrudedPartInstance::ExtrudedPartInstance()
	: DescribedCreatable<ExtrudedPartInstance, PartInstance, sExtrudedPart>(MinExtrudedPartSize ),
	visualTrussStyle(FULL_ALTERNATING_CROSS_BEAM)
{
	setName("Truss");
	Primitive* myPrim = this->getPartPrimitive();		// this access function is used to maintain const correctness between DataModel and World
	myPrim->setSurfaceType(NORM_X, UNIVERSAL);
	myPrim->setSurfaceType(NORM_X_NEG, UNIVERSAL);
	myPrim->setSurfaceType(NORM_Y, UNIVERSAL);
	myPrim->setSurfaceType(NORM_Y_NEG, UNIVERSAL);
	myPrim->setSurfaceType(NORM_Z, UNIVERSAL);
	myPrim->setSurfaceType(NORM_Z_NEG, UNIVERSAL);
}

ExtrudedPartInstance::~ExtrudedPartInstance()
{
}
const Vector3& ExtrudedPartInstance::getMinimumUiSize() const 
{
	return MinExtrudedPartSize;
}
static int countMinExtrudedDistance(const Vector3& rbxSize)
{
	int count = 0;
	if(rbxSize.x == MinExtrudedDimensionSize){
		count++;
	}
	if(rbxSize.y == MinExtrudedDimensionSize){
		count++;
	}
	if(rbxSize.z == MinExtrudedDimensionSize){
		count++;
	}
	return count;
}

static float largestComponent(const Vector3& rbxSize)
{
	if(rbxSize.x >= rbxSize.y && rbxSize.x >= rbxSize.z){
		return rbxSize.x;
	}
	if(rbxSize.y >= rbxSize.x && rbxSize.y >= rbxSize.z){
		return rbxSize.y;
	}
	return rbxSize.z;
}


void ExtrudedPartInstance::setVisualTrussStyle(VisualTrussStyle _style)
{
	if(visualTrussStyle != _style) {
		visualTrussStyle  = _style;

		raisePropertyChanged(prop_styleXml);
		raisePropertyChanged(prop_styleUi);
		shouldRenderSetDirty();
	}
}
void ExtrudedPartInstance::setPartSizeXml(const Vector3& rbxSize)
{
	if(countMinExtrudedDistance(rbxSize) >= 2 && largestComponent(rbxSize) <= MaxExtrudedDimensionSize){
		Super::setPartSizeXml(rbxSize);
	}
	else{
		Vector3 newSize = (rbxSize.max(MinExtrudedPartSize)).min(MaxExtrudedPartSize);
		if(countMinExtrudedDistance(newSize) < 2){
			if(newSize.x >= newSize.y && newSize.x >= newSize.z){
				newSize.y = MinExtrudedPartSize.y;
				newSize.z = MinExtrudedPartSize.z;
			}
			else if(newSize.y >= newSize.x && newSize.y >= newSize.z){
				newSize.x = MinExtrudedPartSize.x;
				newSize.z = MinExtrudedPartSize.z;
			}
			else{
				RBXASSERT(newSize.z >= newSize.x && newSize.z >= newSize.y || !newSize.isFinite());
				newSize.x = MinExtrudedPartSize.x;
				newSize.y = MinExtrudedPartSize.y;
			}
		}
		if(newSize != getPartSizeXml()){
			Super::setPartSizeXml(newSize);
		}
		else{
			refreshPartSizeUi();
		}
	}
}
int ExtrudedPartInstance::getResizeIncrement() const
{
	return MinExtrudedDimensionSize;
}

Faces ExtrudedPartInstance::getResizeHandleMask() const
{
	const Vector3& size = getConstPartPrimitive()->getSize();
	if(size.x == MinExtrudedDimensionSize && size.y == MinExtrudedDimensionSize && size.z == MinExtrudedDimensionSize)
	{
		return Faces(NORM_ALL_MASK);
	}
	else{
		if(size.x != MinExtrudedDimensionSize){
			RBXASSERT(size.y == MinExtrudedDimensionSize && size.z == MinExtrudedDimensionSize);
			return Faces(NORM_X_MASK | NORM_X_NEG_MASK);
		}
		else if(size.y != MinExtrudedDimensionSize){
			RBXASSERT(size.x == MinExtrudedDimensionSize && size.z == MinExtrudedDimensionSize);
			return Faces(NORM_Y_MASK | NORM_Y_NEG_MASK);
		}
		else{
			RBXASSERT(size.z != MinExtrudedDimensionSize);
			RBXASSERT(size.x == MinExtrudedDimensionSize && size.y == MinExtrudedDimensionSize);
			return Faces(NORM_Z_MASK | NORM_Z_NEG_MASK);
		}
	}
}
}//namespace
