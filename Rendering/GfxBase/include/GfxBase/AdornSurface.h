#pragma once

#include "GfxBase/Adorn.h"
#include "V8DataModel/Workspace.h"
#include "util/UDim.h"

namespace RBX {


	class AdornSurface : public Adorn
	{
		Adorn* parent;
        Rect2D viewport;
        bool alwaysOnTop;

	public:
		AdornSurface(Adorn* parent, const Rect2D& viewport, const CoordinateFrame& transform, bool alwaysOnTop = false);

        virtual bool useFontSmoothScalling() { return false; }

		void setTexture(int id, const RBX::TextureProxyBaseRef& texture);
		Rect2D getTextureSize( const RBX::TextureProxyBaseRef& texture) const;

		void line2d(const Vector2& p0, const Vector2& p1, const Color4& color);

		virtual void rect2dImpl(const Vector2& x0y0, const Vector2& x1y0, const Vector2& x0y1, const Vector2& x1y1, const Vector2& tex0, const Vector2& tex1, const Color4 & color);

		Vector2 get2DStringBounds(const std::string& s, float size, Text::Font font, const Vector2& availableSpace ) const;
		Vector2 drawFont2DImpl(Adorn* target, const std::string& s, const Vector2& pos2D, float size, bool autoScale, const Color4& color, const Color4& outline, Text::Font font, Text::XAlign xalign, Text::YAlign yalign, const Vector2& availableSpace, const Rect2D& clippingRect, const Rotation2D& rotation );

		const Camera* getCamera() const																		{ return 0; }
		TextureProxyBaseRef createTextureProxy(const ContentId& id, bool& waiting, bool bBlocking, const std::string& context = "")			{ return parent->createTextureProxy(id,waiting,bBlocking,context);	}
        rbx::signal<void()>& getUnbindResourcesSignal()                                                     { return parent->getUnbindResourcesSignal(); }
		Rect2D getViewport() const;

		void setObjectToWorldMatrix(const CoordinateFrame& c) { ; }
		void line3d(const Vector3& startPoint, const Vector3& endPoint, const RBX::Color4& color)			{ ; }
		void line3dAA(const Vector3& startPoint, const Vector3& endPoint, const RBX::Color4& color, float thickness, int zIndex, bool alwaysOnTop) { ; }
		void box(const AABox& b, const Color4& solidColor)							{ ; }
		void box(const CoordinateFrame& cFrame, const Vector3& size, const Color4& color, int zIndex, bool alwaysOnTop) { ; }
		void sphere(const Sphere& s, const Color4& solidColor)						{ ; }
		void sphere(const CoordinateFrame& cFrame, float radius, const Color4& color, int zIndex, bool alwaysOnTop) { ; }
		void explosion(const Sphere& sphere)																{ ; }
		void cylinder(const CoordinateFrame& cFrame, float radius, float height, const Color4& color, int zIndex, bool alwaysOnTop) { ; }
		void cylinderAlongX(float radius, float length, const Color4& solidColor, bool cap)				    { ; }
		void cone(const CoordinateFrame& cFrame, float radius, float height, const Color4& color, int zIndex, bool alwaysOnTop) { ; }
		void ray(const RbxRay& ray, const Color4& color)							{ ; }
		void axes(const Color4&, const Color4&, const Color4&, float)										{ ; }
		void quad(const Vector3&, const Vector3&, const Vector3&, const Vector3&, const Color4&, const Vector2&, const Vector2&, int zIndex, bool alwaysOnTop)    { ; }
		void convexPolygon(const Vector3*, int, const Color4&)										        { ; }
		void convexPolygon2d(const Vector2*, int, const Color4&)                                            { ; }
		void extrusion(RBX::I3DLinearFunc*, int, RBX::I3DLinearFunc*, int, const Color4&, bool,	bool)	    { ; }
	};

}

