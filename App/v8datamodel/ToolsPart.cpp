/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/ToolsPart.h"

#include "V8DataModel/Workspace.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/SpecialMesh.h"
#include "V8DataModel/ChangeHistory.h"
#include "Script/Script.h"
#include "V8World/Primitive.h"
#include "V8World/World.h"
#include "Util/SoundService.h"
#include "SelectState.h"

namespace RBX {

const char* const sFillTool = "Fill";
const char* const  sMaterialTool = "Material";
const char* const sDropperTool = "Dropper";


PartTool::PartTool(Workspace* workspace) :
	MouseCommand(workspace)
{
	FASTLOG1(FLog::MouseCommandLifetime, "PartTool created: %p", this);
}

PartTool::~PartTool()
{
	FASTLOG1(FLog::MouseCommandLifetime, "PartTool destroyed: %p", this);
}

void PartTool::onMouseHover(const shared_ptr<InputObject>& inputObject)
{
	partInstance = shared_from(getPart(workspace, inputObject));
}


void PartTool::render3dAdorn(Adorn* adorn)
{
	Super::render3dAdorn(adorn);

	if (partInstance) {
		partInstance->render3dSelect(adorn, SELECT_NORMAL);
	}			
}


FillToolColor FillTool::color;

FillToolColor::FillToolColor():color(BrickColor::defaultColor()) {}

shared_ptr<MouseCommand> FillTool::onMouseDown(const shared_ptr<InputObject>& inputObject)
{
	if (PartInstance* part = getPart(workspace, inputObject)) {
		part->setColor(color.get());

		ChangeHistoryService::requestWaypoint(getName().c_str(), workspace);
		workspace->getDataState().setDirty(true);
	}
	return shared_ptr<MouseCommand>();
}


PartMaterial MaterialTool::material = PLASTIC_MATERIAL;

shared_ptr<MouseCommand> MaterialTool::onMouseDown(const shared_ptr<InputObject>& inputObject)
{
	if (PartInstance* part = getPart(workspace, inputObject)) {
		part->setRenderMaterial(material);

		ChangeHistoryService::requestWaypoint(getName().c_str(), workspace);
		workspace->getDataState().setDirty(true);
	}
	return shared_ptr<MouseCommand>();
}

shared_ptr<MouseCommand> DropperTool::onMouseDown(const shared_ptr<InputObject>& inputObject)
{
	if (PartInstance* part = getPart(workspace, inputObject)) {
		FillTool::color.set(part->getColor());
	}

	//enable FillTool
	return Creatable<RBX::MouseCommand>::create<RBX::FillTool>(workspace);
}


} // namespace
