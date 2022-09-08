/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#include "AppDraw/DrawAdorn.h"
#include "AppDraw/DrawPrimitives.h"
#include "GfxBase/Part.h"
#include "GfxBase/Adorn.h"
#include "AppDraw/HitTest.h"
#include "Util/Math.h"
#include "Util/Rect.h"
#include "Util/Extents.h"
#include "Util/HitTest.h"
#include "Util/Face.h"
#include "GfxBase/MeshGen.h"
#include "V8DataModel/Camera.h"
#include "V8DataModel/PartInstance.h"
#include "G3D/Color3uint8.h"

FASTFLAGVARIABLE(Studio3DGridUseAALines, true)

namespace RBX {

static const float ZEROPLANE_GRID_SIZE_BASE     = 400.0f;
static const float ZEROPLANE_GRID_FACTOR        = 8.0f;
static const float HandleOffset                 = 0.5f;
static const float RotationGridLineThickness    = 0.045f;
static const float RotationGridLineMinAngle     = Math::degreesToRadians(2.5f);
static const float TorusThickness               = 0.025f;
static const float TorusThicknessMinAngle       = Math::degreesToRadians(2.5f);
static const float SphereRadius                 = 0.5f;
static const float SphereMinAngle               = Math::degreesToRadians(2.0f);
static const float ArrowLength                  = 3.0f;
static const float ArrowMinAngle                = Math::degreesToRadians(3.5f);
static const float HandleTransparency           = 0.65f;
static const float AxisTransparency             = 0.28f;

const Color3 DrawAdorn::axisColors[3] =
{
    Color3::red(),
    Color3::green(),
    Color3::blue()
};

// common colors
const Color3 DrawAdorn::beige(G3D::Color3uint8(0xF5,0xF5,0xDC));
const Color3 DrawAdorn::darkblue(G3D::Color3uint8(0x00,0x00,0x8B));
const Color3 DrawAdorn::powderblue(G3D::Color3uint8(0xB0,0xE0,0xE6));
const Color3 DrawAdorn::skyblue(G3D::Color3uint8(0x87,0xCE,0xEB));
const Color3 DrawAdorn::violet(G3D::Color3uint8(0xEE,0x82,0xEE));
const Color3 DrawAdorn::slategray(G3D::Color3uint8(0x70,0x80,0x90));
const Color3 DrawAdorn::aqua(G3D::Color3uint8(0x00,0xFF,0xFF));
const Color3 DrawAdorn::tan(G3D::Color3uint8(0xD2,0xB4,0x8C));
const Color3 DrawAdorn::wheat(G3D::Color3uint8(0xF5,0xDE,0xB3));
const Color3 DrawAdorn::cornflowerblue(G3D::Color3uint8(0x64,0x95,0xED));
const Color3 DrawAdorn::limegreen(G3D::Color3uint8(0x32,0xCD,0x32));
const Color3 DrawAdorn::magenta(G3D::Color3uint8(0xFF,0x00,0xFF));
const Color3 DrawAdorn::pink(G3D::Color3uint8(0xFF,0xC0,0xCB));
const Color3 DrawAdorn::silver(G3D::Color3uint8(0xC0,0xC0,0xC0));

void DrawAdorn::cylinder(
	Adorn*						adorn,
	const						CoordinateFrame& worldC,		
	int							axis,
	float						length,
	float						radius,
	const Color4&				color,
	bool						cap)
{
	const Matrix3& relativeRotation = Math::getAxisRotationMatrix(axis);		
	CoordinateFrame rotatedWorldC(worldC.rotation * relativeRotation, worldC.translation);

	adorn->setObjectToWorldMatrix(rotatedWorldC);
	adorn->cylinderAlongX(radius, length, color, cap);
}



void DrawAdorn::faceInWorld(Adorn*			adorn,
							const Face&		face,
							float			thickness,
							const Color4&	color)
{
	Vector3 x = (face[1] - face[0]);
	Vector3 y = (face[3] - face[0]);
	Vector3 z = x.cross(y);
	Vector3 center = (face[0] + face[2]) * 0.5;

	CoordinateFrame c(center);

	c.rotation.setColumn(0, x.unit());
	c.rotation.setColumn(1, y.unit());
    c.rotation.setColumn(2, z.unit());

	adorn->setObjectToWorldMatrix(c);

	float dx = x.magnitude() * 0.5f;
	float dy = y.magnitude() * 0.5f;


	Extents extents(	Vector3(-dx, -dy, -thickness),		Vector3(dx, dy, thickness)	);
	adorn->box(extents, color);
}


void DrawAdorn::surfaceBorder(Adorn*			adorn,
							  const Vector3&	halfRealSize,
							  float				highlight,
							  int				surfaceId,
							  const Color4&		color)
{
	int c1 = surfaceId % 3;					// x, y, or z - primary
	float cZ = halfRealSize[c1] * ((surfaceId < 3) ? 1 : -1);
	Vector3 p0, p1;
	p0[c1] = cZ - highlight;
	p1[c1] = cZ + highlight;

	int c2 = (c1 + 1) % 3;					// y
	int c3 = (c1 + 2) % 3;					// z
	for (int direction = 0; direction < 2; ++direction) {
		int cY = direction ? c2 : c3;
		int cZ = direction ? c3 : c2;
		for (int polarity = -1; polarity <= 1; polarity += 2) {
            p0[cY] = polarity * halfRealSize[cY] - highlight;
            p1[cY] = polarity * halfRealSize[cY] + highlight;
			p0[cZ] = -halfRealSize[cZ] - highlight;
			p1[cZ] = halfRealSize[cZ] + highlight;

			AABox(p0, p1);

			adorn->box(AABox(p0, p1), color);
		}
	}
}

void DrawAdorn::surfaceGridOnFace(
							const Primitive&	prim,
							Adorn*				adorn,
							int					surfaceId,
							const Color4&		color,
							int					boxesPerStud )
{
	CoordinateFrame bodyCoord = prim.getCoordinateFrame() ;
	CoordinateFrame faceCoord = bodyCoord * prim.getConstGeometry()->getSurfaceCoordInBody(surfaceId);
		
	Vector4 bounds(-1e10f, -1e10f, 1e10f, 1e10f);
	for( int i = 0; i < prim.getConstGeometry()->getNumVertsInSurface(surfaceId); i++ )
	{
		Vector3 vertInBody = prim.getConstGeometry()->getSurfaceVertInBody(surfaceId, i);
		Vector3 vertInWorld = bodyCoord.pointToWorldSpace(vertInBody);
		Vector3 vertInFace = faceCoord.pointToObjectSpace(vertInWorld);

		// max vector
		if( vertInFace.x > bounds.x )
			bounds.x = vertInFace.x;
		if( vertInFace.y > bounds.y )
			bounds.y = vertInFace.y;

		// Min vector
		if( vertInFace.x < bounds.z )
			bounds.z = vertInFace.x;
		if( vertInFace.y < bounds.w )
			bounds.w = vertInFace.y;
	}
	surfaceGridAtCoord(adorn, faceCoord, bounds, Vector3::unitX(), Vector3::unitY(), color, boxesPerStud);
}


void DrawAdorn::zeroPlaneGrid(
	Adorn*						adorn,
	const RBX::Camera&			camera,
	const int					studsPerBox,
	const int					yLevel,
	const G3D::Color4&			smallGridColor,
	const G3D::Color4&			largeGridColor)
{
	const Vector3 cameraPos = camera.getCameraCoordinateFrame().translation;
	const Vector3 zeroPlaneCenter = G3D::Plane(Vector3(0,1,0),Vector3(0,yLevel,0)).closestPoint(cameraPos);
	const int x = (int) (Math::iRound((zeroPlaneCenter.x + studsPerBox/2.0f)/studsPerBox) * studsPerBox);
	const int z = (int) (Math::iRound((zeroPlaneCenter.z + studsPerBox/2.0f)/studsPerBox) * studsPerBox);
	const Vector3 zeroPlaneCenterOnGrid = Vector3(x,zeroPlaneCenter.y,z);
	const float distFromPlane = (cameraPos - zeroPlaneCenter).magnitude();
	const float lineThickness = 2;

	const int largeGridStep =  studsPerBox * ZEROPLANE_GRID_FACTOR;
	const int lineSegmentStep = largeGridStep;

    adorn->setObjectToWorldMatrix(Vector3::zero());

	// if we are close enough to the zero plane, lets render the smaller grid
	if(distFromPlane < ZEROPLANE_GRID_SIZE_BASE)
	{
		const float zEnd = ZEROPLANE_GRID_SIZE_BASE + zeroPlaneCenter.z;
		const float zStart = -ZEROPLANE_GRID_SIZE_BASE + zeroPlaneCenter.z;
		const float xEnd = ZEROPLANE_GRID_SIZE_BASE + zeroPlaneCenter.x;
		const float xStart = -ZEROPLANE_GRID_SIZE_BASE + zeroPlaneCenter.x;

		const int zeroPlaneSmallCenterX = Math::iRound(zeroPlaneCenterOnGrid.x/studsPerBox) * studsPerBox;
		const int zeroPlaneSmallCenterZ = Math::iRound(zeroPlaneCenterOnGrid.z/studsPerBox) * studsPerBox;

		for(int i = -ZEROPLANE_GRID_SIZE_BASE + zeroPlaneSmallCenterX; i < (ZEROPLANE_GRID_SIZE_BASE + zeroPlaneSmallCenterX); i += studsPerBox )
		{
			float lastStep = zStart;
			for(int j = zStart + lineSegmentStep; j <= zEnd + lineSegmentStep; j += lineSegmentStep)
			{
				const float cameraToPlaneDistance = (Vector3(i,zeroPlaneCenterOnGrid.y,j + (j - lastStep)/2.0f) - cameraPos).magnitude();
				const float alpha = std::max(1.0f - (cameraToPlaneDistance/ZEROPLANE_GRID_SIZE_BASE),0.0f);
				if(alpha > 0.0f)
				{
					if (FFlag::Studio3DGridUseAALines)
						adorn->line3dAA(Vector3(i,zeroPlaneCenterOnGrid.y,lastStep), 
										Vector3(i,zeroPlaneCenterOnGrid.y,j), 
										RBX::Color4(smallGridColor.rgb(),alpha),
										lineThickness, 0, false);
					else
						adorn->line3d(Vector3(i,zeroPlaneCenterOnGrid.y,lastStep), 
										Vector3(i,zeroPlaneCenterOnGrid.y,j), 
										RBX::Color4(smallGridColor.rgb(),alpha));
				}
				lastStep = j;
			}
		}
		for(int i = -ZEROPLANE_GRID_SIZE_BASE + zeroPlaneSmallCenterZ; i < (ZEROPLANE_GRID_SIZE_BASE + zeroPlaneSmallCenterZ); i += studsPerBox )
		{
			float lastStep = xStart;
			for(int j = xStart + lineSegmentStep; j <= xEnd + lineSegmentStep; j += lineSegmentStep)
			{
				const float cameraToPlaneDistance = (Vector3(j + (j - lastStep)/2.0f,zeroPlaneCenterOnGrid.y,i) - cameraPos).magnitude();
				const float alpha = std::max(1.0f - (cameraToPlaneDistance/ZEROPLANE_GRID_SIZE_BASE),0.0f);
				if(alpha > 0.0f)
				{
					if (FFlag::Studio3DGridUseAALines)
						adorn->line3dAA(Vector3(lastStep,zeroPlaneCenterOnGrid.y,i), 
										Vector3(j,zeroPlaneCenterOnGrid.y,i),
										RBX::Color4(smallGridColor.rgb(),alpha),
										lineThickness, 0, false);
					else
						adorn->line3d(Vector3(lastStep,zeroPlaneCenterOnGrid.y,i), 
										Vector3(j,zeroPlaneCenterOnGrid.y,i),
										RBX::Color4(smallGridColor.rgb(),alpha));
				}
				lastStep = j;
			}
		}
	}

	// now render the large grid
	const int zeroPlaneLargeCenterX = Math::iRound(zeroPlaneCenterOnGrid.x/largeGridStep) * largeGridStep;
	const int zeroPlaneLargeCenterZ = Math::iRound(zeroPlaneCenterOnGrid.z/largeGridStep) * largeGridStep;

	for(int i = -ZEROPLANE_GRID_SIZE_BASE + zeroPlaneLargeCenterX; i <= (ZEROPLANE_GRID_SIZE_BASE + zeroPlaneLargeCenterX); i += largeGridStep )
	{
		const Vector3 lineStart = Vector3(i,zeroPlaneCenterOnGrid.y,-ZEROPLANE_GRID_SIZE_BASE + zeroPlaneLargeCenterZ);
		const Vector3 lineEnd = Vector3(i,zeroPlaneCenterOnGrid.y,ZEROPLANE_GRID_SIZE_BASE + zeroPlaneLargeCenterZ);
		const Vector3 midPoint = lineEnd + ((lineEnd - lineStart) * 0.5f);
		const float distFromPlane = (cameraPos - midPoint).magnitude();

		const float largeGridAlpha = std::max(1.0f - ((distFromPlane * 1.5f)/(ZEROPLANE_GRID_SIZE_BASE)),0.25f);
		if (FFlag::Studio3DGridUseAALines)
			adorn->line3dAA(lineStart, 
							lineEnd, 
							RBX::Color4(largeGridColor.rgb(),largeGridAlpha),
							lineThickness, 0, false);
		else
			adorn->line3d(lineStart, 
							lineEnd, 
							RBX::Color4(largeGridColor.rgb(),largeGridAlpha));
	}
	for(int i = -ZEROPLANE_GRID_SIZE_BASE + zeroPlaneLargeCenterZ; i <= (ZEROPLANE_GRID_SIZE_BASE + zeroPlaneLargeCenterZ); i += largeGridStep )
	{
		const Vector3 lineStart = Vector3(i,zeroPlaneCenterOnGrid.y,-ZEROPLANE_GRID_SIZE_BASE + zeroPlaneLargeCenterZ);
		const Vector3 lineEnd = Vector3(i,zeroPlaneCenterOnGrid.y,ZEROPLANE_GRID_SIZE_BASE + zeroPlaneLargeCenterZ);
		const Vector3 midPoint = lineEnd + ((lineEnd - lineStart) * 0.5f);
		const float distFromPlane = (cameraPos - midPoint).magnitude();

		const float largeGridAlpha = std::max(1.0f - ((distFromPlane * 1.5f)/(ZEROPLANE_GRID_SIZE_BASE)),0.25f);
		if (FFlag::Studio3DGridUseAALines)
			adorn->line3dAA(Vector3(-ZEROPLANE_GRID_SIZE_BASE + zeroPlaneLargeCenterX,zeroPlaneCenterOnGrid.y,i),
							Vector3(ZEROPLANE_GRID_SIZE_BASE + zeroPlaneLargeCenterX,zeroPlaneCenterOnGrid.y,i), 
							RBX::Color4(largeGridColor.rgb(),largeGridAlpha),
							lineThickness, 0, false);
		else
			adorn->line3d(Vector3(-ZEROPLANE_GRID_SIZE_BASE + zeroPlaneLargeCenterX,zeroPlaneCenterOnGrid.y,i),
							Vector3(ZEROPLANE_GRID_SIZE_BASE + zeroPlaneLargeCenterX,zeroPlaneCenterOnGrid.y,i), 
							RBX::Color4(largeGridColor.rgb(),largeGridAlpha));
	}

    // last, we render the axes rays at the origin
    static const float ArrowSize = 4.0f;
    if ( camera.frustum().intersectsSphere(Vector3::zero(),ArrowSize) )
    {
        adorn->setMaterial(Adorn::Material_SelfLitHighlight);
        adorn->setObjectToWorldMatrix(CoordinateFrame(Vector3(0,0,0)));
        adorn->ray(RBX::RbxRay(Vector3(0,0,0),Vector3(ArrowSize,0,0)),axisColors[0]);
        adorn->ray(RBX::RbxRay(Vector3(0,0,0),Vector3(0,ArrowSize,0)),axisColors[1]);
        adorn->ray(RBX::RbxRay(Vector3(0,0,0),Vector3(0,0,ArrowSize)),axisColors[2]);
		adorn->setMaterial(Adorn::Material_Default);
    }
 }

void DrawAdorn::surfaceGridAtCoord(
	Adorn*						adorn,
	CoordinateFrame&			cF,
	const Vector4&				bounds,
	const Vector3&				gridXDir,
	const Vector3&				gridYDir,
	const G3D::Color4&			color,
	int							boxesPerStud )
{
	int direction1 = 1;
	int direction2 = 0;
	int xMaxInt = Math::iRound(bounds.x + 1);
	int yMaxInt = Math::iRound(bounds.y + 1);
	int xMinInt = Math::iRound(bounds.z - 1);
	int yMinInt = Math::iRound(bounds.w - 1);

	Matrix3 planeCoordRot = Math::fromDirectionCosines(gridXDir, gridYDir, gridXDir.cross(gridYDir), Vector3::unitX(), Vector3::unitY(), Vector3::unitZ());

	CoordinateFrame planeCoord = cF;
	planeCoord.rotation *= planeCoordRot;
	CoordinateFrame adornCoord = planeCoord;

	Vector3 ptXDir = Vector3::zero();
	ptXDir.y = 0.5f * (yMaxInt + yMinInt);
	for( int i = xMinInt; i <= xMaxInt; i++ )
	{
		if( boxesPerStud == 0 && i > xMinInt )
			i = xMaxInt;
		ptXDir.x = (float)i;
		Vector3 ptInWorld = planeCoord.pointToWorldSpace(ptXDir);
		adornCoord.translation = ptInWorld;
		cylinder(adorn, adornCoord, direction1, (float)(yMaxInt - yMinInt), 0.03, color);
		if( i < xMaxInt )
		{
			for( int j = 1; j < boxesPerStud; j++ )
			{
				ptXDir.x = (float)i + (1.0f/(float)boxesPerStud) * (float)j;
				ptInWorld = planeCoord.pointToWorldSpace(ptXDir);
				adornCoord.translation = ptInWorld;
				cylinder(adorn, adornCoord, direction1, (float)(yMaxInt - yMinInt), 0.01, color);
			}
		}
	}

	Vector3 ptYDir = Vector3::zero();
	ptYDir.x =  0.5f * (xMaxInt + xMinInt);
	for( int i = yMinInt; i <= yMaxInt; i++ )
	{
		if( boxesPerStud == 0 && i > yMinInt )
			i = yMaxInt;
		ptYDir.y = (float)i;
		Vector3 ptInWorld = planeCoord.pointToWorldSpace(ptYDir);
		adornCoord.translation = ptInWorld;
		cylinder(adorn, adornCoord, direction2, (float)(xMaxInt - xMinInt), 0.03, color);
		if( i < yMaxInt )
		{
			for( int j = 1; j < boxesPerStud; j++ )
			{
				ptYDir.y = (float)i + (1.0f/(float)boxesPerStud) * (float)j;
				ptInWorld = planeCoord.pointToWorldSpace(ptYDir);
				adornCoord.translation = ptInWorld;
				cylinder(adorn, adornCoord, direction2, (float)(xMaxInt - xMinInt), 0.01, color);
			}
		}
	}
}


void DrawAdorn::axisWidget(Adorn* adorn, const RBX::Camera& camera)
{
	Rect2D viewport = adorn->getViewport();

	float sizeOfWidget = 0.1f;
	Vector3 offset = Vector3((viewport.width()/2) - 50,(-viewport.height()/2) + 50,0);
	Vector3 widgetOrigin3dPos = camera.getRenderingCoordinateFrame().translation + (camera.getRenderingCoordinateFrame().lookVector() * 2);
	Vector3 widgetOrigin2dPos = camera.project(widgetOrigin3dPos) - offset;

	Vector3 widgetXPos = camera.project(widgetOrigin3dPos + Vector3::unitX() * sizeOfWidget) - offset;
	adorn->line2d(Vector2(widgetOrigin2dPos.x,widgetOrigin2dPos.y),Vector2(widgetXPos.x,widgetXPos.y),axisColors[0]);
	adorn->drawFont2D(
        "X",
        Vector2(widgetXPos.x,widgetXPos.y),
        12.0f,
        false,
        axisColors[0],
        RBX::Color4::clear(),
		RBX::Text::FONT_ARIAL );
    
    Vector3 widgetYPos = camera.project(widgetOrigin3dPos + Vector3::unitY() * sizeOfWidget) - offset;
	adorn->line2d(Vector2(widgetOrigin2dPos.x,widgetOrigin2dPos.y),Vector2(widgetYPos.x,widgetYPos.y),axisColors[1]);
	adorn->drawFont2D(
        "Y",
        Vector2(widgetYPos.x + 3.0f,widgetYPos.y),
        12.0f,
        false,
        axisColors[1],
		RBX::Color4::clear(),
		RBX::Text::FONT_ARIAL );

    Vector3 widgetZPos = camera.project(widgetOrigin3dPos + Vector3::unitZ() * sizeOfWidget) - offset;
	adorn->line2d(Vector2(widgetOrigin2dPos.x,widgetOrigin2dPos.y),Vector2(widgetZPos.x,widgetZPos.y),axisColors[2]);
	adorn->drawFont2D(
        "Z",
        Vector2(widgetZPos.x,widgetZPos.y),
        12.0f,
        false,
        axisColors[2],
        RBX::Color4::clear(),
		RBX::Text::FONT_ARIAL );
}

void DrawAdorn::circularGridAtCoord(
    Adorn*                  adorn,
    const CoordinateFrame&  coordFrame,
    const G3D::Vector3&		size,
	const G3D::Vector3&	    cameraPos,
    NormalId                normalId,
    const Color4&           color,
    int                     boxesPerStud )
{	
    const Vector3 halfSize           = size / 2;
    const Extents localExtents       = Extents(-halfSize,halfSize);
    const Vector3 handlePos          = handlePosInObject(
        cameraPos,
        localExtents,
        HANDLE_ROTATE,
        normalId );
    const Vector3 handlePosInWorld   = coordFrame.pointToWorldSpace(handlePos);
    const Vector3 delta              = coordFrame.translation - handlePosInWorld;
    const float   radius             = delta.length();
	const Vector3 normal             = normalIdToVector3(normalId);
    const int     direction          = (normalId == NORM_X || normalId == NORM_X_NEG ) ? 1 : 0;
    const float   rotationAngle      = Math::pif() / boxesPerStud;
    const Matrix3 rotation_increment = Matrix3::fromAxisAngle(normal,rotationAngle);
    const float   line_thickness     = scaleRelativeToCamera(
        cameraPos,
        coordFrame.translation,
        RotationGridLineMinAngle,
        RotationGridLineThickness );

	adorn->setMaterial(Adorn::Material_SelfLit);

    CoordinateFrame frame = coordFrame;
    for ( int i = 0 ; i < boxesPerStud ; ++i )
    {
        frame.rotation *= rotation_increment;
        cylinder(
            adorn,
            frame,
            direction,
            radius * 2,
            line_thickness,
            color );
    }
    
	adorn->setMaterial(Adorn::Material_Default);
}

void DrawAdorn::lineSegmentRelativeToCoord(
	Adorn*						adorn,
	const CoordinateFrame&		cF,
	const Vector3&      		pt0,
	const Vector3&      		pt1,
	const G3D::Color3&			color,
	float						lineThickness )
{
	CoordinateFrame lineCf;

	Vector3 lineVector = pt1 - pt0;
	lineCf.translation = pt0 + lineVector * 0.5f;
	float lineLength = lineVector.magnitude();
	lineCf.rotation = Math::getWellFormedRotForZVector(lineVector/lineLength);

	DrawAdorn::cylinder(adorn, cF*lineCf, 2, lineLength, lineThickness, color);
}

void DrawAdorn::verticalLineSegmentSplit(
	Adorn*						adorn,
	const CoordinateFrame&		cF,
	const Vector3&      		pt0,
	const Vector3&      		pt1,
	const float&      		    delta,
	const float&      		    dropFactor,
	const short&      		    level,
	const G3D::Color3&			color,
	float						lineThickness )
{
	short maxLevels = 3;
	float recursiveDropFactor = 0.25f;
	Vector3 splitPoint = 0.5 * (pt1 - pt0) + pt0;
	splitPoint.y -= dropFactor * delta;

	if( level < maxLevels )
	{
		verticalLineSegmentSplit(adorn, cF, pt0, splitPoint, delta, recursiveDropFactor*dropFactor, level+1, color, lineThickness);
		verticalLineSegmentSplit(adorn, cF, splitPoint, pt1, delta, recursiveDropFactor*dropFactor, level+1, color, lineThickness);
	}
	else
	{
		lineSegmentRelativeToCoord(adorn, cF, pt0, splitPoint, color, lineThickness);
		lineSegmentRelativeToCoord(adorn, cF, pt1, splitPoint, color, lineThickness);
	}

}

void DrawAdorn::surfacePolygon(
	Adorn*						adorn,
	PartInstance&				partInst,
	int							surfaceId,
	const G3D::Color3&			color,
	float						lineThickness )
{
	std::vector<Vector3> polygonInPart;
	for( int i = 0; i < PartInstance::getConstPrimitive(&partInst)->getConstGeometry()->getNumVertsInSurface(surfaceId); i++ )
	    polygonInPart.push_back(PartInstance::getConstPrimitive(&partInst)->getConstGeometry()->getSurfaceVertInBody(surfaceId, i));
	
    DrawAdorn::polygonRelativeToCoord(adorn, PartInstance::getConstPrimitive(&partInst)->getCoordinateFrame(), polygonInPart, color, lineThickness);
}

void DrawAdorn::polygonRelativeToCoord(
	Adorn*						adorn,
	const CoordinateFrame&		cF,
	std::vector<Vector3>&		vertices,
	const G3D::Color4&			color,
	float						lineThickness )
{
	CoordinateFrame lineCf;

	for(unsigned int i = 0; i < vertices.size(); i++ )
	{
		int next = i < vertices.size()-1 ? i + 1 : 0;
		Vector3 lineVector = vertices[next] - vertices[i];
		lineCf.translation = vertices[i] + lineVector * 0.5f;
		float lineLength = lineVector.magnitude();
		lineCf.rotation = Math::getWellFormedRotForZVector(lineVector/lineLength);

		cylinder(adorn, cF*lineCf, 2, lineLength, lineThickness, color);
	}
}


void DrawAdorn::partSurface(
	const Part&				part, 
	int						surfaceId,
	Adorn*					adorn,
	const Color4&			color,
	float					thickness)
{
	adorn->setObjectToWorldMatrix(part.coordinateFrame);

	Vector3 halfRealSize = 0.5f * part.gridSize;
	surfaceBorder(adorn, halfRealSize, thickness, surfaceId, color);
}

Vector3 DrawAdorn::handlePosInObject(
    const Vector3&  cameraPos,
    const Extents&  localExtents, 
    HandleType      handleType,
    NormalId        normalId )
{
    const Vector3 size          = localExtents.size();
    const Vector3 halfSize      = size / 2;

    switch ( handleType )
    {
        case HANDLE_ROTATE:
        {
            const float halfdiagonal    = halfSize.length();
            const float default_radius  = halfdiagonal + HandleOffset * 4;

            // TODO fix this so we can have a nice scaled circle far away
            //  use the default radius because if we scale it's gonna get huge
            //  need to determine circle size on screen and make sure it's larger
            //  than the minimum size on screen
            const float radius = default_radius;
//             const float radius = scaleRelativeToCamera(
//                 cameraPos,
//                 localExtents.center(),
//                 TorusMinAngle,
//                 default_radius );

            const Vector3 axis = normalIdToVector3(normalIdToU(normalId));
            return localExtents.center() + axis * radius;
        }
        case HANDLE_RESIZE:
        {
            const Vector3 axis = normalIdToVector3(normalId);
            const Vector3 start_offset(HandleOffset,HandleOffset,HandleOffset);
            return localExtents.center() + axis * (halfSize + start_offset * 4);
        }
        case HANDLE_VELOCITY:
        case HANDLE_MOVE:
        {
            const Vector3 axis = normalIdToVector3(normalId);
            const Vector3 start_offset(HandleOffset,HandleOffset,HandleOffset);
            return localExtents.center() + axis * (halfSize + start_offset);
        } 
        default:
            RBXASSERT(false);
            return Vector3::zero();
    }
}

float DrawAdorn::scaleHandleRelativeToCamera(
    const Vector3&  cameraPos,
    HandleType      handleType,
    const Vector3&  handlePos )
{
    float min_angle;
    float expected_radius;

    switch ( handleType )
    {
        case HANDLE_RESIZE:
            min_angle = SphereMinAngle;
            expected_radius = SphereRadius;
            break;
        case HANDLE_MOVE:
            min_angle = ArrowMinAngle;
            expected_radius = ArrowLength;
            break;
        case HANDLE_ROTATE:
            min_angle = SphereMinAngle;
            expected_radius = SphereRadius;
            break;
        case HANDLE_VELOCITY:
            min_angle = SphereMinAngle;
            expected_radius = SphereRadius;
            break;
    }

    return scaleRelativeToCamera(
        cameraPos,
        handlePos,
        min_angle,
        expected_radius );
}

// visual angle = radius/distance
float DrawAdorn::scaleRelativeToCamera(
    const Vector3&  cameraPos,
    const Vector3&  handlePos,
    float           minAngle,
    float           expectedSize )
{
	const float distance = (cameraPos - handlePos).magnitude();

    float scale = 1.0f;
    if ( distance > (1.0f / minAngle) ) 
        scale = distance * minAngle;

    const float radius = scale * expectedSize;
    return radius;
}

void DrawAdorn::handles2d(
	const G3D::Vector3&			size,
	const G3D::CoordinateFrame&	position,
	const RBX::Camera&			camera,		
	Adorn*						adorn,
	HandleType					handleType,
	const G3D::Color4&			color,
    bool                        useAxisColor,
	int							normalIdMask)
{
    Vector3 halfSize = size / 2;
    RBX::Extents localExtents(-halfSize,halfSize); 

    for (int posNeg = 0; posNeg < 2; ++posNeg) 
    {
        for (int xyz = 0; xyz < 3; ++xyz) 
        {
            if (normalIdMask & (1<<(xyz + (posNeg*3))))
            {
                NormalId normalId = (NormalId)(posNeg*3 + xyz);
                Vector3 handlePos = handlePosInObject(
                    camera.coordinateFrame().translation,
                    localExtents, 
                    handleType, 
                    normalId );

                Vector3 handlePosInWorld = position.pointToWorldSpace(handlePos);

                if (handleType == HANDLE_MOVE)
                {
                    // compute relative size on screen based on distance
                    const float handleRadius = scaleRelativeToCamera(
                        camera.coordinateFrame().translation,
                        handlePosInWorld,
                        ArrowMinAngle,
                        ArrowLength-0.5f);			

                    Vector3 correctedHandlePos = normalIdToVector3(normalId) * handleRadius;
                
                    //transform point back to world
                    CoordinateFrame frame(position.rotation,handlePosInWorld);
                    handlePosInWorld = frame.pointToWorldSpace(correctedHandlePos);
                }

                Vector3 screenPos = camera.project(handlePosInWorld);

                if (screenPos.z == std::numeric_limits<float>::infinity())
                    return;
                
                adorn->rect2d(Rect::fromCenterSize(screenPos.xy(), 6.0f).toRect2D(),
                              useAxisColor ? axisColors[xyz] : color);
            }
        }
    }
}

void DrawAdorn::partInfoText2D(
	const G3D::Vector3&			size,
	const G3D::CoordinateFrame&	position,
	const RBX::Camera&			camera,		
	Adorn*						adorn,
    const std::string&          text,
	const G3D::Color4&			color,
    float                       fontSize,
	int							normalIdMask)
{
    Vector3 halfSize = size / 2;
    RBX::Extents localExtents(-halfSize,halfSize); 
   
    const Vector3 max        = localExtents.max();
    const Vector3 min        = localExtents.min();

    Vector3 points[8];
    points[0] = Vector3(min.x, min.y, min.z);
    points[1] = Vector3(min.x, max.y, min.z);
    points[2] = Vector3(min.x, max.y, max.z);
    points[3] = Vector3(min.x, min.y, max.z);
    
    points[4] = Vector3(max.x, min.y, min.z);
    points[5] = Vector3(max.x, max.y, min.z);
    points[6] = Vector3(max.x, max.y, max.z);
    points[7] = Vector3(max.x, min.y, max.z);

    Vector2 lowestRightCornerMax = Vector2(0.0f, 0.0f);
    float minDistanceToOptimal = FLT_MAX;
    Vector2 lowestRightCorner;

    // In projection space, find the optimal point, the lowest rightest maximum of all the points.
    for (int i = 0; i < 8; i++)
    {
        const Vector3 worldPos = position.pointToWorldSpace(points[i]);
        const Vector3 projPos  = camera.project(worldPos);

        points[i] = projPos;

        if (projPos.z == std::numeric_limits<float>::infinity())
            continue;

        if (projPos.y > lowestRightCornerMax.y)
        {
            lowestRightCornerMax.y = projPos.y;
        }

        if (projPos.x > lowestRightCornerMax.x)
        {
            lowestRightCornerMax.x = projPos.x;
        }
    }

    // Find the point closest to lowest rightest maximum.
    for (int i = 0; i < 8; i++)
    {
        const Vector3 projPos = points[i];

        if (projPos.z == std::numeric_limits<float>::infinity())
            continue;

        float distanceToOptimal = (projPos.xy() - lowestRightCornerMax).length();
        
        if (minDistanceToOptimal > distanceToOptimal)
        {
            minDistanceToOptimal = distanceToOptimal;
            lowestRightCorner = projPos.xy();
        }
    }
    
    adorn->drawFont2D(text,
        lowestRightCorner,
        fontSize,
        false,
        color,
        RBX::Color4::clear(),
        RBX::Text::FONT_ARIAL);
}

void DrawAdorn::handles3d(	
	const G3D::Vector3&			size,
	const G3D::CoordinateFrame&	position,
	Adorn*						adorn,
	HandleType					handleType,
	const Vector3&				cameraPos,
	const G3D::Color4&			color,
    bool                        useAxisColor,
	int							normalIdMask,
	NormalId                    normalIdTohighlight,
	const G3D::Color4&			highlightColor)
{
    const Vector3 halfSize = size / 2;
    const Extents localExtents(-halfSize,halfSize);

    int usedaxes = 0; // for rotation handles, prevent use of NORM_X and NORM_X_NEG at the same time.
    for (int posNeg = 0; posNeg < 2; ++posNeg) 
    {
        for (int xyz = 0; xyz < 3; ++xyz) 
        {
            if(normalIdMask & (1<<(xyz + (posNeg*3))))
            {
                NormalId normalId = intToNormalId(posNeg * 3 + xyz);

                // determine where the handle should be located
                const Vector3 handlePos = handlePosInObject(
                    cameraPos,
                    localExtents,
                    handleType,
                    normalId );
                const Vector3 handlePosInWorld = position.pointToWorldSpace(handlePos);

                // compute relative size on screen based on distance
                const float handleRadius = scaleHandleRelativeToCamera(
                    cameraPos,
                    handleType,
                    handlePosInWorld );

                const CoordinateFrame frame(position.rotation,handlePosInWorld);
                adorn->setObjectToWorldMatrix(frame);

                if ( normalIdTohighlight == normalId )
                    adorn->setMaterial(Adorn::Material_SelfLitHighlight);
                else
                    adorn->setMaterial(Adorn::Material_SelfLit);

                const int axis_index = normalId % 3;
                Color4 handleColor = useAxisColor ? axisColors[axis_index] : color;
                
                if (normalIdTohighlight != normalId)
                    handleColor.a *= HandleTransparency;
                
                switch (handleType)
                {
                    case HANDLE_RESIZE:
                    case HANDLE_VELOCITY:
                    {
                        if (normalIdTohighlight == normalId)
                            adorn->sphere(Sphere(Vector3::zero(),handleRadius+0.08), handleColor);
                        else
                            adorn->sphere(Sphere(Vector3::zero(),handleRadius), handleColor);
                        break;
                    }
                    case HANDLE_MOVE:
                    {
                        Vector3 direction = normalIdToVector3(normalId) * handleRadius;
                        RbxRay rayInPart = RbxRay::fromOriginAndDirection(Vector3::zero(),direction);
                        adorn->ray(rayInPart, handleColor);
                        break;
                    }
                    case HANDLE_ROTATE:
                    {
                        const int axis_flag = normalIdToMask(normalId);

                        // only allow if positive orientation wasn't already specified.
                        if ( (usedaxes & axis_flag) == 0 )
                        {
                            const Vector3 delta = position.translation - handlePosInWorld;
                            const float torus_radius = delta.length();

                            // compute relative size thickness based on distance
                            const float thickness = scaleRelativeToCamera(
                                cameraPos,
                                position.translation,
                                TorusThicknessMinAngle,
                                TorusThickness );

                            Color4 axisColor(handleColor);
                            
                            if (normalIdTohighlight != normalId)
                                axisColor.a = AxisTransparency;
                            
                            torus(
                                adorn,
                                position,
                                normalId,
                                torus_radius,
                                thickness,
                                axisColor);

                            usedaxes |= axis_flag;
                        }

                        // need to rest the matrix because torus changed it
                        adorn->setObjectToWorldMatrix(handlePosInWorld);
                        if (normalIdTohighlight == normalId)
                            adorn->sphere( Sphere(Vector3::zero(),handleRadius+0.08), 
                                           handleColor);
                        else
                            adorn->sphere(Sphere(Vector3::zero(),handleRadius), Color3(handleColor.r - 0.02, handleColor.g - 0.02, handleColor.b - 0.02));
                        break;
                    }
                    default:
                        RBXASSERT(false);
                        break;
                }
            }
        }
    }

    adorn->setMaterial(Adorn::Material_Default);
}

// circle, with normal of linespace parallel to radius.
class CircleRadialNormal : public I3DLinearFunc
{
	Vector3 x;
	Vector3 y;
	Vector3 z;
	float r;
	NormalId axis;
public:
	CircleRadialNormal(float radius, NormalId axis) : r(radius), axis(axis)
	{	
		x = uvwToObject(Vector3::unitX(), axis);
		y = uvwToObject(Vector3::unitY(), axis);
		z = uvwToObject(Vector3::unitZ(), axis);
	}
	Vector3 eval(float t)
	{
		double theta = t *2* G3D::pi();
		return x*(float)(cos(theta)*r) + y*(float)(sin(theta)*r);
	}
	// first derivative.
	Vector3 evalTangent(float t)
	{
		double theta = t *2* G3D::pi();
		return y*(float)cos(theta) - x*(float)sin(theta);
	}
	Vector3 evalNormal(float t)
	{
		double theta = t *2* G3D::pi();
		return x*(float)cos(theta) + y*(float)sin(theta);
	}
	Vector3 evalBinormal(float t)
	{
		return -z;
	}

