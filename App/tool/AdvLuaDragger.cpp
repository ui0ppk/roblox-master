/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Tool/ToolsArrow.h"
#include "Tool/AdvLuaDragger.h"
#include "Tool/DragUtilities.h"
#include "Tool/Dragger.h"
#include "Tool/AdvRunDragger.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/MouseCommand.h"
#include "V8World/ContactManager.h"
#include "V8World/World.h"
#include "V8World/Joint.h"
#include "Tool/DragTypes.h"

#define DISABLE_GROUP_SNAP

LOGGROUP(DragProfile)
DYNAMIC_FASTFLAGVARIABLE(UnifyDragGridSizes, true)

namespace RBX {

const char* const sAdvLuaDragger = "AdvancedDragger";

AdvLuaDragger::AdvLuaDragger()
	: DescribedCreatable<AdvLuaDragger, Instance, sAdvLuaDragger>(sAdvLuaDragger)
	, dragPhase(NO_PARTS)
	, hitPointHeight(0.0f)
{
	lockParent();
}

AdvLuaDragger::~AdvLuaDragger()
{
}

static void addPart(shared_ptr<Instance> part, std::vector<weak_ptr<PartInstance> >* outputParts)
{
	if(shared_ptr<PartInstance> partInstance = boost::dynamic_pointer_cast<PartInstance>(part)){
		if (Workspace::getWorkspaceIfInWorkspace(partInstance.get())) {
			weak_ptr<PartInstance> weakPartInstance(partInstance);
			outputParts->push_back(weakPartInstance);
		}
		else {
			throw std::runtime_error("Only Part objects in the Workspace should be passed to a Dragger:MouseDown function");
		}
	}
	else{
		throw std::runtime_error("Only Part objects should be passed to a Dragger:MouseDown function");
	}
}

static void copyParts(shared_ptr<const Instances> inputParts, std::vector<weak_ptr<PartInstance> >& outputParts)
{
	std::for_each(inputParts->begin(), inputParts->end(), boost::bind(&addPart, _1, &outputParts));
}

void AdvLuaDragger::mouseDownPublic(shared_ptr<Instance> _mousePart,
				 				 Vector3 _pointOnMousePart,
								 shared_ptr<const Instances> _dragParts)
{
	if (std::find(_dragParts->begin(), _dragParts->end(), _mousePart) == _dragParts->end())
		throw std::runtime_error("Mouse part needs to be in the list of drag parts when you call Dragger::MouseDown");

	shared_ptr<PartInstance> mousePartInstance = boost::dynamic_pointer_cast<PartInstance>(_mousePart);
	std::vector<weak_ptr<PartInstance> > dragPartInstances;
	copyParts(_dragParts, dragPartInstances);
	if(mousePartInstance.get() == NULL)
		throw std::runtime_error("You must have a non-nil MousePart when you call Dragger:MouseDown");
	if (!Workspace::getWorkspaceIfInWorkspace(mousePartInstance.get())) 
		throw std::runtime_error("You must have a MousePart that is in the Workspace when you call Dragger:MouseDown");
	if(dragPartInstances.size() == 0)
		throw std::runtime_error("You must have some parts when you call Dragger:MouseDown");

	mouseDown(mousePartInstance, _pointOnMousePart, dragPartInstances);
}


void AdvLuaDragger::mouseDown(shared_ptr<PartInstance> _mousePart,
						   const Vector3& _pointOnMousePart,
						   const std::vector<weak_ptr<PartInstance> > _dragParts)
{
	RBXASSERT(Workspace::getWorkspaceIfInWorkspace(_mousePart.get()));

	if ((dragPhase == MOUSE_DOWN) || (dragPhase == DRAGGING)) {
		throw std::runtime_error("Call to AdvLuaDragger::mouseDown when already dragging");
	}

	RBXASSERT(jointsIMade.size() == 0);

	dragParts = _dragParts;
	mousePart = _mousePart;
	pointOnMousePart = _pointOnMousePart;

	for (WeakParts::const_iterator iter = dragParts.begin(); iter != dragParts.end(); ++iter)
	{
		if (shared_ptr<PartInstance> partInstance = iter->lock())
			m_originalPositions.push_back(_mousePart->getCoordinateFrame().toObjectSpace(partInstance->getCoordinateFrame()));
		else
			m_originalPositions.push_back(CoordinateFrame());
	}

	dragPhase = MOUSE_DOWN;
}

void AdvLuaDragger::mouseMove(RbxRay mouseRay)
{
	FASTLOG3F(FLog::DragProfile, "AdvLuaDragger - received mouse ray, origin: %fx%fx%f", 
		mouseRay.origin().x, mouseRay.origin().y, mouseRay.origin().z);

	FASTLOG3F(FLog::DragProfile, "AdvLuaDragger - received mouse ray, direction: %fx%fx%f", 
		mouseRay.direction().x, mouseRay.direction().y, mouseRay.direction().z);

	RbxRay unitRay = mouseRay.unit();

	if (dragPhase == NO_PARTS) {
		throw std::runtime_error("Call to AdvLuaDragger::mouseMove without mouseDown");
	}
	else if ((dragPhase == MOUSE_UP_DRAGGED) || (dragPhase == MOUSE_UP_NO_DRAG)) {
		throw std::runtime_error("Call to AdvLuaDragger::mouseMove after mouseUp");
	}
	else if (dragPhase == MOUSE_DOWN) {		
		tryStartDragging(unitRay);		// can change phase to DRAGGING
	}

	if (dragPhase == DRAGGING) {
		doDrag(unitRay);
	}
}

void AdvLuaDragger::mouseUp()
{
	if (dragPhase == DRAGGING) {

		if(shared_ptr<PartInstance> safeMousePart = mousePart.lock())
		{
			if(Workspace::getWorkspaceIfInWorkspace(safeMousePart.get()))
			{
				G3D::Array<Primitive*> primitives;
				DragUtilities::partsToPrimitives(dragParts, primitives);
				RBXASSERT(primitives.size() > 0);
			}
		}
		
		DragUtilities::joinToOutsiders(dragParts);
				
		DragUtilities::stopDragging(dragParts);
		advRunDragger.reset();

		dragPhase = MOUSE_UP_DRAGGED;
		


	}
	else if (dragPhase == MOUSE_DOWN) {
		dragPhase = MOUSE_UP_NO_DRAG;
	}
	else {
		throw std::runtime_error("Call to AdvLuaDragger::mouseUp without mouseDown");
	}

	RBXASSERT(jointsIMade.size() == 0);
}

void AdvLuaDragger::tryStartDragging(const RbxRay& unitMouseRay)
{
	if (shared_ptr<PartInstance> safeMousePart = mousePart.lock()){
		if (Workspace::getWorkspaceIfInWorkspace(safeMousePart.get())) {
			Vector3 mousePartWorld = safeMousePart->getCoordinateFrame().pointToWorldSpace(pointOnMousePart);
			Vector3 rayToPart = mousePartWorld - unitMouseRay.origin();
			float distanceToPart = rayToPart.magnitude();
			Vector3 rayToSameDistance = unitMouseRay.direction() * distanceToPart;
			float movementInWorld = (rayToPart - rayToSameDistance).magnitude();
			float breakFreeDis = breakFreeDistance();

			if (movementInWorld > breakFreeDis) {	
				Vector3 hitPointOnBackground;
				if (getSnapHitPoint(safeMousePart.get(), unitMouseRay, hitPointOnBackground)) 
				{
					hitPointOffset = mousePartWorld - hitPointOnBackground;  // stays constant during the drag operation
					hitPointHeight = 0.0f;	// init
					startDragging();
				}
			}
		}
	}
}

void AdvLuaDragger::startDragging()
{
	if(shared_ptr<PartInstance> safeMousePart = mousePart.lock()){
		RBXASSERT(Workspace::getWorkspaceIfInWorkspace(safeMousePart.get()));

		DragUtilities::unJoinFromOutsiders(dragParts);				// destroy joints outside the parts....

		DragUtilities::setDragging(dragParts);
		
		DragUtilities::clean(dragParts);

		dragPhase = DRAGGING;

		advRunDragger.reset(new AdvRunDragger());	
		advRunDragger->initLocal(	Workspace::findWorkspace(safeMousePart.get()),
								    mousePart, 
								    pointOnMousePart,
                                    dragParts);
	}
}

void AdvLuaDragger::doDrag(const RbxRay& unitMouseRay)
{
	FASTLOG(FLog::DragProfile, "AdvLuaDragger - doDrag");

	RBXASSERT(dragPhase == DRAGGING);
	if (shared_ptr<PartInstance> safeMousePart = mousePart.lock()){
		if (Workspace::getWorkspaceIfInWorkspace(safeMousePart.get())) 
		{
			if( dragParts.size() == 1 )
			{
				if (advRunDragger.get()) {							// single part move
					advRunDragger->snap(unitMouseRay);
				}
			}
			else 
			{
				if (AdvRunDragger::dragMultiPartsAsSinglePart)
				{
					if (advRunDragger.get()) {	
						//std::clock_t start = std::clock();
						advRunDragger->snap(unitMouseRay);
						//RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Time to snap (multiple part new): %lf", ( std::clock() - start ) / (double) CLOCKS_PER_SEC);
					}	
				}
				else
				{
					FASTLOG1(FLog::DragProfile, "Group drag, part: %p", safeMousePart.get());
#ifndef DISABLE_GROUP_SNAP
					if (advRunDragger.get()) {						
						advRunDragger->snapGroup(unitMouseRay);
					}
#else
					Vector3 hitSnapped;
					if (getSnapHitPoint(safeMousePart.get(), unitMouseRay, hitSnapped)) 
					{
						Vector3 currentLoc = safeMousePart->getCoordinateFrame().translation;

						// uncomment this to have the hitpoint project on the y-axis down to the drag surface
						// instead of using the mouse ray intersection with the drag surface
						// it is commented for now since there are a couple edge cases that do not behave ideally (Tim L)
						// hitSnapped += hitPointOffset;
						currentLoc = safeMousePart->getCoordinateFrame().pointToWorldSpace(pointOnMousePart);

						Vector3 currentBottom = currentLoc - Vector3(0.0f, hitPointHeight, 0.0f);
						Vector3 delta = DragUtilities::toGrid(hitSnapped - currentBottom);

						FASTLOG3F(FLog::DragProfile, "Delta, %fx%fx%f", delta.x, delta.y, delta.z);

						if (delta != Vector3::zero()) 
						{	
							Vector3 hitLocation;
							if (DragUtilities::hitObject(dragParts, unitMouseRay, getContactManager(safeMousePart.get()), hitLocation))
								DragUtilities::safeMoveYDrop(dragParts, delta, getContactManager(safeMousePart.get()), hitLocation.y);
							else
								DragUtilities::safeMoveYDrop(dragParts, delta, getContactManager(safeMousePart.get()), Dragger::groundPlaneDepth());

							CoordinateFrame finalPosition = safeMousePart->getCoordinateFrame();

							size_t coordinateFrameOffset = 0;
							for (WeakParts::const_iterator iter = dragParts.begin(); iter != dragParts.end() && coordinateFrameOffset < m_originalPositions.size(); ++iter)
							{
								if (shared_ptr<PartInstance> partInstance = iter->lock())
									partInstance->setCoordinateFrame(finalPosition * m_originalPositions[coordinateFrameOffset]);

								++coordinateFrameOffset;
							}

							float requestedUpMovement = delta.y;
							float actualUpMovement = finalPosition.translation.y - currentLoc.y;
							float error = actualUpMovement - requestedUpMovement;
							if (error != 0.0f) {
								hitPointHeight += error;
							}
						}

						FASTLOG(FLog::DragProfile, "Done dragging");
					}
#endif
				}
			}
		}
	}
}

bool AdvLuaDragger::getSnapHitPoint(PartInstance* part, const RbxRay& unitMouseRay, Vector3& hitPoint)
{
	if (Math::intersectRayPlane(MouseCommand::getSearchRay(unitMouseRay),
								Plane(Vector3::unitY(), Vector3::zero()), 
								hitPoint)) {
		hitPoint = DragUtilities::hitObjectOrPlane(	dragParts, 
													unitMouseRay, 
													getContactManager(part)		);

		hitPoint = DragUtilities::toGrid(hitPoint);
		return true;
	}
	else {
		return false;
	}
}

void AdvLuaDragger::axisRotate(Vector3::Axis axis)
{
	switch(axis)
	{
	case Vector3::X_AXIS:
		rotateOnSnapFace(Vector3::X_AXIS,Math::matrixRotateX());
		break;
	case Vector3::Y_AXIS:
		rotateOnSnapFace(Vector3::Y_AXIS,Math::matrixRotateY());
		break;
	case Vector3::Z_AXIS:
		rotateOnSnapFace(Vector3::Z_AXIS,Math::matrixTiltZ());
		break;
	default:
        RBXASSERT(false);
		break;
	}
}

void AdvLuaDragger::rotateOnSnapFace(Vector3::Axis axis, const Matrix3& rotMatrix)
{
	if (dragPhase == DRAGGING){
		if(shared_ptr<PartInstance> safeMousePart = mousePart.lock()){
			if(Workspace::getWorkspaceIfInWorkspace(safeMousePart.get())){
				G3D::Array<Primitive*> primitives;
				DragUtilities::partsToPrimitives(dragParts, primitives);
				RBXASSERT(primitives.size() > 0);
				
				if( primitives.size() == 1 )
				{
					// use the single part run dragger
					advRunDragger.get()->rotatePart90DegAboutSnapFaceAxis(axis);
				}
				else
					Dragger::safeRotate(primitives, rotMatrix, getContactManager(safeMousePart.get()));
			}
		}
	}
	else {
		RBXASSERT(0);
	}
}

const float AdvLuaDragger::breakFreeDistance( void )
{
	DRAG::DraggerGridMode mode = AdvArrowTool::advGridMode;
	switch(mode)
	{
	case DRAG::ONE_STUD:
	default:
		return 0.5f;
		break;
	case DRAG::QUARTER_STUD:
		return 0.1f;
		break;
	case DRAG::OFF:
		if (DFFlag::UnifyDragGridSizes)
			return 0.005f;
		else
			return 0.001f;
		break;
	}

}

void AdvLuaDragger::toggleRunDraggerJointCreateMode( void )
{
	if (dragPhase == DRAGGING)
	{
		if(shared_ptr<PartInstance> safeMousePart = mousePart.lock())
		{
			if(Workspace::getWorkspaceIfInWorkspace(safeMousePart.get()))
			{
				G3D::Array<Primitive*> primitives;
				DragUtilities::partsToPrimitives(dragParts, primitives);
				RBXASSERT(primitives.size() > 0);

				if( primitives.size() == 1 )
				{
					bool jointCreateOn = advRunDragger.get()->getJointCreateMode();
					if( jointCreateOn )
						advRunDragger.get()->setJointCreateMode(false);
					else
						advRunDragger.get()->setJointCreateMode(true);
				}
			}
		}
	}
}

void AdvLuaDragger::alignPartToGrid( void )
{
	// only for single part dragging
	if (dragPhase == DRAGGING)
	{
		if(shared_ptr<PartInstance> safeMousePart = mousePart.lock())
		{
			if(Workspace::getWorkspaceIfInWorkspace(safeMousePart.get()))
			{
				G3D::Array<Primitive*> primitives;
				DragUtilities::partsToPrimitives(dragParts, primitives);
				RBXASSERT(primitives.size() > 0);
			}
		}
	}
}


ContactManager& AdvLuaDragger::getContactManager(PartInstance* part)
{
	return *Workspace::findWorkspace(part)->getWorld()->getContactManager();
}


} // namespace
