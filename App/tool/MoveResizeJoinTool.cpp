/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Tool/MoveResizeJoinTool.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/ChangeHistory.h"
#include "Tool/Dragger.h"
#include "V8World/World.h"
#include "V8World/ContactManager.h"
#include "Util/Math.h"
#include "Util/HitTest.h"
#include "Util/SoundService.h"
#include "AppDraw/DrawAdorn.h"
#include "V8DataModel/Camera.h"
#include "V8World/Tolerance.h"

DYNAMIC_FASTFLAG(RestoreTransparencyOnToolChange)
DYNAMIC_FASTFLAG(DraggerUsesNewPartOnDuplicate)
FASTFLAGVARIABLE(StudioUseDraggerGrid, true)

	//Deprecate FormFactor
DYNAMIC_FASTFLAG(FormFactorDeprecated)

namespace RBX {

const char* const sMoveResizeJoinTool = "MoveResizeJoin";

/////////////////////////////////////////////////////////////////////
//
// MoveResizeJoin TOOL
//

weak_ptr<PVInstance> MoveResizeJoinTool::scalingPart;

void MoveResizeJoinTool::findTargetPV(const shared_ptr<InputObject>& inputObject)
{
	ServiceClient< Selection > selection(this->workspace);

	Instances::const_iterator it;

	for (it = selection->begin(); it != selection->end(); ++it) {
		if (PartInstance* part = Instance::fastDynamicCast<PartInstance>(it->get())) {

			RbxRay gridRay(getUnitMouseRay(inputObject));
			if (HandleHitTest::hitTestHandleLocal(	part->getPartPrimitive()->getExtentsLocal(),
													part->getLocation(),
													HANDLE_RESIZE,
													gridRay, 
													hitPointGrid, 
													localNormalId,
													part->getResizeHandleMask().normalIdMask))
			{
				targetPV = shared_from(part);
				if (DFFlag::DraggerUsesNewPartOnDuplicate)
					scalingPart = shared_from(part);
				return;
			}
		}
	}
	targetPV.reset();
};

void MoveResizeJoinTool::render3dAdorn(Adorn* adorn)
{
    renderHoverOver(adorn);

	ServiceClient< Selection > selection(this->workspace);
	shared_ptr<PartInstance> hoveredPart = targetPV.expired() ? shared_ptr<PartInstance>() : shared_polymorphic_downcast<PartInstance>(targetPV.lock());

	Instances::const_iterator it;

	for (it = selection->begin(); it != selection->end(); ++it) {
		if (PartInstance* part = Instance::fastDynamicCast<PartInstance>(it->get())) {

			RBX::DrawAdorn::handles3d(
				part->getPartSizeXml(),
				part->getCoordinateFrame(),
				adorn, 
				HANDLE_RESIZE,
				workspace->getConstCamera()->coordinateFrame().translation,
				DrawAdorn::resizeColor(),
                true,
				part->getResizeHandleMask().normalIdMask,
				hoveredPart.get() == part ? localNormalId : NORM_UNDEFINED,
				Color3(0.3f, 0.7f, 1.0f));
		}
	}
}

void MoveResizeJoinTool::render2d(Adorn* adorn)
{
	//Super::render2d(adorn);


	ServiceClient< Selection > selection(this->workspace);
	shared_ptr<PartInstance> hoveredPart = targetPV.expired() ? shared_ptr<PartInstance>() : shared_polymorphic_downcast<PartInstance>(targetPV.lock());

	Instances::const_iterator it;

	for (it = selection->begin(); it != selection->end(); ++it) {
		if (PartInstance* part = Instance::fastDynamicCast<PartInstance>(it->get())) {

				DrawAdorn::handles2d(
					part->getPartSizeXml(),
					part->getCoordinateFrame(),
					*(workspace->getConstCamera()),
					adorn, 
					HANDLE_RESIZE,
					DrawAdorn::resizeColor(),
                    true,
					part->getResizeHandleMask().normalIdMask);

            if ( FFlag::StudioUseDraggerGrid && showDraggerGrid && AdvArrowToolBase::advGridMode != DRAG::OFF && 
                 part == hoveredPart.get() &&
                 dragging && 
                 part->getPartType() != BALL_PART && 
				 part->getPartType() != CYLINDER_PART)
			{
				CoordinateFrame cf = part->getCoordinateFrame();
				Vector3 cameraDirection = workspace->getConstCamera()->getCameraCoordinateFrame().vectorToWorldSpace(Vector3::unitZ());
				Vector3 bodyXWorld = part->getCoordinateFrame().vectorToWorldSpace(Vector3::unitX());
				Vector3 bodyYWorld = part->getCoordinateFrame().vectorToWorldSpace(Vector3::unitY());
				Vector3 bodyZWorld = part->getCoordinateFrame().vectorToWorldSpace(Vector3::unitZ());

                cf.translation = origPartPosition.translation;
				Vector4 bounds = Vector4::zero();
				float integerPart = 0.0f;

				Color3 gridColor = DrawAdorn::resizeColor();
				int boxesPerStud = smallGridBoxesPerStud();

				switch( localNormalId )
				{
					case NORM_X:
						modff(part->getPartSizeUi().x * 0.2f, &integerPart);
						cf.translation += cf.vectorToWorldSpace(-origPartSize.x * 0.5f * Vector3::unitX());
						// Show grid on best plane based on camera direction
						if( fabs(bodyZWorld.dot(cameraDirection)) > fabs(bodyYWorld.dot(cameraDirection)) ) // z is normal to grid
						{
							bounds.x = (integerPart + 1.0f) * 5.0f;			// max x-dir
							bounds.z = 0.0f;								// min x-dir
							bounds.y = part->getPartSizeUi().y * 0.5f;		// max y-dir
							bounds.w = -bounds.y;							// min y-dir
							DrawAdorn::surfaceGridAtCoord(adorn, cf, bounds, Vector3::unitX(), Vector3::unitY(), gridColor, boxesPerStud);
						}
						else // y is normal to grid
						{
							bounds.x = part->getPartSizeUi().z * 0.5f;		// max x-dir
							bounds.z = -bounds.x;							// min x-dir
							bounds.y = (integerPart + 1.0f) * 5.0f;			// max y-dir
							bounds.w = 0.0f;								// min y-dir
							DrawAdorn::surfaceGridAtCoord(adorn, cf, bounds, Vector3::unitZ(), Vector3::unitX(), gridColor, boxesPerStud);
						}
						break;
					case NORM_X_NEG:
						modff(part->getPartSizeUi().x * 0.2f, &integerPart);
						cf.translation += cf.vectorToWorldSpace(origPartSize.x * 0.5f * Vector3::unitX());
						// Show grid on best plane based on camera direction
						if( fabs(bodyZWorld.dot(cameraDirection)) > fabs(bodyYWorld.dot(cameraDirection)) )
						{
							bounds.x = 0.0f;								// max x-dir
							bounds.z = -(integerPart + 1.0f) * 5.0f;		// min x-dir
							bounds.y = part->getPartSizeUi().y * 0.5f;		// max y-dir
							bounds.w = -bounds.y;							// min y-dir
							DrawAdorn::surfaceGridAtCoord(adorn, cf, bounds, Vector3::unitX(), Vector3::unitY(), gridColor, boxesPerStud);
						}
						else
						{
							bounds.x = part->getPartSizeUi().z * 0.5f;		// max x-dir
							bounds.z = -bounds.x;							// min x-dir
							bounds.y = 0.0f;								// max y-dir
							bounds.w = -(integerPart + 1.0f) * 5.0f;		// min y-dir
							DrawAdorn::surfaceGridAtCoord(adorn, cf, bounds, Vector3::unitZ(), Vector3::unitX(), gridColor, boxesPerStud);
						}
						break;
					case NORM_Y:
						// only allow one stud y-dir resize for brick form factor
						modff(part->getPartSizeUi().y * 0.2f, &integerPart);
						cf.translation += cf.vectorToWorldSpace(-origPartSize.y * 0.5f * Vector3::unitY());
						// Show grid on best plane based on camera direction
						if( fabs(bodyZWorld.dot(cameraDirection)) > fabs(bodyXWorld.dot(cameraDirection)) )
						{
							bounds.x = part->getPartSizeUi().x * 0.5f;		// max x-dir
							bounds.z = -bounds.x;							// min x-dir
							bounds.y = (integerPart + 1.0f) * 5.0f;			// max y-dir
							bounds.w = 0.0f;								// min y-dir
							DrawAdorn::surfaceGridAtCoord(adorn, cf, bounds, Vector3::unitX(), Vector3::unitY(), gridColor, boxesPerStud);
						}
						else
						{
							bounds.x = (integerPart + 1.0f) * 5.0f;			// max x-dir
							bounds.z = 0.0f;								// min x-dir
							bounds.y = part->getPartSizeUi().z * 0.5f;		// max y-dir
							bounds.w = -bounds.y;							// min y-dir
							DrawAdorn::surfaceGridAtCoord(adorn, cf, bounds, Vector3::unitY(), Vector3::unitZ(), gridColor, boxesPerStud);
						}
						break;
					case NORM_Y_NEG:
						// only allow one stud y-dir resize for brick form factor
						modff(part->getPartSizeUi().y * 0.2f, &integerPart);
						cf.translation += cf.vectorToWorldSpace(origPartSize.y * 0.5f * Vector3::unitY());
						// Show grid on best plane based on camera direction
						if( fabs(bodyZWorld.dot(cameraDirection)) > fabs(bodyXWorld.dot(cameraDirection)) )
						{
							bounds.x = part->getPartSizeUi().x * 0.5f;		// max x-dir
							bounds.z = -bounds.x;							// min x-dir
							bounds.y = 0.0f;								// max y-dir
							bounds.w = -(integerPart + 1.0f) * 5.0f;		// min y-dir
							DrawAdorn::surfaceGridAtCoord(adorn, cf, bounds, Vector3::unitX(), Vector3::unitY(), gridColor, boxesPerStud);
						}
						else
						{
							bounds.x = 0.0f;								// max x-dir
							bounds.z = -(integerPart + 1.0f) * 5.0f;		// min x-dir
							bounds.y = part->getPartSizeUi().z * 0.5f;		// max y-dir
							bounds.w = -bounds.y;							// min y-dir
							DrawAdorn::surfaceGridAtCoord(adorn, cf, bounds, Vector3::unitY(), Vector3::unitZ(), gridColor, boxesPerStud);
						}
						break;
					case NORM_Z:
						modff(part->getPartSizeUi().z * 0.2f, &integerPart);
						cf.translation += cf.vectorToWorldSpace(-origPartSize.z * 0.5f * Vector3::unitZ());
						// Show grid on best plane based on camera direction
						if( fabs(bodyYWorld.dot(cameraDirection)) > fabs(bodyXWorld.dot(cameraDirection)) )
						{
							bounds.x = (integerPart + 1.0f) * 5.0f;			// max x-dir
							bounds.z = 0.0f;								// min x-dir
							bounds.y = part->getPartSizeUi().x * 0.5f;		// max y-dir
							bounds.w = -bounds.y;							// min y-dir
							DrawAdorn::surfaceGridAtCoord(adorn, cf, bounds, Vector3::unitZ(), Vector3::unitX(), gridColor, boxesPerStud);
						}
						else
						{
							bounds.x = part->getPartSizeUi().y * 0.5f;		// max x-dir
							bounds.z = -bounds.x;							// min x-dir
							bounds.y = (integerPart + 1.0f) * 5.0f;			// max y-dir
							bounds.w = 0.0f;								// min y-dir
							DrawAdorn::surfaceGridAtCoord(adorn, cf, bounds, Vector3::unitY(), Vector3::unitZ(), gridColor, boxesPerStud);
						}
						break;
					case NORM_Z_NEG:
						modff(part->getPartSizeUi().z * 0.2f, &integerPart);
						cf.translation += cf.vectorToWorldSpace(origPartSize.z * 0.5f * Vector3::unitZ());
						// Show grid on best plane based on camera direction
						if( fabs(bodyYWorld.dot(cameraDirection)) > fabs(bodyXWorld.dot(cameraDirection)) )
						{
							bounds.x = 0.0f;								// max x-dir
							bounds.z = -(integerPart + 1.0f) * 5.0f;		// min x-dir
							bounds.y = part->getPartSizeUi().x * 0.5f;		// max y-dir
							bounds.w = -bounds.y;							// min y-dir
							DrawAdorn::surfaceGridAtCoord(adorn, cf, bounds, Vector3::unitZ(), Vector3::unitX(), gridColor, boxesPerStud);
						}
						else
						{
							bounds.x = part->getPartSizeUi().y * 0.5f;		// max x-dir
							bounds.z = -bounds.x;							// min x-dir
							bounds.y = 0.0f;								// max y-dir
							bounds.w = -(integerPart + 1.0f) * 5.0f;		// min y-dir
							DrawAdorn::surfaceGridAtCoord(adorn, cf, bounds, Vector3::unitY(), Vector3::unitZ(), gridColor, boxesPerStud);
						}
						break;
                    default:
                        break;
				}
			}
		}
	}
}

void MoveResizeJoinTool::onMouseHover(const shared_ptr<InputObject>& inputObject)
{
	RBXASSERT(!captured());

	findTargetPV(inputObject);
	overHandle = (!targetPV.expired());

	Super::onMouseHover(inputObject);
}
	
void MoveResizeJoinTool::onMouseIdle(const shared_ptr<InputObject>& inputObject)
{
	Super::onMouseIdle(inputObject);
	
	if (dragging) {
		;
	}
	else if (captured()) {
		cursor = "advClosed-hand";
	}
	else if (overHandle) {
		cursor = "advCursor-openedHand";
	}
	else {
		cursor = Super::getCursorName();
	}
}

shared_ptr<MouseCommand> MoveResizeJoinTool::onMouseDown(const shared_ptr<InputObject>& inputObject)
{
	findTargetPV(inputObject);
	if ( targetPV.expired()) 
	{
		return Super::onMouseDown(inputObject);		
	}
	else
	{
		down = inputObject->get2DPosition();
		movePerp = 0;
		overHandle = true;
		shared_ptr<PartInstance> part = shared_polymorphic_downcast<PartInstance>(targetPV.lock());
		// try to prevent the part from moving due to physics effects during the resize operation
		part->setDragging(true);
		// set this based on Grid setting
		origPartPosition = part->getCoordinateFrame();
		origPartSize = part->getPartSizeUi();
		// if collision checks are turned off then, make part transparent
		if (!RBX::AdvArrowTool::advCollisionCheckMode)
		{
			if (DFFlag::RestoreTransparencyOnToolChange)
				AdvArrowToolBase::savePartTransparency(part);
			else
			{
				origPartTransparency = part->getTransparencyUi();
				part->setTransparency(0.5);
			}
		}
		capture();
		return shared_from(this);	// captured is true....
	}
}

float MoveResizeJoinTool::moveIncrement( void )
{
	switch( AdvArrowTool::advGridMode )
	{
	case DRAG::OFF:
		return 0.01f;
	case DRAG::QUARTER_STUD:
		return 0.2f;
	case DRAG::ONE_STUD:
	default:
		{
			if (DFFlag::FormFactorDeprecated) 
			{
				return 1.0f;
			}
			shared_ptr<PartInstance> part = shared_polymorphic_downcast<PartInstance>(targetPV.lock());
			switch(part->getFormFactor())
			{
			case PartInstance::BRICK:
				return localNormalId % 3 == 1 ? 1.2f : 1.0f;
			case PartInstance::PLATE:
				return localNormalId % 3 == 1 ? 0.4f : 1.0f;
			default:
				return 1.0f;
			}
		}
	}
}

int MoveResizeJoinTool::smallGridBoxesPerStud( void )
{
	switch( AdvArrowTool::advGridMode )
	{
	case DRAG::OFF:
		return 0;
	case DRAG::QUARTER_STUD:
		return 5;
	case DRAG::ONE_STUD:
	default:
		return 1;
	}
}

shared_ptr<MouseCommand> MoveResizeJoinTool::onKeyDown(const shared_ptr<InputObject>& inputObject)
{
	Super::onKeyDown(inputObject);

	return shared_from(this);
}


void MoveResizeJoinTool::onMouseMove(const shared_ptr<InputObject>& inputObject)
{
	Super::onMouseMove(inputObject);

	if (DFFlag::DraggerUsesNewPartOnDuplicate && scalingPart.lock() != targetPV.lock())
	{
		shared_ptr<PartInstance> origPart = shared_dynamic_cast<PartInstance>(targetPV.lock());
		origPart->setDragging(false);
		targetPV = scalingPart;
		shared_ptr<PartInstance> part = shared_dynamic_cast<PartInstance>(targetPV.lock());
		part->setDragging(true);
		origPartPosition = origPartPosition + Vector3(0.0f, part->getCoordinateFrame().translation.y - origPart->getCoordinateFrame().translation.y, 0.0f);
	}
	
	if (!targetPV.expired()) {
		Vector3 worldNormal = Math::getWorldNormal(localNormalId, targetPV.lock()->getLocation());
		RbxRay axisRay = RbxRay::fromOriginAndDirection(hitPointGrid, worldNormal);
		RbxRay gridRay = getUnitMouseRay(inputObject);
		Vector3 closePoint = RBX::Math::closestPointOnRay(axisRay, gridRay);
		Vector3 handleToCurrent = closePoint - hitPointGrid;
		float distanceAlongAxis = axisRay.direction().dot(handleToCurrent);

    	capturedDrag(distanceAlongAxis);
	}

	// During drag set the grid adornment based on current grid setting
	dragging = true;

}

shared_ptr<MouseCommand> MoveResizeJoinTool::onMouseUp(const shared_ptr<InputObject>& inputObject)
{
	dragging = false;
	if (DFFlag::RestoreTransparencyOnToolChange)
		AdvArrowToolBase::restoreSavedPartsTransparency();
	shared_ptr<PartInstance> part = shared_polymorphic_downcast<PartInstance>(targetPV.lock());
	if( part )
	{
		if (!RBX::AdvArrowTool::advCollisionCheckMode && !DFFlag::RestoreTransparencyOnToolChange)
			part->setTransparency(origPartTransparency);
        // resize is done.  Let physics take effect again.
		part->setDragging(false);
		origPartPosition = part->getCoordinateFrame();
		origPartSize = part->getPartSizeUi();
	}

	if (captured()) {

		releaseCapture();


		// Save the undo/redo state - if deleted in the process, then don't
		if (!targetPV.expired()) {
			workspace->getDataState().setDirty(true);
		}
	}

	//baseclass will take care of setting waypoint for undo/redo
	Super::onMouseUp(inputObject);

	return shared_ptr<MouseCommand>();
}


void MoveResizeJoinTool::capturedDrag(float axisDelta)
{
	if (targetPV.expired()) {
		RBXASSERT(0);	// should be caught higher up
	}
	else {
		// Debug version confirms the dynamic_cast of targetInstance
		shared_ptr<PartInstance> part = shared_polymorphic_downcast<PartInstance>(targetPV.lock());

		resizeFloat(part, localNormalId, axisDelta, RBX::AdvArrowTool::advCollisionCheckMode);
	}
}


bool MoveResizeJoinTool::resizeFloat(shared_ptr<PartInstance> part, NormalId localNormalId, float amount, bool checkIntersection)
{
	part->destroyJoints();
	bool result = advResizeImpl(part, localNormalId, amount, checkIntersection);
	part->join();
	return result;
}


bool MoveResizeJoinTool::advResizeImpl(shared_ptr<PartInstance> part, NormalId localNormalId, float amount, bool checkIntersection)
{
    amount = G3D::iRound(amount / moveIncrement()) * moveIncrement();

	Vector3 originalSizeXml = origPartSize;
    CoordinateFrame originalPosition = origPartPosition;

	Vector3 deltaSizeUi; 
	float posNeg = (localNormalId < NORM_X_NEG) ? 1.0f : -1.0f;

	if (part->hasThreeDimensionalSize())
    {
		deltaSizeUi[localNormalId % 3] = amount;
	}
	else
    {
		RBXASSERT(part->getFormFactor() == PartInstance::SYMETRIC);
		deltaSizeUi = amount * Vector3(originalSizeXml.x / originalSizeXml.y, 1, originalSizeXml.z / originalSizeXml.y);
	}

	Vector3 newSizeUi = originalSizeXml + deltaSizeUi;

    if (!part->hasThreeDimensionalSize())
    {
        if (fabs(newSizeUi.x) < fabs(newSizeUi.z) && fabs(newSizeUi.x) < fabs(newSizeUi.y))
        {
            float minDimension = part->getMinimumXOrZDimension();
            if (newSizeUi.x < minDimension)
                newSizeUi = Vector3(minDimension, minDimension, minDimension) * Vector3(1.0f, originalSizeXml.y / originalSizeXml.x, originalSizeXml.z / originalSizeXml.x);
        }
        else if (fabs(newSizeUi.z) < fabs(newSizeUi.y))
        {
            float minDimension = part->getMinimumXOrZDimension();
            if (newSizeUi.z < minDimension)
                newSizeUi = Vector3(minDimension, minDimension, minDimension) * Vector3(originalSizeXml.x / originalSizeXml.z, originalSizeXml.y / originalSizeXml.z, 1.0f);
        }
        else
        {
            float minDimension = part->getMinimumYDimension();
            if (newSizeUi.y < minDimension)
                newSizeUi = Vector3(minDimension, minDimension, minDimension) * Vector3(originalSizeXml.x / originalSizeXml.y, 1.0f, originalSizeXml.z / originalSizeXml.y);
        }
    }
    else
    {
		if (DFFlag::FormFactorDeprecated) {
			if (AdvArrowTool::advGridMode != DRAG::OFF) {
				double gridSize = 0.2f;
				if (AdvArrowTool::advGridMode == DRAG::ONE_STUD) gridSize = 1.0f;

				/*
					To explain the logic below:
					If the new size is greater than the grid size, then don't do anything. Just set the new size. Otherwise, use a separate logic to get the size:
						newSizeUi = newSizeUi < gridSize? (x) : newSizeUi;

					If the fmod(originalSizeXml,gridSize) is greater than the minimum dimensions (should be 0.2) then set the part size to that fmod - otherwise, set to the gridSize.
					This logic is there for the cases where fmod(originalSizeXml,gridSize) is very close to 0 - as in, the originalSizeXml is an integer multiple of the gridSize. 
					In this case we want the minimum size not to be 0, but the gridSize.
				*/
				newSizeUi.x = newSizeUi.x < gridSize ? (fmod(originalSizeXml.x,gridSize) >= part->getMinimumXOrZDimension() ? fmod(originalSizeXml.x,gridSize) : gridSize) : newSizeUi.x;
				newSizeUi.y = newSizeUi.y < gridSize ? (fmod(originalSizeXml.y,gridSize) >= part->getMinimumYDimension() ? fmod(originalSizeXml.y,gridSize) : gridSize) : newSizeUi.y;
				newSizeUi.z = newSizeUi.z < gridSize ? (fmod(originalSizeXml.z,gridSize) >= part->getMinimumXOrZDimension() ? fmod(originalSizeXml.z,gridSize) : gridSize) : newSizeUi.z;
			}
			newSizeUi.x = newSizeUi.x < part->getMinimumXOrZDimension() ? part->getMinimumXOrZDimension() : newSizeUi.x;
			newSizeUi.y = newSizeUi.y < part->getMinimumYDimension() ? part->getMinimumYDimension() : newSizeUi.y;
			newSizeUi.z = newSizeUi.z < part->getMinimumXOrZDimension() ? part->getMinimumXOrZDimension() : newSizeUi.z;
		} else {
			newSizeUi.x = newSizeUi.x < part->getMinimumXOrZDimension() ? part->getMinimumXOrZDimension() : newSizeUi.x;
			newSizeUi.y = newSizeUi.y < part->getMinimumYDimension() ? part->getMinimumYDimension() : newSizeUi.y;
			newSizeUi.z = newSizeUi.z < part->getMinimumXOrZDimension() ? part->getMinimumXOrZDimension() : newSizeUi.z;
		}
    }

    Vector3 deltaSizeXml = newSizeUi - originalSizeXml;
	Vector3 deltaSizeWorld = originalPosition.rotation * deltaSizeXml;

    Vector3 newTranslation;
    
    if (!part->hasThreeDimensionalSize())
    {
        Vector3 direction;
        direction[localNormalId % 3] = 1.0f;
    	Vector3 deltaSizeWorldY = originalPosition.rotation * (deltaSizeXml * direction);
        newTranslation = originalPosition.translation;
        newTranslation = newTranslation + 0.5 * deltaSizeWorldY * posNeg;
    }
    else
    {
        newTranslation = originalPosition.translation + 0.5f * deltaSizeWorld * posNeg;
    }

	CoordinateFrame newCoord(originalPosition.rotation, newTranslation);

    Vector3 preCollisionSizeXml = part->getPartSizeXml();
    CoordinateFrame preCollisionPosition = part->getCoordinateFrame();
     
	part->setCoordinateFrame(newCoord);
	part->setPartSizeXml(newSizeUi);

	if (checkIntersection)
	{
		if (ContactManager* contactManager = Workspace::getContactManagerIfInWorkspace(part.get())) {
			if (Dragger::intersectingWorldOrOthers(	*part, 
													*contactManager, 
													Tolerance::maxOverlapAllowedForResize(),
													Dragger::maxDragDepth())) {
					
				part->setCoordinateFrame(preCollisionPosition);
				part->setPartSizeXml(preCollisionSizeXml);
				return false;
			}
		}
	}

	return true;
}

void MoveResizeJoinTool::setSelection(shared_ptr<RBX::Instance> oldSelection, shared_ptr<RBX::Instance> newSelection)
{
	if (shared_ptr<RBX::PVInstance> partInstance = RBX::Instance::fastSharedDynamicCast<RBX::PVInstance>(oldSelection))
	{
		if (partInstance == scalingPart.lock())
		{
			if (shared_ptr<RBX::PVInstance> pvInstance = RBX::Instance::fastSharedDynamicCast<RBX::PVInstance>((newSelection)))
			{
				scalingPart = pvInstance;
				if (DFFlag::RestoreTransparencyOnToolChange && !RBX::AdvArrowTool::advCollisionCheckMode)
				{
					shared_ptr<PartInstance> selectedPart = RBX::Instance::fastSharedDynamicCast<RBX::PartInstance>(newSelection);
					AdvArrowToolBase::savePartTransparency(selectedPart);
				}
			}
		}
	}
}


} // namespace
