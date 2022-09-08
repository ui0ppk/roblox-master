/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Tool/AdvMoveTool.h"

#include "V8DataModel/Workspace.h"
#include "V8DataModel/PartInstance.h"
#include "Tool/Dragger.h"
#include "V8World/World.h"
#include "V8World/ContactManager.h"
#include "Util/Math.h"
#include "Util/HitTest.h"
#include "Util/Sound.h"
#include "AppDraw/DrawAdorn.h"
#include "v8datamodel/changehistory.h"

// TODO - move rotation specific stuff to AdvRotateTool
// TODO - move generic stuff down to the Arrow layer to reduce some redundancy with
//  non-adv draggers

DYNAMIC_FASTFLAG(RestoreTransparencyOnToolChange)
FASTFLAG(StudioUseDraggerGrid)
DYNAMIC_FASTFLAGVARIABLE(DraggerUsesNewPartOnDuplicate, false)

namespace RBX {

const char* const   sAdvMoveTool        = "AdvMove";
static const float  MinimumExtentsSize  = 0.4f;

///////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////

AdvMoveToolBase::AdvMoveToolBase(Workspace* workspace) 
	: AdvArrowToolBase(workspace)
	, dragging(false)
	, cursor("advCursor-default")
	, overHandleNormalId(NORM_UNDEFINED)
    , mExtents(Vector3::zero(),Vector3::zero())
    , mMultiRotation(Matrix3::identity())
    , mInitializedExtents(false)
{
}


void AdvMoveToolBase::onMouseHover(const shared_ptr<InputObject>& inputObject)
{
	Super::onMouseHover(inputObject);

	onMouseIdle(inputObject);
}


void AdvMoveToolBase::onMouseIdle(const shared_ptr<InputObject>& inputObject)
{	
	Super::onMouseIdle(inputObject);

	if (dragging) {
		;
	}
	else if (captured()) {
		cursor = "advClosed-hand";
	}
	else if (getOverHandle(inputObject)) {
		cursor = "advCursor-openedHand";
	}
	else {
		cursor = Super::getCursorName();
	}
}


shared_ptr<MouseCommand> AdvMoveToolBase::onMouseDown(const shared_ptr<InputObject>& inputObject)
{
	NormalId normalId;
	Vector3 tempPt;
    
    Extents extents;
    overHandleNormalId = NORM_UNDEFINED;
    CoordinateFrame location;
    bool isLocal = false;

    getExtentsAndLocation(extents, location, isLocal);

    if (getOverHandle(inputObject, tempPt, normalId))
    {
		if (DFFlag::DraggerUsesNewPartOnDuplicate)
		{
			RBX::Selection* selection = ServiceProvider::find<Selection>(workspace);
			if (selection)
				selectionChangedConnection = selection->selectionChanged.connect(boost::bind(&RBX::AdvMoveToolBase::setToSelection, this));
		}

        downPoint2d = inputObject->get2DPosition();
		dragAxis = (int)(normalId) % 3;
		dragAxisDirection = (normalId > 2) ? -1 : 1;
        dragNormalId = normalId;
        lastPoint3d = tempPt;

        dragRay = Ray::fromOriginAndDirection(lastPoint3d, location.vectorToWorldSpace(normalIdToVector3(normalId)));

		ServiceClient< FilteredSelection<Instance> > instanceSelection(workspace);
		PartArray partArray;
		DragUtilities::instancesToParts(instanceSelection->items(), partArray);
		megaDragger.reset(new MegaDragger(	
			NULL, 
			partArray, 
			workspace) );

		megaDragger->startDragging();
		//IMPORTANT: Transparency modification should be done only after startDragging as it will update world correctly else issues with ChangeHistory
		//if collision checks are turned off then make parts transparent after saving the original values
		if (!RBX::AdvArrowTool::advCollisionCheckMode)
			saveAndModifyPartsTransparency();

		capture();
		return shared_from(this);
	}
	else {
        return Super::onMouseDown(inputObject);		
	}
}

void AdvMoveToolBase::setToSelection()
{
	if (DFFlag::RestoreTransparencyOnToolChange)
		AdvArrowToolBase::restoreSavedPartsTransparency();
	megaDragger->setToSelection(workspace);
	if (DFFlag::RestoreTransparencyOnToolChange && !RBX::AdvArrowTool::advCollisionCheckMode)
		saveAndModifyPartsTransparency();
}

float AdvMoveToolBase::snapRotationAngle(float Angle) const
{
    if ( AdvArrowToolBase::advGridMode == DRAG::ONE_STUD )
    {
        float increment = Math::pif() / 4;
        Angle = floorf(Angle / increment) * increment;
    }
    else if ( AdvArrowToolBase::advGridMode == DRAG::QUARTER_STUD )
    {
        float increment = Math::pif() / 12;
        Angle = floorf(Angle / increment) * increment;
    }
    
    return Angle;
}

void AdvMoveToolBase::onMouseMove(const shared_ptr<InputObject>& inputObject)
{
	RBXASSERT(captured());
	Super::onMouseMove(inputObject);

	if (	!dragging 
		&&	((inputObject->get2DPosition() - downPoint2d).length() > 4))
	{
		megaDragger->alignAndCleanParts();
		dragging = true;
	}


	if (	dragging
		&&	megaDragger->anyDragPartAlive()) 
	{
        if ( getDragType() == HANDLE_MOVE )
        {
		    Vector3 closePoint = Math::closestPointOnRay(dragRay, getUnitMouseRay(inputObject));
			Vector3 delta = closePoint - lastPoint3d; // snap point if required
            bool isLocal = getLocalSpaceMode();
            if (isLocal)
            {
                delta = DragUtilities::toLocalGrid(delta);
            }
            else
            {
			delta = DragUtilities::toGrid(delta);
            }
            
		    if (delta != Vector3::zero()) 
            {
			    megaDragger->continueDragging();			// do this every time - multiplayer

				Vector3 move(delta);

				if (!RBX::AdvArrowTool::advCollisionCheckMode)
					megaDragger->moveAlongLine(delta);
				else
					move = megaDragger->safeMoveAlongLine(delta, !isLocal);
				
			    if (move != Vector3::zero())
                {
                    lastPoint3d = lastPoint3d + move;
                    
                    if (!isLocal)
                        lastPoint3d = DragUtilities::toGrid(lastPoint3d);
                }
		    }
        }
        else if ( getDragType() == HANDLE_ROTATE )
        {
            Extents         extents;
            CoordinateFrame owner_frame;
            bool            is_local = false;

            if ( !getExtentsAndLocation(extents,owner_frame,is_local) )
                return;

            // owner position and orientation

            const Vector3&  owner_translation   = owner_frame.translation;
            const Matrix3&  owner_rotation      = owner_frame.rotation;

            // compute pick ray

            const RbxRay    mouse_ray = getUnitMouseRay(inputObject);
            const Vector3   mouse_screen(inputObject->get2DPosition().x,inputObject->get2DPosition().y,0);

            // project owner to screen space

            const Camera&   camera          = *workspace->getConstCamera();
            const Vector3   owner_screen    = camera.project(owner_translation);

            // project adjusted pick ray to screen space

            const Vector3   screen_diff     = mouse_screen - owner_screen;
            const Vector3   mid_screen      = owner_screen + screen_diff / 2;
            const RbxRay    projection_ray  = camera.worldRay(mid_screen.x,mid_screen.y);
            const Vector3   axis_vector     = normalIdToVector3(dragNormalId);

            // determine plane normal

            Vector3 plane_normal;
            if ( is_local )
                plane_normal = normalize(owner_rotation * axis_vector);
            else
                plane_normal = axis_vector;

            // compute intersection of pick ray to axis plane

            Vector3 plane_hit;
            const Plane plane(plane_normal,owner_translation);
            Math::intersectRayPlane(projection_ray,plane,plane_hit);

            // compute new vector

            Vector3 new_vector = plane_hit - owner_translation;
            if ( Math::isNanInfVector3(new_vector) )
                return;

            // compute new rotation matrix from new vector

            Vector3 right;
            Vector3 up;
            Vector3 at;

            const NormalId pos_normal = intToNormalId(dragNormalId % 3);
            const int pos_neg = dragNormalId == pos_normal ? 1.0f : -1.0f;
            switch ( pos_normal )
            {
                case NORM_X:
                    right   = normalize(owner_rotation.column(0));
                    at      = normalize(pos_neg * new_vector);
                    up      = normalize(at.cross(right));
                    break;
                case NORM_Y:
                    up      = normalize(owner_rotation.column(1));
                    right   = normalize(pos_neg * new_vector);
                    at      = normalize(right.cross(up));
                    break;
                case NORM_Z:
                    at      = normalize(owner_rotation.column(2));
                    up      = normalize(pos_neg * new_vector);
                    right   = normalize(up.cross(at));
                    break;
                default:
                    RBXASSERT(false);
            }

            Matrix3 rotation(
                right.x,up.x,at.x,
                right.y,up.y,at.y,
                right.z,up.z,at.z );

            // compute delta rotation
            const Matrix3 rotate_by = owner_rotation.inverse() * rotation;

            // determine angle of rotation so that we can snap

            Vector3 rotation_axis;
            float rotation_angle;
            rotate_by.toAxisAngle(rotation_axis,rotation_angle);
            const float snaped_angle = snapRotationAngle(rotation_angle);

            // only rotate with a minimum angle

            if ( snaped_angle > 0.001f )
            {
                Matrix3 snapped_rotate = Matrix3::fromAxisAngle(rotation_axis,snaped_angle);
				snapped_rotate = megaDragger->rotateDragParts(snapped_rotate, RBX::AdvArrowTool::advCollisionCheckMode);

                if ( !is_local )
                {
                    mMultiRotation *= snapped_rotate;
                    Math::orthonormalizeIfNecessary(mMultiRotation);
                }
            }
        }
	}
}

shared_ptr<MouseCommand> AdvMoveToolBase::onMouseUp(const shared_ptr<InputObject>& inputObject)
{
	//restore transparency
	if (DFFlag::RestoreTransparencyOnToolChange)
		AdvArrowToolBase::restoreSavedPartsTransparency();
	else if (!RBX::AdvArrowTool::advCollisionCheckMode)
			restoreSavedPartsTransparency();

	//make sure if megadragger is present, in case of CTRL+Select, it doesn't get created
	if (megaDragger.get())
	{
		megaDragger->finishDragging();
		megaDragger.release();
	}

	if (DFFlag::DraggerUsesNewPartOnDuplicate && selectionChangedConnection.connected())
		selectionChangedConnection.disconnect();

	dragging = false;
	releaseCapture();

	//baseclass will take care of setting waypoint for undo/redo
	Super::onMouseUp(inputObject);

	return shared_ptr<MouseCommand>();
}

shared_ptr<MouseCommand> AdvMoveToolBase::onKeyDown(const shared_ptr<InputObject>& inputObject)
{
	Super::onKeyDown(inputObject);

	return shared_from(this);
}

void AdvMoveToolBase::render2d(Adorn* adorn)
{
    Extents worldExtents;
    CoordinateFrame location;
    bool isLocal = false;

    if (getExtentsAndLocation(worldExtents, location, isLocal))
    {
        RBX::DrawAdorn::handles2d(
            worldExtents.size(),
            location,
            *(workspace->getConstCamera()),
            adorn,
            HANDLE_MOVE,
            DrawAdorn::resizeColor());
        
        if (isLocal)
        {
            RBX::DrawAdorn::partInfoText2D(
                worldExtents.size(),
                location,
                *(workspace->getConstCamera()),
                adorn,
                "L",
                DrawAdorn::cornflowerblue);
        }
    }
}

void AdvMoveToolBase::render3dAdorn(Adorn* adorn)
{
    renderHoverOver(adorn);

	Extents worldExtents;
    CoordinateFrame location;
    bool isLocal = false;

    if (getExtentsAndLocation(worldExtents, location, isLocal))
    {
            RBX::DrawAdorn::handles3d(
                worldExtents.size(),
                location,
                adorn, 
                HANDLE_MOVE,
                workspace->getConstCamera()->coordinateFrame().translation,
                DrawAdorn::resizeColor(),
                true,
                NORM_ALL_MASK,
                overHandleNormalId,
                Color3(0.3f, 0.7f, 1.0f));
    }
}

bool AdvMoveToolBase::getExtentsAndLocation(
    Extents&            extents,
    CoordinateFrame&    location,
    bool&               isLocal ) const
{
    ServiceClient<Selection> selection(workspace);
    std::vector<Primitive*> primitives;

    std::for_each(
        selection->begin(), 
        selection->end(), 
        boost::bind(
            &DragUtilities::getPrimitives2, 
            _1, 
            boost::ref(primitives)) );

    if ( primitives.empty() )
        return false;

    if ( primitives.size() == 1 && getLocalSpaceMode())
    {
        isLocal = true;

        PartInstance* part = PartInstance::fromPrimitive(primitives[0]);
        if ( !part )
            return false;

        extents = part->getPartPrimitive()->getExtentsLocal();
        if ( extents.longestSide() < MinimumExtentsSize )
            extents = Extents::fromCenterRadius(extents.center(),MinimumExtentsSize / 2);

        location = part->getLocation();       
    }
    else
    {
        isLocal = false;

        if (getDragType() == HANDLE_ROTATE || getDragType() == HANDLE_MOVE)
        {
            Extents newExtents;
            CoordinateFrame primaryPartLocation;
			if (getDragType() == HANDLE_ROTATE)
			{
                PartInstance* part = Dragger::computePrimaryPart(primitives);
                primaryPartLocation = part->getCoordinateFrame();
                newExtents = Dragger::computeExtentsRelative(primitives, primaryPartLocation);
            }
            else
            {
                newExtents = Dragger::computeExtents(primitives);
            }
            // The minimum distance between two different world space pivots
            // to cause a new extents bounds to be calculated.
            const float minDistance = 0.01f;
            
            if ((dragging && getDragType() == HANDLE_MOVE) || !mInitializedExtents ||
                (newExtents.center() - mExtents.center()).length() > minDistance)
            {
                mInitializedExtents = true;
                mExtents = newExtents;
                if ( mExtents.longestSide() < MinimumExtentsSize )
                    mExtents = Extents::fromCenterRadius(mExtents.center(),MinimumExtentsSize / 2);   
            }

            extents              = mExtents;
            location.rotation    = mMultiRotation;
            if (getDragType() == HANDLE_ROTATE)
            {
                location.translation = primaryPartLocation.pointToWorldSpace(mExtents.center());
            }
            else
            {
                location.translation = mExtents.center();
            }
        }
        else
        {
            location.translation = extents.center();
            location.rotation    = Matrix3::identity();
        }
    }
    return true;
}

bool AdvMoveToolBase::getExtents(Extents& extents) const
{
	ServiceClient<Selection> selection(workspace);

	std::vector<Primitive*> primitives;

	std::for_each(
			selection->begin(), 
			selection->end(), 
			boost::bind(	&DragUtilities::getPrimitives2, 
							_1, 
							boost::ref(primitives))
							);

	if (primitives.size() > 0) 
	{
		extents = Dragger::computeExtents(primitives);
		return true;
	}
	else {
		return false;
	}
}

bool AdvMoveToolBase::getOverHandle(const shared_ptr<InputObject>& inputObject) const
{
	Vector3 tempV;
	NormalId tempN;
	return getOverHandle(inputObject, tempV, tempN);
}

bool AdvMoveToolBase::getOverHandle(const shared_ptr<InputObject>& inputObject, Vector3& hitPointWorld, NormalId& normalId) const
{
    RBXASSERT(!captured());

    Extents extents;
    overHandleNormalId = NORM_UNDEFINED;
    CoordinateFrame location;
    bool isLocal = false;
    
    if (getExtentsAndLocation(extents, location, isLocal))
    {
        if (!isLocal)
            extents = Extents::fromCenterCorner(Vector3::zero(),extents.size() * 0.5f);

        if (HandleHitTest::hitTestHandleLocal(extents,
                                              location,
                                              HANDLE_MOVE,
                                              this->getUnitMouseRay(inputObject),
                                              hitPointWorld,
                                              normalId)) {
            overHandleNormalId = normalId;
            return true;
        }
    }
    return false;
}

bool AdvMoveToolBase::getLocalSpaceMode() const
{
    return AdvMoveTool::advLocalTranslationMode;
}

void AdvMoveToolBase::saveAndModifyPartsTransparency()
{
	PartArray allSelectedParts;
	ServiceClient< FilteredSelection<PVInstance> > selectedPVInstances(workspace);
	DragUtilities::pvsToParts(selectedPVInstances->items(), allSelectedParts);
	
	for (size_t ii = 0; ii < allSelectedParts.size(); ++ii)
	{
		const shared_ptr<PartInstance>& spPart = allSelectedParts[ii].lock();
		if (PartInstance::nonNullInWorkspace(spPart))
		{
			if (DFFlag::RestoreTransparencyOnToolChange)
				AdvArrowToolBase::savePartTransparency(spPart);
			else
			{
				origPartsTransparency[spPart] = spPart->getTransparencyUi();
				spPart->setTransparency(0.5f);
			}
		}
	}
}

void AdvMoveToolBase::restoreSavedPartsTransparency()
{
	for (PartsTransparencyCollection::const_iterator it = origPartsTransparency.begin(); it != origPartsTransparency.end(); ++it)
	{
		if (!it->first.expired())
			it->first.lock()->setTransparency(it->second);
	}

	origPartsTransparency.clear();
}

shared_ptr<MouseCommand> AdvMoveTool::onMouseDown(const shared_ptr<InputObject>& inputObject)
{	
	ServiceClient<Selection> selection(workspace);

	std::vector<Primitive*> primitives;
	std::for_each(selection->begin(), selection->end(), boost::bind(&DragUtilities::getPrimitives2,_1, boost::ref(primitives)));
	if( primitives.size() > 0 )
		originalLocation = primitives[0]->getCoordinateFrame().translation;

	return AdvMoveToolBase::onMouseDown(inputObject);
}

void AdvMoveTool::render2d(Adorn* adorn)
{
	AdvMoveToolBase::render2d(adorn);

	if (FFlag::StudioUseDraggerGrid && showDraggerGrid && dragging && (AdvArrowToolBase::advGridMode != DRAG::OFF))
	{
		ServiceClient<Selection> selection(workspace);
		std::vector<Primitive*> primitives;
		std::for_each(selection->begin(), selection->end(), boost::bind(&DragUtilities::getPrimitives2, _1, boost::ref(primitives)));

		if (primitives.size() > 0) 
		{
			G3D::Color3	gridColor = Color3::gray();

			int boxesPerStud = 1;
			if( AdvArrowToolBase::advGridMode == DRAG::QUARTER_STUD )
				boxesPerStud = 5;
						
			Extents extents = Dragger::computeExtents(primitives);
			float effectiveRadius = extents.longestSide();
			Vector4 bounds(effectiveRadius, effectiveRadius, -effectiveRadius, -effectiveRadius);

			//compute the best plane for grid drawing based upon camera direction
			PartInstance* part = PartInstance::fromPrimitive(primitives[0]);	
			G3D::Vector3 gridXDir, gridYDir;
			getGridXYUsingCamera(part, gridXDir, gridYDir);

			CoordinateFrame cordFrame(originalLocation);
			DrawAdorn::surfaceGridAtCoord(adorn, cordFrame, bounds, gridXDir, gridYDir, gridColor, boxesPerStud);
		}
	}
}

void AdvMoveTool::getGridXYUsingCamera(RBX::PartInstance *part, G3D::Vector3 &gridXDir, G3D::Vector3 &gridYDir)
{
	Vector3 cameraDirection = workspace->getConstCamera()->getCameraCoordinateFrame().vectorToWorldSpace(Vector3::unitZ());
	Vector3 bodyXWorld      = part->getCoordinateFrame().vectorToWorldSpace(Vector3::unitX());
	Vector3 bodyYWorld      = part->getCoordinateFrame().vectorToWorldSpace(Vector3::unitY());
	Vector3 bodyZWorld      = part->getCoordinateFrame().vectorToWorldSpace(Vector3::unitZ());

	switch(dragAxis)
	{
	case NORM_X:
	case NORM_X_NEG:		
		if(fabs(bodyZWorld.dot(cameraDirection)) > fabs(bodyYWorld.dot(cameraDirection)))
		{
			gridXDir = Vector3::unitX(); 
			gridYDir = Vector3::unitY();
		}
		else
		{
			gridXDir = Vector3::unitZ(); 
			gridYDir = Vector3::unitX();
		}
		break;
	case NORM_Y:
	case NORM_Y_NEG:		
		if( fabs(bodyZWorld.dot(cameraDirection)) > fabs(bodyXWorld.dot(cameraDirection)) )
		{
			gridXDir = Vector3::unitX(); 
			gridYDir = Vector3::unitY();
		}
		else
		{
			gridXDir = Vector3::unitY(); 
			gridYDir = Vector3::unitZ();
		}
		break;
	case NORM_Z:
	case NORM_Z_NEG:		
		if( fabs(bodyYWorld.dot(cameraDirection)) > fabs(bodyXWorld.dot(cameraDirection)) )
		{
			gridXDir = Vector3::unitZ();
			gridYDir = Vector3::unitX();
		}
		else
		{
			gridXDir = Vector3::unitY(); 
			gridYDir = Vector3::unitZ();
		}
		break;					
	}
}

} // namespace
