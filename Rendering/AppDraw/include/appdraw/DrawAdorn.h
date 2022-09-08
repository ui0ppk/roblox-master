 /* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#pragma once

#include "SelectState.h"
#include "appDraw/HandleType.h"
#include "Util/G3DCore.h"
#include "Util/NormalId.h"
#include <vector>
#include "V8World/Primitive.h"
#include "V8DataModel/PartInstance.h"
#include "Tool/DragTypes.h"

namespace RBX {
	
	class Part;
	class Adorn;
	class Face;
	class Camera;

	class DrawAdorn {
	private:
		static void surfaceBorder(
			Adorn*			adorn,
			const Vector3&	halfRealSize,
			float			highlight,
			int				surfaceId,
			const Color4&	color);

	public:
		static const Color3& resizeColor() {
			static Color3 c(0.1f, 0.6f, 1.0f);
			return c;
		}

        static float scaleHandleRelativeToCamera(
		    const Vector3&  cameraPos,
            HandleType      handleType,
            const Vector3&  handlePos );

        static Vector3 handlePosInObject(
            const Vector3&  cameraPos,
            const Extents&  localExtents, 
            HandleType      handleType,
            NormalId        normalId );

		static void cylinder(
			Adorn*						adorn,
			const CoordinateFrame&      worldC,		
			int							axis,
			float						length,
			float						radius,
			const Color4&				color,
			bool						cap = true);

		static void faceInWorld(
			Adorn*			adorn,
			const Face&		face,
			float			thickness,
			const Color4&	color);

		static void partSurface(
			const Part&					part, 
			int							surfaceId,
			Adorn*						adorn,
			const G3D::Color4&			color = G3D::Color4(G3D::Color3::orange()),
			float						thickness = 0.2f);

		static void surfaceGridOnFace(
			const Primitive&			prim,
			Adorn*						adorn,
			int							surfaceId,
			const G3D::Color4&			color,
			int							boxesPerStud );

		static void zeroPlaneGrid(
			Adorn*						adorn,
			const RBX::Camera&			camera,
			const int					studsPerBox,
			const int					yLevel,
			const G3D::Color4&			smallGridColor,
			const G3D::Color4&			largeGridColor);

		static void surfaceGridAtCoord(
			Adorn*						adorn,
			CoordinateFrame&			cF,
			const Vector4&				bounds,
			const Vector3&				gridXDir,
			const Vector3&				gridYDir,
			const G3D::Color4&			color,
			int							boxesPerStud );

		static void axisWidget(
			Adorn* adorn, 
			const RBX::Camera& camera);

		static void circularGridAtCoord(
			Adorn*                      adorn,
            const CoordinateFrame&      coordFrame,
            const G3D::Vector3&		    size,
			const G3D::Vector3&	        cameraPos,
            NormalId                    normalId,
            const Color4&               color,
            int                         boxesPerStud );

		static void surfacePolygon(
			Adorn*						adorn,
			PartInstance&				partInst,
			int							surfaceId,
			const G3D::Color3&			color,
			float						lineThickness );

		static void polygonRelativeToCoord(
			Adorn*						adorn,
			const CoordinateFrame&		cF,
			std::vector<Vector3>&		vertices,
			const G3D::Color4&			color,
			float						lineThickness );

		static void lineSegmentRelativeToCoord(
			Adorn*						adorn,
			const CoordinateFrame&		cF,
			const Vector3&      		pt0,
			const Vector3&      		pt1,
			const G3D::Color3&			color,
			float						lineThickness );

		static void verticalLineSegmentSplit(
			Adorn*						adorn,
			const CoordinateFrame&		cF,
			const Vector3&      		pt0,
			const Vector3&      		pt1,
			const float&      		    delta,
			const float&      		    magicParam,
			const short&      		    level,
			const G3D::Color3&			color,
			float						lineThickness );

		static void handles3d(	
			const G3D::Vector3&			size,
			const G3D::CoordinateFrame&	position,
			Adorn*						adorn,
			HandleType					handleType,
			const G3D::Vector3&			cameraPos,
			const G3D::Color4&			color,
            bool                        useAxisColor = true,
			int							normalIdMask = NORM_ALL_MASK,
			NormalId                    normalIdTohighlight = NORM_UNDEFINED,
			const G3D::Color4&			highlightColor = cornflowerblue
			);

		// Must be in 2d Mode
		static void handles2d(	
			const G3D::Vector3&			size,
			const G3D::CoordinateFrame&	position,
			const RBX::Camera&			camera,		
			Adorn*						adorn,
			HandleType					handleType,
			const G3D::Color4&			color,
            bool                        useAxisColor = true,
			int							normalIdMask = NORM_ALL_MASK
			);
        
        static void partInfoText2D(
            const G3D::Vector3&			size,
            const G3D::CoordinateFrame&	position,
            const RBX::Camera&			camera,		
            Adorn*						adorn,
            const std::string&          text,
            const G3D::Color4&			color,
            float                       fontSize = 18.0f,
            int							normalIdMask = NORM_ALL_MASK);

		static void chatBubble2d(
			Adorn*						adorn,
			const Rect2D&				rect,
			const G3D::Vector2			pointer,
			float						cornerradius,
			const float					linewidth,
			const int					quarterdivs,
			const Color4&				color);

		static void torus(
			Adorn*						adorn,
			const						CoordinateFrame& worldC,		
			NormalId					axis,
			float						radius,
			float						thicknessradius,
			const Color4&				color);

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
        static void star(
            Adorn*                  adorn,
            const Vector3&          center,
            float                   size   = 1.0f,
            const Color4&           colorX = Color3::white(),
            const Color4&           colorY = Color3::white(),
            const Color4&           colorZ = Color3::white() );

        /**
         * Draw an outline (wireframe) box in 3D.
         * 
         * @param adorn     adornment to use for drawing
         * @param box       box to draw
         * @param color     color of lines
         */
        static void outlineBox(
            Adorn*          adorn,
            const AABox&    box,
            const Color4&   color );

        /**
         * Draw an outline (wireframe) box in 3D.
         * 
         * @param adorn     adornment to use for drawing
         * @param extents   box dimensions
         * @param color     color of lines
         */
        static void outlineBox(
            Adorn*          adorn,
            const Extents&  extents,
            const Color4&   color ) 
        {
	        AABox aa_box(extents.min(),extents.max());
	        outlineBox(adorn,aa_box,color);
        }

        /**
         * Draw select indicator around a box similar to 3DS.
         * 
         * @param adorn     adornment to use for drawing
         * @param box       box to draw
         * @param color     color of lines
         */
        static void selectionBox(
            Adorn*          adorn,
            const AABox&    box,
            const Color4&   color );

        /**
         * Draw select indicator around a box similar to 3DS.
         * 
         * @param adorn     adornment to use for drawing
         * @param extents   box dimensions
         * @param color     color of lines
         */
        static void selectionBox(
            Adorn*          adorn,
            const Extents&  extents,
            const Color4&   color ) 
        {
	        AABox aa_box(extents.min(),extents.max());
	        selectionBox(adorn,aa_box,color);
        }

        // common colors
        static const Color3 beige;
        static const Color3 darkblue;
        static const Color3 powderblue;
        static const Color3 skyblue;
        static const Color3 violet;
        static const Color3 slategray;
        static const Color3 aqua;
        static const Color3 tan;
        static const Color3 wheat;
        static const Color3 cornflowerblue;
        static const Color3 limegreen;
        static const Color3 magenta;
        static const Color3 pink;
        static const Color3 silver;
        
        // Color values for each axis (x, y, z).
        static const Color3 axisColors[3];

    private:

        static float scaleRelativeToCamera(
		    const Vector3&  cameraPos,
            const Vector3&  handlePos,
            float           minAngle,
            float           expectedRadius );
	};

} // namespace

