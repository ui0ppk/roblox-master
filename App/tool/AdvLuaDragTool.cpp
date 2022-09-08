/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Tool/AdvLuaDragTool.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Workspace.h"
#include "V8Datamodel/ChangeHistory.h"
#include "V8DataModel/GameBasicSettings.h"

LOGGROUP(DragProfile)
DYNAMIC_FASTFLAG(RestoreTransparencyOnToolChange)

namespace RBX {

// This must be a human-readable name. It will be used for such things as undo/redo:
const char* const sAdvLuaDragTool = "Drag";

AdvLuaDragTool::AdvLuaDragTool(	PartInstance* mousePart,
							const Vector3& hitPointWorld,
							const std::vector<weak_ptr<PartInstance> >& partArray,
							Workspace* workspace,
							shared_ptr<Instance> selectIfNoDrag
							)
	: Named<AdvArrowToolBase, sAdvLuaDragTool>(workspace)
	, selectIfNoDrag(selectIfNoDrag)
	, cursor("advCursor-default")
    , dragging(false)

{
	advLuaDragger = Creatable<Instance>::create<AdvLuaDragger>();

	advLuaDragger->mouseDown(	shared_from<PartInstance>(mousePart),
							mousePart->getCoordinateFrame().pointToObjectSpace(hitPointWorld),
							partArray	);
}

AdvLuaDragTool::~AdvLuaDragTool()
{
	RBX::GameBasicSettings::singleton().setCanMousePan(true);
}


shared_ptr<MouseCommand> AdvLuaDragTool::onMouseDown(const shared_ptr<InputObject>& inputObject)
{
	if(PartInstance* part = getUnlockedPart(inputObject))
		if(!part->getDragUtilitiesSupport())
			return shared_ptr<MouseCommand>();

	capture();
	setCursor("advClosed-hand");

	RBX::GameBasicSettings::singleton().setCanMousePan(false);

    downPoint2d = inputObject->get2DPosition();

	return shared_from(this);
}

bool AdvLuaDragTool::canDrag(const shared_ptr<InputObject>& inputObject) const
{   
    // Adding some tolerance so that selection and move are farther apart as acitons.
    if (!dragging)
    {
        if (((Vector2(inputObject->get2DPosition()) - downPoint2d).length() >= 8))
            return true;
        else
            return false;
    }
    
    return true;
}

void AdvLuaDragTool::onMouseMove(const shared_ptr<InputObject>& inputObject)
{
    if (canDrag(inputObject))
        dragging = true;
    else
        return;

	FASTLOG(FLog::DragProfile, "AdvLuaDragTool - mouse move received");
	Super::onMouseMove(inputObject);

	RBXASSERT(this->captured());
	advLuaDragger->mouseMove(MouseCommand::getUnitMouseRay(inputObject, workspace));
}


void AdvLuaDragTool::onMouseIdle(const shared_ptr<InputObject>& inputObject)
{
    if (canDrag(inputObject))
        dragging = true;
}


shared_ptr<MouseCommand> AdvLuaDragTool::onMouseUp(const shared_ptr<InputObject>& inputObject)
{
	RBXASSERT(this->captured());
	advLuaDragger->mouseUp();
    dragging = false;
    
	if (DFFlag::RestoreTransparencyOnToolChange)
		AdvArrowToolBase::restoreSavedPartsTransparency();

	RBX::GameBasicSettings::singleton().setCanMousePan(true);
	if (!advLuaDragger->didDrag())
	{
		if (shared_ptr<Instance> instance = selectIfNoDrag.lock())
		{
			ServiceClient< Selection > selection(workspace);

			shared_ptr<Instances> instances(new Instances());
			instances->push_back(instance);
			selection->setSelection(instances);
		}
	}

	releaseCapture();
	
	if (DragUtilities::anyPartAlive(advLuaDragger->getParts())) {
		workspace->setInsertPoint(DragUtilities::computeExtents(advLuaDragger->getParts()).topCenter());
	}

	Super::onMouseUp(inputObject);

	if (advLuaDragger->didDrag())
		ChangeHistoryService::requestWaypoint(getName().c_str(), workspace);

	return shared_ptr<MouseCommand>();
}

shared_ptr<MouseCommand> AdvLuaDragTool::onKeyDown(const shared_ptr<InputObject>& inputObject)
{
	Super::onKeyDown(inputObject);

	if (advLuaDragger->isDragging()) {
		switch (inputObject->getKeyCode())
		{
		case SDLK_r: // _R_otate 90 about snap face z axis
			{
				advLuaDragger->rotateOnSnapFace(Vector3::Z_AXIS, Math::matrixRotateY());
				break;
			}
		case SDLK_t: // _T_ilt 90 about snap face x axis
			{
				advLuaDragger->rotateOnSnapFace(Vector3::X_AXIS, Math::matrixTiltZ());
				break;
			}
		case SDLK_e: // Align part to grid (_E_ject ???)
			{
				advLuaDragger->alignPartToGrid();
				break;
			}
        default:
            break;
		}
		return shared_from(this);
	}
	else {
		RBXASSERT(0);		// when did this happen
		return shared_ptr<MouseCommand>();
	}
}


} // namespace
