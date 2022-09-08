/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Tool/LuaDragger.h"
#include "Tool/DragUtilities.h"
#include "Tool/Dragger.h"
#include "Tool/RunDragger.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/MouseCommand.h"
#include "V8DataModel/GameBasicSettings.h"
#include "V8World/ContactManager.h"
#include "V8World/World.h"
#include "V8World/Joint.h"
#include "Tool/DragTypes.h"




namespace RBX {

const char* const sLuaDragger = "Dragger";

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<LuaDragger, void(shared_ptr<Instance>, Vector3, shared_ptr<const Instances>)> func_MouseDown(&LuaDragger::mouseDownPublic, "MouseDown", "mousePart", "pointOnMousePart", "parts", Security::None);
static Reflection::BoundFuncDesc<LuaDragger, void(RBX::RbxRay)> func_MouseMove(&LuaDragger::mouseMove, "MouseMove", "mouseRay", Security::None);
static Reflection::BoundFuncDesc<LuaDragger, void()> func_MouseUp(&LuaDragger::mouseUp, "MouseUp", Security::None);
static Reflection::BoundFuncDesc<LuaDragger, void(RBX::Vector3::Axis)> func_AxisRotate(&LuaDragger::axisRotate, "AxisRotate", "axis", RBX::Vector3::X_AXIS, Security::None);
REFLECTION_END();

LuaDragger::LuaDragger()
	: DescribedCreatable<LuaDragger, Instance, sLuaDragger>(sLuaDragger)
	, dragPhase(NO_PARTS)
	, hitPointHeight(0.0f)
{
	lockParent();
}

LuaDragger::~LuaDragger()
{
	RBX::GameBasicSettings::singleton().setCanMousePan(true);
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

void LuaDragger::mouseDownPublic(shared_ptr<Instance> _mousePart,
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


void LuaDragger::mouseDown(shared_ptr<PartInstance> _mousePart,
						   const Vector3& _pointOnMousePart,
						   const std::vector<weak_ptr<PartInstance> > _dragParts)
{
	RBXASSERT(Workspace::getWorkspaceIfInWorkspace(_mousePart.get()));

	RBX::GameBasicSettings::singleton().setCanMousePan(false);

	if ((dragPhase == MOUSE_DOWN) || (dragPhase == DRAGGING)) {
		throw std::runtime_error("Call to LuaDragger::mouseDown when already dragging");
	}

	RBXASSERT(jointsIMade.size() == 0);
	RBXASSERT(runDragger.get() == NULL);

	dragParts = _dragParts;
	mousePart = _mousePart;
	pointOnMousePart = _pointOnMousePart;

	dragPhase = MOUSE_DOWN;
}

void LuaDragger::mouseMove(RbxRay mouseRay)
{
	RbxRay unitRay = mouseRay.unit();

	if (dragPhase == NO_PARTS) {
		throw std::runtime_error("Call to LuaDragger::mouseMove without mouseDown");
	}
	else if ((dragPhase == MOUSE_UP_DRAGGED) || (dragPhase == MOUSE_UP_NO_DRAG)) {
		throw std::runtime_error("Call to LuaDragger::mouseMove after mouseUp");
	}
	else if (dragPhase == MOUSE_DOWN) {		
		tryStartDragging(unitRay);		// can change phase to DRAGGING
	}

	if (dragPhase == DRAGGING)
	{
		if(RBX::GameBasicSettings::singleton().inMousepanMode())
			RBX::GameBasicSettings::singleton().setCanMousePan(false);
		doDrag(unitRay);
	}
	else if(!RBX::GameBasicSettings::singleton().getCanMousePan())
		RBX::GameBasicSettings::singleton().setCanMousePan(true);
}

void LuaDragger::mouseUp()
{
	RBX::GameBasicSettings::singleton().setCanMousePan(true);
	if (dragPhase == DRAGGING) {

		DragUtilities::joinToOutsiders(dragParts);
				
		DragUtilities::stopDragging(dragParts);
		runDragger.reset();

		dragPhase = MOUSE_UP_DRAGGED;
		


	}
	else if (dragPhase == MOUSE_DOWN) {
		dragPhase = MOUSE_UP_NO_DRAG;
	}
	else {
		throw std::runtime_error("Call to LuaDragger::mouseUp without mouseDown");
	}

	RBXASSERT(jointsIMade.size() == 0);
}

void LuaDragger::tryStartDragging(const RbxRay& unitMouseRay)
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
				Vector3 temp;
				if (getSnapHitPoint(safeMousePart.get(), unitMouseRay, temp)) {
					hitPointHeight = 0.0f;	// init
					startDragging();
				}
			}
		}
	}
}

