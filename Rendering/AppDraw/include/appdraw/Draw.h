 /* Copyright 2003-2013 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "SelectState.h"
#include "Util/G3DCore.h"
#include "V8DataModel/ModelInstance.h"

#include <vector>

namespace G3D {
	class Rect2D;
	class CoordinateFrame;
	class Ray;
	class RenderDevice;
	class Color4;
}

namespace RBX 
{
		
	class Part;
	class Details;
	class Adorn;

	class Draw {
	private:

		// generic
		static void adornSurfaces(const Part& part, Adorn* adorn, const G3D::Color3& controllerColor);
		static void frameBox(const Part& part, Adorn* adorn, const Color4& color);
		static void constraint(const Part& part, Adorn* adorn, int face, const G3D::Color3& controllerColor);

        static bool 	   m_showHoverOver;
        static G3D::Color4 m_hoverOverColor;
        static G3D::Color4 m_selectColor;

	public:
        static const G3D::Color4& selectColor() { return m_selectColor; }
        static void setSelectColor(const G3D::Color4& Color);

        static const G3D::Color4& hoverOverColor() { return m_hoverOverColor; }
        static void setHoverOverColor(const G3D::Color4& color);

		// DRAWING - assumes 2D mode
		static void selectionSquare(const G3D::Rect2D& rect, float thick);

		static void partAdorn(	
			const Part&					part, 
			Adorn*						adorn,
			const G3D::Color3&			controllerColor);

		static void selectionBox(	
			const Part&					part, 
			Adorn*						adorn,
			const G3D::Color4&			selectColor,
			float						lineThickness = 0.15f);

		static void selectionBox(	
			ModelInstance&				model, 
			Adorn*						adorn,
			const G3D::Color4&			selectColor,
			float						lineThickness = 0.15f);

		static void selectionBox(	
			const Part&					part, 
			Adorn*						adorn,
			SelectState					selectState,
			float						lineThickness = 0.15f);

        static void showHoverOver(bool Show) { m_showHoverOver = Show; }
        static bool isHoverOver()            { return m_showHoverOver; }
	};

} // namespace

