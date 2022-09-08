#include "GfxBase/AdornSurface.h"

namespace RBX {

	AdornSurface::AdornSurface(Adorn* parent, const Rect2D& viewport, const CoordinateFrame& transform, bool alwaysOnTop)
        : parent(parent)
        , viewport(viewport)
        , alwaysOnTop(alwaysOnTop)
	{
        parent->setObjectToWorldMatrix(transform);
	}

	Rect2D AdornSurface::getViewport() const
	{
        return viewport;
	}

	//////////////////////////////////////////////////////////////////////////

	void AdornSurface::setTexture(int id, const RBX::TextureProxyBaseRef& t)
	{
		return parent->setTexture(id, t);
	}

	Rect2D AdornSurface::getTextureSize(const RBX::TextureProxyBaseRef& texture) const
	{
		return parent->getTextureSize(texture);
	}

	void AdornSurface::line2d(const Vector2& p0, const Vector2& p1, const Color4& color)
	{
        Vector3 p03D = Vector3(p0, 0);
        Vector3 p13D = Vector3(p1, 0);

        p03D.y *= -1;
        p13D.y *= -1;

        parent->line3d(p03D, p13D, color);
	}

	void AdornSurface::rect2dImpl(
		const Vector2& x0y0, const Vector2& x1y0, const Vector2& x0y1, const Vector2& x1y1,
		const Vector2& tex0, const Vector2& tex1, const Color4 & color)
	{
		Vector3 px0y0(x0y0, 0);
		Vector3 px1y0(x1y0, 0);
		Vector3 px0y1(x0y1, 0);
		Vector3 px1y1(x1y1, 0);

		px0y1.y *= -1;
		px1y1.y *= -1;
		px0y0.y *= -1;
		px1y0.y *= -1;

		parent->quad(px0y0, px1y0, px0y1, px1y1, color, tex0, tex1, 0, alwaysOnTop);
	}

	Vector2 AdornSurface::drawFont2DImpl(Adorn* target, const std::string& s, const Vector2& pos2D, float size, bool autoScale, const Color4& color, const Color4& outline, Text::Font font, Text::XAlign xalign, Text::YAlign yalign, const Vector2& availableSpace, const Rect2D& clippingRect, const Rotation2D& rotation )
	{
		return parent->drawFont2DImpl(target, s, pos2D, size, autoScale, color, outline, font, xalign, yalign, availableSpace, clippingRect, rotation);
	}

	Vector2 AdornSurface::get2DStringBounds(const std::string& s, float size, Text::Font font, const Vector2& availableSpace ) const
	{
		return parent->get2DStringBounds(s, size, font, availableSpace);
	}
}
