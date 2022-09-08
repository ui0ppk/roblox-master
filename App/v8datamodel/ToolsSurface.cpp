/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/ToolsSurface.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/Surface.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/ChangeHistory.h"
#include "Util/Math.h"
#include "AppDraw/DrawAdorn.h"
#include "Util/SoundService.h"
#include "Util/UserInputBase.h"

namespace RBX {

const char* const sFlatTool = "Flat";
const char* const sGlueTool = "Glue";
const char* const sWeldTool = "Weld";
const char* const sStudsTool = "Studs";
const char* const sInletTool = "Inlet";
const char* const sUniversalTool = "Universal";
const char* const sHingeTool = "Hinge";
const char* const sRightMotorTool = "RightMotor";
const char* const sLeftMotorTool = "LeftMotor";
const char* const sOscillateMotorTool = "OscillateMotor";
const char* const sSmoothNoOutlinesTool = "SmoothNoOutlines";
const char* const sDecalTool = "DecalTool";



SurfaceTool::SurfaceTool(Workspace* workspace) :
	MouseCommand(workspace)
{
	FASTLOG1(FLog::MouseCommandLifetime, "Surface Tool created: %p", this);	
}

SurfaceTool::~SurfaceTool()
{
	FASTLOG1(FLog::MouseCommandLifetime, "Surface Tool destroyed: %p", this);	
}

void SurfaceTool::onMouseHover(const shared_ptr<InputObject>& inputObject)
{
	PartInstance* tempPartInstance;
	surface = getSurface(workspace, inputObject, NULL, tempPartInstance, surfaceId);
	partInstance = shared_from(surface.getPartInstance() ? tempPartInstance : NULL);
}

shared_ptr<MouseCommand> SurfaceTool::onMouseDown(const shared_ptr<InputObject>& inputObject)
{
	onMouseHover(inputObject);
	if (surface.getPartInstance()) 
	{
		doAction(&surface);
		ChangeHistoryService::requestWaypoint(name.c_str(), workspace);
		workspace->getDataState().setDirty(true);
	}
	return shared_ptr<MouseCommand>();
}

void SurfaceTool::render3dAdorn(Adorn* adorn)
{
	Super::render3dAdorn(adorn);

	if (partInstance) {
		RBXASSERT(surface.getPartInstance());

		DrawAdorn::partSurface(
			partInstance->getPart(), 
			surfaceId,
			adorn);
	}			
}

DecalTool::DecalTool(Workspace* workspace, Decal *decal, RBX::InsertMode insertMode)
: Named<SurfaceTool, sDecalTool>(workspace)	
{
	this->decal = shared_from(decal);
	this->insertMode = insertMode;
	parentIsPart = Instance::fastDynamicCast<RBX::PartInstance>(decal->getParent()) != NULL;
}

void DecalTool::onMouseHover(const shared_ptr<InputObject>& inputObject)
{
	SurfaceTool::onMouseHover(inputObject);

	if ((insertMode == RBX::INSERT_TO_3D_VIEW) && !parentIsPart)
	{
		if (partInstance)
		{
			decal->setParent(partInstance.get());
			decal->setFace(surface.getNormalId());
		}
		else
		{
			decal->setParent(NULL);
		}
	}
	else
	{
		if (partInstance && (decal->isDescendantOf(partInstance.get())))
			decal->setFace(surface.getNormalId());
		else
			partInstance = shared_ptr<PartInstance>();
	}
}

shared_ptr<MouseCommand> DecalTool::onKeyDown(const shared_ptr<InputObject>& inputObject)
{
	// delete or ESC abort the drag and kill the decal
	if (inputObject->getKeyCode() == SDLK_DELETE || inputObject->getKeyCode() == SDLK_ESCAPE)
	{
		return onCancelOperation();
	}
	return shared_from(this);
}

shared_ptr<MouseCommand> DecalTool::onCancelOperation()
{
	releaseCapture();
	if (decal) decal.get()->setParent(NULL);
	return shared_ptr<MouseCommand>();
}

shared_ptr<MouseCommand> DecalTool::onMouseUp(const shared_ptr<InputObject>& inputObject)
{
	if (insertMode != INSERT_TO_3D_VIEW)
	{
		releaseCapture();
		ChangeHistoryService::requestWaypoint(getName().c_str(), workspace);
		return shared_ptr<MouseCommand>();
	}

	if (Instance::fastDynamicCast<RBX::PartInstance>(decal->getParent()) != NULL)
	{
		parentIsPart = true;
		insertMode = INSERT_TO_TREE;
		return shared_from(this);
	}

	return shared_ptr<MouseCommand>();
}



void FlatTool::doAction(Surface* surface)
{
	surface->getPartInstance()->destroyImplicitJoints();
		surface->flat();
	surface->getPartInstance()->join();
}

void GlueTool::doAction(Surface* surface)
{
	surface->getPartInstance()->destroyImplicitJoints();
		surface->flat();
		surface->setSurfaceType(GLUE);
	surface->getPartInstance()->join();
}

void WeldTool::doAction(Surface* surface)
{
	surface->getPartInstance()->destroyImplicitJoints();
		surface->flat();
		surface->setSurfaceType(WELD);
	surface->getPartInstance()->join();
}

void StudsTool::doAction(Surface* surface)
{
	surface->getPartInstance()->destroyImplicitJoints();
		surface->flat();
		surface->setSurfaceType(STUDS);
	surface->getPartInstance()->join();
}

void InletTool::doAction(Surface* surface)
{
	surface->getPartInstance()->destroyImplicitJoints();
		surface->flat();
		surface->setSurfaceType(INLET);
	surface->getPartInstance()->join();
}

void UniversalTool::doAction(Surface* surface)
{
	surface->getPartInstance()->destroyImplicitJoints();
		surface->flat();
		surface->setSurfaceType(UNIVERSAL);
	surface->getPartInstance()->join();
}

void HingeTool::doAction(Surface* surface)
{
	surface->getPartInstance()->destroyImplicitJoints();
		surface->setSurfaceType(ROTATE);
		surface->setSurfaceInput(LegacyController::NO_INPUT);
		surface->setParamA(0.0);
		surface->setParamB(0.0);
	surface->getPartInstance()->join();
}

bool HingeTool::isStickyVerb = true;
shared_ptr<MouseCommand> HingeTool::isSticky() const
{	
	if (isStickyVerb)
		return Creatable<MouseCommand>::create<HingeTool>(workspace);
	else
		return shared_ptr<MouseCommand>();
}

void RightMotorTool::doAction(Surface* surface)
{
	surface->getPartInstance()->destroyImplicitJoints();
		surface->setSurfaceType(ROTATE_V);
		surface->setSurfaceInput(LegacyController::CONSTANT_INPUT);
		surface->setParamA(-0.1f);
		surface->setParamB(0.1f);
	surface->getPartInstance()->join();
}

bool RightMotorTool::isStickyVerb = true;
shared_ptr<MouseCommand> RightMotorTool::isSticky() const
{	
	if (isStickyVerb)
		return Creatable<MouseCommand>::create<RightMotorTool>(workspace);
	else
		return shared_ptr<MouseCommand>();
}

void LeftMotorTool::doAction(Surface* surface)
{
	surface->getPartInstance()->destroyImplicitJoints();
		surface->setSurfaceType(ROTATE_V);
		surface->setSurfaceInput(LegacyController::CONSTANT_INPUT);
		surface->setParamA(-0.1f);
		surface->setParamB(0.1f);
	surface->getPartInstance()->join();
}

void OscillateMotorTool::doAction(Surface* surface)
{
	surface->getPartInstance()->destroyImplicitJoints();
		surface->setSurfaceType(ROTATE_V);
		surface->setSurfaceInput(LegacyController::SIN_INPUT);
		surface->setParamA(-0.1f);
		surface->setParamB(0.1f);
	surface->getPartInstance()->join();
}

void SmoothNoOutlinesTool::doAction(Surface* surface)
{
	surface->getPartInstance()->destroyImplicitJoints();
	surface->flat();
	surface->setSurfaceType(NO_SURFACE_NO_OUTLINES);
	surface->getPartInstance()->join();
}
} // namespace