	//string that encodes this function in a unique way.
	std::string hashString()
	{
		std::ostringstream hash;
		hash << "CircleRN(" << r << "," << axis << ")";
		return hash.str();
	}
};

void DrawAdorn::chatBubble2d(
	Adorn*						adorn,
	const Rect2D&				rect,
	const G3D::Vector2			pointer,
	float						cornerradius,
	const float					linewidth,
	const int					quarterdivs,
	const Color4&				color)
{
	cornerradius = std::min(cornerradius, std::min(rect.width(), rect.height()));
	const float pointerwidth = std::min(cornerradius * 1.5f, (rect.width() - cornerradius * 2.0f));
	
	std::vector<G3D::Vector2> verts;
	std::vector<G3D::Vector2> vertsBorder;

	// start at base, right side of pointer.
	// go counterclockwise.
	// ui space:					math space:
	// (0,0)                  
	//   /--------------\               n
	//   |              |                \
	//   |              |             n-1\ \ 0     1
	//   \-----/  /-----/          /------\  \-----\
	//     n-1/ / 0     1          |               |
	//        /                    |               |
	//       n                     \---------------/
	//                           (0,0) 

	// y1 should be bottom.
	Vector2 p0(rect.center().x + pointerwidth/2, rect.y1());
	Vector2 p1(rect.center().x - pointerwidth/2, rect.y1());
	verts.push_back(p0);
	vertsBorder.push_back(p0 + Vector2(linewidth, linewidth));


	RBXASSERT(rect.wh().x >= cornerradius *2);
	RBXASSERT(rect.wh().y >= cornerradius * 2);

	Rect2D innerrect = rect.border(cornerradius);

	double angleinc = (quarterdivs > 0) ? (-(Math::pi() * 0.5) / quarterdivs) : 0;
	for(int i = 0; i <= quarterdivs; ++i)
	{
		double angle = (quarterdivs > 0) ? (Math::pi() * 0.5 + angleinc * i) : (Math::pi() * 0.25);
		Vector2 r = Vector2((float)cos(angle), (quarterdivs > 0) ? (float)sin(angle) : 1.0f);
		verts.push_back(innerrect.x1y1() + r * cornerradius);
		vertsBorder.push_back(innerrect.x1y1() + r * (cornerradius + linewidth));
	}
	for(int i = 0; i <= quarterdivs; ++i)
	{
		double angle = (quarterdivs > 0) ? (Math::pi() * 0.0 + angleinc * i) : (Math::pi() * 1.75);
		Vector2 r = Vector2((float)cos(angle), (float)sin(angle));
		verts.push_back(innerrect.x1y0() + r * cornerradius);
		vertsBorder.push_back(innerrect.x1y0() + r * (cornerradius + linewidth));
	}
	for(int i = 0; i <= quarterdivs; ++i)
	{
		double angle = (quarterdivs > 0) ? (Math::pi() * 1.5 + angleinc * i) : (Math::pi() * 1.25);
		Vector2 r = Vector2((float)cos(angle), (float)sin(angle));
		verts.push_back(innerrect.x0y0() + r * cornerradius);
		vertsBorder.push_back(innerrect.x0y0() + r * (cornerradius + linewidth));
	}
	for(int i = 0; i <= quarterdivs; ++i)
	{
		double angle = (quarterdivs > 0) ? (Math::pi() * 1.0 + angleinc * i) : (Math::pi() * .75);
		Vector2 r = Vector2((float)cos(angle), (quarterdivs > 0) ? (float)sin(angle) : 1.0f);
		verts.push_back(innerrect.x0y1() + r * cornerradius);
		vertsBorder.push_back(innerrect.x0y1() + r * (cornerradius + linewidth));
	}
	verts.push_back(p1);
	vertsBorder.push_back(p1 + Vector2(-linewidth, linewidth));
	verts.push_back(pointer);
	Vector2 pointerdir = pointer - (p0+p1)*0.5f;
	pointerdir.unitize();

	vertsBorder.push_back(pointer + pointerdir * linewidth*3); // todo: not correct.

	adorn->convexPolygon2d(&vertsBorder[0], vertsBorder.size(), G3D::Color4(Color3::black()));
	adorn->convexPolygon2d(&verts[0], verts.size(), G3D::Color4(color));
}

void DrawAdorn::torus(
	Adorn*						adorn,
	const						CoordinateFrame& position,		
	NormalId					axis,
	float						radius,
	float						thicknessradius,
	const Color4&				color)
{
	CircleRadialNormal trajectory(radius, axis);
	CircleRadialNormal profile(thicknessradius, NORM_Z);

	adorn->setObjectToWorldMatrix(position);
	adorn->extrusion(&trajectory, 32, &profile, 8, color);
}

/**
 * Draws a wire star made up of 3 line segments.
 *  
 * @param adorn         object used to generate the geometry
 * @param center        world space center coordinates
 * @param size          lines are -size to +size on the axis
 * @param colorX        color of x axis line
 * @param colorY        color of y axis line
 * @param colorZ        color of z axis line
 */
void DrawAdorn::star(
    Adorn*                  adorn,
    const Vector3&          center,
    float                   size,
    const Color4&           colorX,
    const Color4&           colorY,
    const Color4&           colorZ )
{
    size /= 2.0f;

    adorn->line3d(
        center - Vector3::unitX() * size,
        center + Vector3::unitX() * size,
        colorX );
    adorn->line3d(
        center - Vector3::unitY() * size,
        center + Vector3::unitY() * size,
        colorY );
    adorn->line3d(
        center - Vector3::unitZ() * size,
        center + Vector3::unitZ() * size,
        colorZ );
}

/**
 * Draw an outline (wireframe) box in 3D.
 * 
 * @param adorn     adornment to use for drawing
 * @param box       box to draw
 * @param color     color of lines
 */
void DrawAdorn::outlineBox(
    Adorn*          adorn,
    const AABox&    box,
    const Color4&   color )
{
    const Vector3& p0 = box.low();
    const Vector3& p1 = box.high();

    const float X0 = p0.x;
    const float Y0 = p0.y;
    const float Z0 = p0.z;
    const float X1 = p1.x;
    const float Y1 = p1.y;
    const float Z1 = p1.z;

    adorn->line3d(
        Vector3(X0,Y0,Z0),
        Vector3(X1,Y0,Z0),
        color );
    adorn->line3d(
        Vector3(X0,Y1,Z0),
        Vector3(X1,Y1,Z0),
        color );
    adorn->line3d(
        Vector3(X0,Y0,Z1),
        Vector3(X1,Y0,Z1),
        color );
    adorn->line3d(
        Vector3(X0,Y1,Z1),
        Vector3(X1,Y1,Z1),
        color );

    adorn->line3d(
        Vector3(X0,Y0,Z0),
        Vector3(X0,Y1,Z0),
        color);
    adorn->line3d(
        Vector3(X1,Y0,Z0),
        Vector3(X1,Y1,Z0),
        color );
    adorn->line3d(
        Vector3(X0,Y0,Z1),
        Vector3(X0,Y1,Z1),
        color);
    adorn->line3d(
        Vector3(X1,Y0,Z1),
        Vector3(X1,Y1,Z1),
        color );

    adorn->line3d(
        Vector3(X0,Y0,Z0),
        Vector3(X0,Y0,Z1),
        color );
    adorn->line3d(
        Vector3(X1,Y0,Z0),
        Vector3(X1,Y0,Z1),
        color );
    adorn->line3d(
        Vector3(X0,Y1,Z0),
        Vector3(X0,Y1,Z1),
        color );
    adorn->line3d(
        Vector3(X1,Y1,Z0),
        Vector3(X1,Y1,Z1),
        color );
}

/**
 * Draw select indicator around a box similar to 3DS.
 * 
 * @param adorn     adornment to use for drawing
 * @param box       box to draw
 * @param color     color of lines
 */
void DrawAdorn::selectionBox(
    Adorn*          adorn,
    const AABox&    box,
    const Color4&   color )
{
    const Vector3& P0 = box.low();
    const Vector3& P1 = box.high();

    float X0 = P0.x;
    float Y0 = P0.y;
    float Z0 = P0.z;
    float X1 = P1.x;
    float Y1 = P1.y;
    float Z1 = P1.z;

    float s0 = X0 + (X1 - X0) / 4;
    float s1 = X1 + (X0 - X1) / 4;
    float s2 = Y0 + (Y1 - Y0) / 4;
    float s3 = Y1 + (Y0 - Y1) / 4;
    float s4 = Z0 + (Z1 - Z0) / 4;
    float s5 = Z1 + (Z0 - Z1) / 4;

    // side0
    adorn->line3d(
        Vector3(X0,Y0,Z0),
        Vector3(s0,Y0,Z0),
        color );
    adorn->line3d(
        Vector3(X0,Y1,Z0),
        Vector3(s0,Y1,Z0),
        color );
    adorn->line3d(
        Vector3(X0,Y0,Z1),
        Vector3(s0,Y0,Z1),
        color );
    adorn->line3d(
        Vector3(X0,Y1,Z1),
        Vector3(s0,Y1,Z1),
        color );

    // side1
    adorn->line3d(
        Vector3(X1,Y0,Z0),
        Vector3(s1,Y0,Z0),
        color );
    adorn->line3d(
        Vector3(X1,Y1,Z0),
        Vector3(s1,Y1,Z0),
        color );
    adorn->line3d(
        Vector3(X1,Y0,Z1),
        Vector3(s1,Y0,Z1),
        color );
    adorn->line3d(
        Vector3(X1,Y1,Z1),
        Vector3(s1,Y1,Z1),
        color );

    // side2
    adorn->line3d(
        Vector3(X0,Y0,Z0),
        Vector3(X0,s2,Z0),
        color );
    adorn->line3d(
        Vector3(X1,Y0,Z0),
        Vector3(X1,s2,Z0),
        color );
    adorn->line3d(
        Vector3(X0,Y0,Z1),
        Vector3(X0,s2,Z1),
        color );
    adorn->line3d(
        Vector3(X1,Y0,Z1),
        Vector3(X1,s2,Z1),
        color );

    // side3
    adorn->line3d(
        Vector3(X0,Y1,Z0),
        Vector3(X0,s3,Z0),
        color );
    adorn->line3d(
        Vector3(X1,Y1,Z0),
        Vector3(X1,s3,Z0),
        color );
    adorn->line3d(
        Vector3(X0,Y1,Z1),
        Vector3(X0,s3,Z1),
        color );
    adorn->line3d(
        Vector3(X1,Y1,Z1),
        Vector3(X1,s3,Z1),
        color );

    // side4
    adorn->line3d(
        Vector3(X0,Y0,Z0),
        Vector3(X0,Y0,s4),
        color );
    adorn->line3d(
        Vector3(X1,Y0,Z0),
        Vector3(X1,Y0,s4),
        color );
    adorn->line3d(
        Vector3(X0,Y1,Z0),
        Vector3(X0,Y1,s4),
        color );
    adorn->line3d(
        Vector3(X1,Y1,Z0),
        Vector3(X1,Y1,s4),
        color );

    // side5
    adorn->line3d(
        Vector3(X0,Y0,Z1),
        Vector3(X0,Y0,s5),
        color );
    adorn->line3d(
        Vector3(X1,Y0,Z1),
        Vector3(X1,Y0,s5),
        color );
    adorn->line3d(
        Vector3(X0,Y1,Z1),
        Vector3(X0,Y1,s5),
        color );
    adorn->line3d(
        Vector3(X1,Y1,Z1),
        Vector3(X1,Y1,s5),
        color );
}

} // namespace
