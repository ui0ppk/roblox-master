#pragma once

#include "GfxBase/Adorn.h"

namespace RBX {

	class AdornBillboarder2D : public Adorn
	{
	protected:
		Adorn* parent;
        Rect2D viewport;
        Vector2 screenOffset;

	public:
        AdornBillboarder2D(Adorn* parent, const Rect2D& viewport, const Vector2& screenOffset);

		/*override*/ TextureProxyBaseRef createTextureProxy(const ContentId& id, bool& waiting, bool bBlocking = false, const std::string& context = "") { return parent->createTextureProxy(id, waiting, bBlocking, context); };
        /*override*/ rbx::signal<void()>& getUnbindResourcesSignal() { return parent->getUnbindResourcesSignal(); }

		/////////////////////////////////////////////////
		//
		// Viewport
		/*override*/ Rect2D getViewport() const;
        virtual const Camera* getCamera() const { return NULL; }

		/////////////////////////////////////////////////
		//
		// Textures, Draw Rectangles, Lines

		/*override*/ void setTexture(
			int id, 
			const RBX::TextureProxyBaseRef& texture) { parent->setTexture(id, texture); };

		/*override*/ Rect2D getTextureSize(
			const RBX::TextureProxyBaseRef& texture) const { return parent->getTextureSize(texture); };

        virtual bool useFontSmoothScalling() { return true; }

		virtual void line2d(
			const Vector2& p0,	
			const Vector2& p1, 
			const Color4& color);

        virtual void rect2dImpl(
			const Vector2& x0y0, const Vector2& x1y0, const Vector2& x0y1, const Vector2& x1y1,
			const Vector2& tex0, const Vector2& tex1, const Color4 & color);

        virtual Vector2 get2DStringBounds(
            const std::string&	s,
            float				size,
            Text::Font			font,
            const Vector2&		availableSpace) const
		{
			return parent->get2DStringBounds(s, size, font, availableSpace);
		}

        virtual Vector2 drawFont2DImpl(
            Adorn*              target,
            const std::string&  s,
            const Vector2&      position,
            float               size,
            bool                autoScale,
            const Color4&       color,
            const Color4&       outline,
            Text::Font			font,
            Text::XAlign        xalign,
            Text::YAlign        yalign,
            const Vector2&		availableSpace,
            const Rect2D&		clippingRect,
            const Rotation2D&   rotation)
		{
			return parent->drawFont2DImpl(target, s, position, size, autoScale, color, outline, font, xalign, yalign, availableSpace, clippingRect, rotation);
		}

		/////////////////////////////////////////////////
		//
		// 3D stuff - procedural

		/*override*/ void setObjectToWorldMatrix(
			const CoordinateFrame&	c) { throw std::runtime_error("Invalid operation"); };

		/*override*/ void box(
			const AABox&		box,
			const Color4&       solidColor = Color4(1,.2f,.2f,.5f))  { throw std::runtime_error("Invalid operation"); };

		/*override*/ void box(
			const CoordinateFrame& cFrame,
			const Vector3& size,
			const Color4& color,
			int zIndex,
			bool alwaysOnTop) { throw std::runtime_error("Invalid operation"); };

		/*override*/ void sphere(
			const Sphere&       sphere,
			const Color4&       solidColor = Color4(1, 1, 0, .5f))  { throw std::runtime_error("Invalid operation"); };

		/*override*/ void sphere(
			const CoordinateFrame& cFrame, 
			float radius, 
			const Color4& color, 
			int zIndex, 
			bool drawFront) { throw std::runtime_error("Invalid ooperation"); };

		/*override*/ void explosion(const Sphere& sphere)
		{
			throw std::runtime_error("Invalid operation");
		};

		/*override*/ void cylinder(
			const CoordinateFrame& cFrame, 
			float radius, 
			float height, 
			const Color4& color, 
			int zIndex, 
			bool alwaysOnTop) { throw std::runtime_error("Invalid operation"); };

		/*override*/ void cylinderAlongX(
			float				radius,
			float				length,
			const Color4&		solidColor,
			bool				cap = true)  { throw std::runtime_error("Invalid operation"); };

		/*override*/ void cone(
			const CoordinateFrame& cFrame,
			float radius,
			float height,
			const Color4& color,
			int zIndex,
			bool alwaysOnTop) { throw std::runtime_error("Invalid operation"); };

		/*override*/ void ray(
			const RbxRay&			ray,
			const Color4&       color = Color3::orange())  { throw std::runtime_error("Invalid operation"); };

		/*override*/ void line3d(
			const Vector3&	startPoint,
			const Vector3&	endPoint,
			const RBX::Color4& color) { throw std::runtime_error("Invalid operation"); };

        /*override*/ virtual void line3dAA(
            const Vector3& startPoint, 
            const Vector3& endPoint, 
            const RBX::Color4& color, 
            float thickness,
			int zIndex,
            bool alwaysOnTop)  { throw std::runtime_error("Invalid operation"); };

		/*override*/ void axes( 
			const Color4&       xColor = Color3::red(),
			const Color4&       yColor = Color3::green(),
			const Color4&       zColor = Color3::blue(),
			float               scale = 1.0f)  { throw std::runtime_error("Invalid operation"); };

		/*override*/ void quad(
			const Vector3&		v0,
			const Vector3&		v1,
			const Vector3&		v2,
			const Vector3&		v3,
			const Color4&		color = Color3::blue(),
			const Vector2&		v0tex = Vector2::zero(),
			const Vector2&      v2tex = Vector2::zero(),
			int					zIndex = -1,
			bool				alwaysOnTop = false)
			{ throw std::runtime_error("Invalid operation"); };

		/*override*/ void convexPolygon2d(
			const Vector2* v,
			int countv,
			const Color4&		color)  { throw std::runtime_error("Invalid operation"); }

		/*override*/ void convexPolygon(
			const Vector3* v,
			int countv,
			const Color4&		color) { throw std::runtime_error("Invalid operation"); };

		// evaluates extrusion, calling trajectory and profile func with domain [0..1].
		// if closeTrajectory or closeProfile is true, func is only evaluated to [0..1[, and evaluation for f(0) is used again for f(1).
		// future: if closeTrajectory != closeProfile, that could indicate we need caps at the end.
		/*override*/ void extrusion(I3DLinearFunc* trajectory, int trajectorysegments, 
							   I3DLinearFunc* profile, int profilesegments,
							   const Color4& color, bool closeTrajectory = true, bool closeProfile = true) { throw std::runtime_error("Invalid operation"); };

	};

};
