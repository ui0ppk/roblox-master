/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#include "AppDraw/Draw.h"
#include "AppDraw/DrawPrimitives.h"
#include "AppDraw/DrawAdorn.h"
#include "GfxBase/Part.h"
#include "GfxBase/Adorn.h"
#include "Util/Math.h"
#include "V8DataModel/PartInstance.h"

namespace RBX {

static const int SelectionBoxLineThreshold = 1500;  //!< tweakable threshold before boxes switch to lines

bool Draw::m_showHoverOver = true;
G3D::Color4 Draw::m_hoverOverColor(0.7f,0.9f,1.0f,1.0f);    // 178, 229, 255
G3D::Color4 Draw::m_selectColor(0.1f,0.6f,1.0f,1.0f);       //  25, 153, 256

void Draw::partAdorn(
	const Part&					part, 
	Adorn*						adorn,
	const G3D::Color3&			controllerColor)
{
	adornSurfaces(part, adorn, controllerColor);
}

/**
 * Set color used for the selection box when a part is selected.
 */
void Draw::setSelectColor(const G3D::Color4& Color)
{
    m_selectColor = Color;
}

/**
 * Set the color used for the selection box when a part is mouse hover over.
 */
void Draw::setHoverOverColor(const G3D::Color4& color)
{
    m_hoverOverColor = color;
}

void Draw::selectionBox(	
	const Part&				part, 
	Adorn*					adorn,
	SelectState				selectState,
	float					lineThickness)
{
    if ( selectState == SELECT_HOVER && m_showHoverOver )
    {
        adorn->setObjectToWorldMatrix(part.coordinateFrame);
        selectionBox(
            part,
            adorn,
            m_hoverOverColor,
            lineThickness * 2 );
    } 
    else if ( selectState != SELECT_HOVER )
    {
	    selectionBox(
            part, 
            adorn, 
            (selectState == SELECT_NORMAL) ? selectColor() : Color3::orange(), 
            lineThickness );
    }
}

void Draw::selectionBox(	
	ModelInstance&				model, 
	Adorn*						adorn,
	const G3D::Color4&			selectColor,
	float						lineThickness)
{
	selectionBox(model.computePart(), adorn, selectColor, lineThickness);
}

void Draw::adornSurfaces(const Part& part, 
						 Adorn* adorn, 
						 const G3D::Color3& controllerColor) 
{
	for (int face = 0; face < 6; ++face) {
		switch (part.surfaceType[face]) 
		{
		default:	break;
		case ROTATE:
		case ROTATE_P:
		case ROTATE_V:		Draw::constraint(part, adorn, face, controllerColor);	break;
		}
	}
}

void Draw::constraint(const Part& part, 
					  Adorn* adorn, 
					  int face,
					  const G3D::Color3& controllerColor) 
{
	SurfaceType surfaceType = part.surfaceType[face];

	debugAssert(	(surfaceType == ROTATE) 
				||	(surfaceType == ROTATE_P) 
				||	(surfaceType == ROTATE_V));

	Vector3 halfSize = part.gridSize * 0.5;
	
	const Matrix3& relativeRotation = Math::getAxisRotationMatrix(face);		
	Vector3 relativeTranslation;

	int normal = face % 3;
	float posNeg = face > 2 ? -1.0f : 1.0f;
	relativeTranslation[normal] = halfSize[normal] * posNeg;

	CoordinateFrame translation(relativeRotation, relativeTranslation);
	CoordinateFrame newObject = part.coordinateFrame * translation;

	adorn->setObjectToWorldMatrix(newObject);

	float axis = 1.0f;	
	float radius = 0.2f;
	adorn->cylinderAlongX(radius, axis, Color3::yellow());

	if ((surfaceType == ROTATE_V) || (surfaceType == ROTATE_P))	
	{
		float axis = 0.25f;	
		float radius = 0.4f;					
		adorn->cylinderAlongX(radius, axis, controllerColor);
	}
}

void Draw::selectionBox(	
	const Part&					part, 
	Adorn*						adorn,
	const G3D::Color4&			selectColor,
	float						lineThickness)
{
    if ( adorn->getCamera() != NULL )
    {
        // determine distance from camera to object
        const Vector3 camera_pos        = adorn->getCamera()->getCameraCoordinateFrame().translation;
        const Vector3 delta             = part.coordinateFrame.translation - camera_pos;
        const float   delta_squared     = delta.squaredLength();

        // determine size of object by using longest axis
        const float  grid_size          = part.gridSize[part.gridSize.primaryAxis()];
        const float  grid_size_squared  = grid_size * grid_size;

        // make sure the distance is greater than the size
        if ( delta_squared > grid_size_squared )
        {
            // adjust distance based on thickness of lines
            const float threshold_dist = SelectionBoxLineThreshold * lineThickness;
            const float threshold_dist_squared = threshold_dist * threshold_dist;

            // subtract the size from the distance and compare to the threshold
		    if ( delta_squared - grid_size_squared > threshold_dist_squared )
            {
                // switch to drawing lines
                const Vector3 half_size = part.gridSize / 2;
                adorn->setObjectToWorldMatrix(part.coordinateFrame);
                DrawAdorn::outlineBox(adorn,Extents(-half_size,half_size),selectColor);
                return;
            }
        }
    }

	adorn->setObjectToWorldMatrix( part.coordinateFrame );

	Vector3 halfSize = 0.5f * part.gridSize;

	float highlight = lineThickness;

	for (int c1 = 0; c1 < 3; ++c1) {			// x, y, z
		int c2 = (c1 + 1) % 3;					// y
		int c3 = (c1 + 2) % 3;					// z
		for (int d2 = -1; d2 < 2; d2+=2) {
		for (int d3 = -1; d3 < 2; d3+=2) {
			
			// for c1, shorten the length of the edge segment by highlight size
			// on both ends so that the edge segments do not overlap on the
			// corners
			Vector3 p0, p1;
			p0[c1] = -halfSize[c1] - (c1 == 0 ? highlight : -highlight);
			p0[c2] = d2 * halfSize[c2] - highlight;
			p0[c3] = d3 * halfSize[c3] - highlight;
			p1[c1] = halfSize[c1] + (c1 == 0 ? highlight : -highlight);
			p1[c2] = d2 * halfSize[c2] + highlight;
			p1[c3] = d3 * halfSize[c3] + highlight;

			if (p0.x <= p1.x && p0.y <= p1.y && p0.z <= p1.z)
				adorn->box(	AABox(p0, p1),
							selectColor	);
		}
		}
	}
}

} // namespace
