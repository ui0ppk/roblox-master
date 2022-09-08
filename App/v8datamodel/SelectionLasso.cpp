/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/SelectionLasso.h"
#include "Humanoid/Humanoid.h"
#include "V8DataModel/PartInstance.h"
#include "GfxBase/Adorn.h"

namespace RBX {

const char* const  sSelectionLasso = "SelectionLasso";

const Reflection::RefPropDescriptor<SelectionLasso, Humanoid>	prop_Humanoid("Humanoid", category_Data, &SelectionLasso::getHumanoidDangerous, &SelectionLasso::setHumanoid);

SelectionLasso::SelectionLasso(const char* name)
	:DescribedNonCreatable<SelectionLasso, GuiBase3d, sSelectionLasso>(name)
{}


void SelectionLasso::setHumanoid(Humanoid* value)
{
	if(humanoid.lock().get() != value){
		humanoid = shared_from(value);
		raisePropertyChanged(prop_Humanoid);
		shouldRenderSetDirty();
	}
}

bool SelectionLasso::shouldRender3dAdorn() const
{
	return Super::shouldRender3dAdorn() && (humanoid.lock());
}

bool SelectionLasso::getHumanoidPosition(Vector3& output) const
{
	if(shared_ptr<Humanoid> safeHumanoid = humanoid.lock()){
		if(PartInstance* torso = safeHumanoid->getTorsoSlow()){
			output = torso->getTranslationUi();
			return true;
		}
	}
	return false;	
}
void SelectionLasso::render3dAdorn(Adorn* adorn)
{
	Vector3 posA;
	if(!getHumanoidPosition(posA)) 
		return;
	Vector3 posB;
	if(!getPosition(posB)){
		return;
	}
	Vector3 dir = posB-posA;
	float distance = dir.unitize();

	if(distance == 0.0)
		return;

	CoordinateFrame moveEndToOrigin; // move cylinder so that end sits at origin
	moveEndToOrigin.translation.x = distance/2;


	CoordinateFrame cframe;
	cframe.lookAt(dir);
	// lookAt aligns -z axis to target.
	// we want +x
	Matrix3 rotateXtoNegZ(0,0,1,0,1,0,-1,0,0); // rotate about y by 90 degrees. (+x -> -z)
	cframe.rotation = cframe.rotation * rotateXtoNegZ;  // cylinder comes in aligned in +x, apply _pre transform_ to get it along -z, so that lookat works right.
	cframe.translation = posA;
	adorn->setObjectToWorldMatrix(cframe * moveEndToOrigin);

	adorn->cylinderAlongX(0.125, distance, color);
}



const char* const  sSelectionPartLasso = "SelectionPartLasso";

const Reflection::RefPropDescriptor<SelectionPartLasso, PartInstance>	prop_Part("Part", category_Data, &SelectionPartLasso::getPartDangerous, &SelectionPartLasso::setPart);


SelectionPartLasso::SelectionPartLasso()
	:DescribedCreatable<SelectionPartLasso, SelectionLasso, sSelectionPartLasso>(sSelectionPartLasso)
{}

void SelectionPartLasso::setPart(PartInstance* value)
{
	if(part.lock().get() != value){
		part = shared_from(value);
		raisePropertyChanged(prop_Part);
		shouldRenderSetDirty();
	}
}

bool SelectionPartLasso::shouldRender3dAdorn() const
{
	if(!part.lock())
		return false;

	return Super::shouldRender3dAdorn();
}


bool SelectionPartLasso::getPosition(Vector3& output) const
{
	if(shared_ptr<PartInstance> safePart = part.lock()){
		output = safePart->getTranslationUi();
		return true;
	}
	return false;
}

const char* const  sSelectionPointLasso = "SelectionPointLasso";

REFLECTION_BEGIN();
static const Reflection::PropDescriptor<SelectionPointLasso, Vector3> prop_Point("Point", category_Data, &SelectionPointLasso::getPoint, &SelectionPointLasso::setPoint);
REFLECTION_END();

SelectionPointLasso::SelectionPointLasso()
	:DescribedCreatable<SelectionPointLasso, SelectionLasso, sSelectionPointLasso>(sSelectionPointLasso)
{}

void SelectionPointLasso::setPoint(Vector3 value)
{
	if(point != value)
	{
		point = value;
		raisePropertyChanged(prop_Point);
	}
}

}
