/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#pragma once

#include "Util/G3DCore.h"

namespace G3D {
	class RenderDevice;
}

namespace RBX 
{
	class Rect;

	class DrawPrimitives {
	public:
		static void rawBox(
			const AABox& box, 
			G3D::RenderDevice* rd);

		static void rawSphere(
			float radius, 
			G3D::RenderDevice* rd);

		static void rawCylinderAlongX(
			float radius, 
			float axis, 
			G3D::RenderDevice* rd,
			bool cap);

		// generic 2d - must be in 2d mode
		static void rect2d(
			const RBX::Rect& rect,
			G3D::RenderDevice* rd,
			const G3D::Color4& color = G3D::Color3::white());

		static void line2d(
			const G3D::Vector2& p0,
			const G3D::Vector2& p1,
			G3D::RenderDevice* rd,
			const G3D::Color4& color = G3D::Color3::white());

		static void outlineRect2d(
			const RBX::Rect& rect, 
			float thick, 
			G3D::RenderDevice* rd,
			const G3D::Color4& color = G3D::Color3::blue());

	};
}	// namespace