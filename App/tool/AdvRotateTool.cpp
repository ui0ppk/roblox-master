/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Tool/AdvRotateTool.h"
#include "Tool/Dragger.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/PartInstance.h"
#include "Util/HitTest.h"
#include "AppDraw/HitTest.h"

namespace RBX {

const char* const sAdvRotateTool    = "AdvRotate";
static const int  OffStudBoxes      = 2;
static const int  OneStudBoxes      = 4;       // 45 degrees
static const int  QuarterStudBoxes  = 12;      // 15 degrees

void AdvRotateTool::render2d(Adorn* adorn)
{
    Extents         extents;
    CoordinateFrame coordFrame;
    bool            isLocal;

    if ( !getExtentsAndLocation(extents,coordFrame,isLocal) )
        return;

    // isLocal ? HANDLE_FLAG_LOCAL_SPACE : HANDLE_FLAG_NONE,

	if (!dragging)
    {
		RBX::DrawAdorn::handles2d(extents.size(),
            coordFrame,
            *(workspace->getConstCamera()),
            adorn,
            HANDLE_ROTATE,
            Color3::green());
        
        if (isLocal)
        {
            RBX::DrawAdorn::partInfoText2D(
                extents.size(),
                coordFrame,
                *(workspace->getConstCamera()),
                adorn,
                "L",
                RBX::DrawAdorn::cornflowerblue);
        }
    }

    NormalId grid_normal = NORM_UNDEFINED;
    if ( dragging )
        grid_normal = dragNormalId;
    else
        grid_normal = mOverHandleNormalId;

    if ( grid_normal != NORM_UNDEFINED )
    {
        int boxesPerStud = 0;
        switch ( AdvMoveToolBase::advGridMode )
        {
            case DRAG::ONE_STUD:
                boxesPerStud = OneStudBoxes;
                break;
            case DRAG::QUARTER_STUD:
                boxesPerStud = QuarterStudBoxes;
                break;
            case DRAG::OFF:
                boxesPerStud = OffStudBoxes;
                break;
        }

        NormalId normalId = intToNormalId(grid_normal);

        DrawAdorn::circularGridAtCoord(
            adorn,
            coordFrame,
            extents.size(),
            workspace->getConstCamera()->coordinateFrame().translation,
            intToNormalId(grid_normal),
            Color4(DrawAdorn::axisColors[normalId % 3], 0.28f),
            boxesPerStud );
    }
}

void AdvRotateTool::render3dAdorn(Adorn* adorn)
{	
    renderHoverOver(adorn);

	Extents         extents;
    CoordinateFrame coordFrame;
    bool            isLocal;

    if ( getExtentsAndLocation(extents,coordFrame,isLocal) )
    {
        if ( !isLocal )
        {
            adorn->setObjectToWorldMatrix(Vector3::zero());
            DrawAdorn::star(
                adorn,
                extents.center(),
                3.0f,
                Color3::yellow(),
                Color3::yellow(),
                Color3::yellow() );

            Extents real_extents;
            getExtents(real_extents);
            DrawAdorn::outlineBox(adorn,real_extents,DrawAdorn::cornflowerblue);
        }

		DrawAdorn::handles3d(
			extents.size(),
			coordFrame,
			adorn,
			HANDLE_ROTATE,
			workspace->getConstCamera()->coordinateFrame().translation,
			getHandleColor(),
            true,
			getNormalMask(),
			mOverHandleNormalId);
    }
}

int AdvRotateTool::getNormalMask() const
{
    if ( dragging )
        return normalIdToMask(dragNormalId);
    else
        return NORM_ALL_MASK;
}

bool AdvRotateTool::getOverHandle(const shared_ptr<InputObject>& inputObject, Vector3& hitPointWorld, NormalId& normalId) const
{
	RBXASSERT(!captured());

	Extents         extents;
    CoordinateFrame coordFrame;
    bool            isLocal;
    bool            result = false;

    mOverHandleNormalId = NORM_UNDEFINED;

    if ( !getExtentsAndLocation(extents,coordFrame,isLocal) )
        return result;

	if ( !isLocal )
        extents = Extents::fromCenterCorner(Vector3::zero(),extents.size() * 0.5f);

    bool is_hit = HandleHitTest::hitTestHandleLocal(
        extents,
        coordFrame,
        HANDLE_ROTATE,
        getUnitMouseRay(inputObject),
        hitPointWorld,
        normalId,
        getNormalMask() );

    if ( is_hit )
        mOverHandleNormalId = normalId;

    return is_hit;
}

bool AdvRotateTool::getLocalSpaceMode() const
{
    return AdvMoveTool::advLocalRotationMode;
}

} // namespace