void LuaDragger::startDragging()
{
	if(shared_ptr<PartInstance> safeMousePart = mousePart.lock()){
		RBXASSERT(Workspace::getWorkspaceIfInWorkspace(safeMousePart.get()));

		DragUtilities::unJoinFromOutsiders(dragParts);				// destroy joints outside the parts....

		DragUtilities::setDragging(dragParts);
		
		if (!safeMousePart->aligned()) {								// Align the dragging part to the grid - snap all other parts
			CoordinateFrame orgCoord = safeMousePart->worldSnapLocation();
			CoordinateFrame alignedCoord = Math::snapToGrid(orgCoord, Dragger::dragSnap());
			DragUtilities::move(dragParts, orgCoord, alignedCoord);
		}
		DragUtilities::clean(dragParts);

		dragPhase = DRAGGING;

		if (dragParts.size() == 1) {
			runDragger.reset(new RunDragger());	
			runDragger->initLocal(	Workspace::findWorkspace(safeMousePart.get()),
									mousePart, 
									pointOnMousePart);
		}
	}
}

void LuaDragger::doDrag(const RbxRay& unitMouseRay)
{
	RBXASSERT(dragPhase == DRAGGING);
	if (shared_ptr<PartInstance> safeMousePart = mousePart.lock()){
		if (Workspace::getWorkspaceIfInWorkspace(safeMousePart.get())) 
		{
			if( dragParts.size() == 1 )
			{
				if (runDragger.get()) {							// single part move
					runDragger->snap(unitMouseRay);
				}
			}
			else 
			{
				Vector3 hitSnapped;
				if (getSnapHitPoint(safeMousePart.get(), unitMouseRay, hitSnapped)) {
					Vector3 currentLoc = safeMousePart->getCoordinateFrame().translation;
					Vector3 currentBottom = currentLoc - Vector3(0.0f, hitPointHeight, 0.0f);
					Vector3 delta = DragUtilities::toGrid(hitSnapped - currentBottom);
					if (delta != Vector3::zero()) {
						DragUtilities::safeMoveYDrop(dragParts, delta, getContactManager(safeMousePart.get()));
						Vector3 newLoc = safeMousePart->getCoordinateFrame().translation;
						float requestedUpMovement = delta.y;
						float actualUpMovement = newLoc.y - currentLoc.y;
						float error = actualUpMovement - requestedUpMovement;
						if (error != 0.0f) {
							hitPointHeight += error;
						}
					}
				}
			}
		}
	}
}

bool LuaDragger::getSnapHitPoint(PartInstance* part, const RbxRay& unitMouseRay, Vector3& hitPoint)
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

void LuaDragger::axisRotate(Vector3::Axis axis)
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

void LuaDragger::rotateOnSnapFace(Vector3::Axis axis, const Matrix3& rotMatrix)
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
					runDragger.get()->rotatePart90DegAboutSnapFaceAxis(axis);
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

void LuaDragger::alignPartToGrid( void )
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


ContactManager& LuaDragger::getContactManager(PartInstance* part)
{
	return *Workspace::findWorkspace(part)->getWorld()->getContactManager();
}


} // namespace
