#include "GfxBase/Adorn.h"

namespace RBX {

template <typename Modifier>
static void outlineRect2dImpl(Adorn* adorn, const Rect2D& rect, float thick, const Color4& color, const Modifier& modifier)
{
    adorn->rect2d(Rect2D::xyxy(rect.x0() - thick, rect.y0() - thick, rect.x0(), rect.y1()), color, modifier);
    adorn->rect2d(Rect2D::xyxy(rect.x1(), rect.y0(), rect.x1() + thick, rect.y1() + thick), color, modifier);

    adorn->rect2d(Rect2D::xyxy(rect.x0(), rect.y0() - thick, rect.x1() + thick, rect.y0()), color, modifier);
    adorn->rect2d(Rect2D::xyxy(rect.x0() - thick, rect.y1(), rect.x1(), rect.y1() + thick), color, modifier);
}

void Adorn::outlineRect2d(const Rect2D& rect, float thick, const Color4& color)
{
	outlineRect2dImpl(this, rect, thick, color, Rotation2D());
}

void Adorn::outlineRect2d(const Rect2D& rect, float thick, const Color4& color, const Rotation2D& rotation)
{
	outlineRect2dImpl(this, rect, thick, color, rotation);
}

void Adorn::outlineRect2d(const Rect2D& rect, float thick, const Color4& color, const Rect2D& clipRect)
{
	outlineRect2dImpl(this, rect, thick, color, clipRect);
}

void Adorn::rect2d(const Rect2D& rect, const Color4& color)
{
	rect2d(rect, Vector2(0, 0), Vector2(1, 1), color, Rotation2D());
}

void Adorn::rect2d(const Rect2D& rect, const Color4& color, const Rotation2D& rotation)
{
    rect2d(rect, Vector2(0, 0), Vector2(1, 1), color, rotation);
}

void Adorn::rect2d(const Rect2D& rect, const Color4& color, const Rect2D& clipRect)
{
    rect2d(rect, Vector2(0, 0), Vector2(1, 1), color, clipRect);
}

void Adorn::rect2d(const Rect2D& rect, const Vector2& texul, const Vector2& texbr, const Color4& color)
{
	rect2d(rect, texul, texbr, color, Rotation2D());
}

void Adorn::rect2d(const Rect2D& rect, const Vector2& texul, const Vector2& texbr, const Color4& color, const Rotation2D& rotation)
{
	if (!rotation.empty())
	{
        Vector2 x0y0 = rotation.rotate(rect.x0y0());
        Vector2 x1y0 = rotation.rotate(rect.x1y0());
        Vector2 x0y1 = rotation.rotate(rect.x0y1());
        Vector2 x1y1 = rotation.rotate(rect.x1y1());

        rect2dImpl(x0y0, x1y0, x0y1, x1y1, texul, texbr, color);
	}
	else
	{
		rect2dImpl(rect.x0y0(), rect.x1y0(), rect.x0y1(), rect.x1y1(), texul, texbr, color);
	}
}

void Adorn::rect2d(const Rect2D& rect, const Vector2& texul, const Vector2& texbr, const Color4& color, const Rect2D& clipRect)
{
    RBX::Rect2D intersectRect = rect;

    Vector2 lowerUV(texul.x,texul.y);
    Vector2 upperUV(texbr.x,texbr.y);

    if(clipRect != rect)
    {
        intersectRect = clipRect.intersect(rect);

        if(rect.width() != 0)
        {
            float uvwidth = upperUV.x - lowerUV.x;

            lowerUV.x += ( uvwidth * ( intersectRect.x0() - rect.x0() ) / rect.width() );
            upperUV.x += ( uvwidth * ( intersectRect.x1() - rect.x1() ) / rect.width() );
        }

        if(rect.height() != 0)
        {
            float uvheight = upperUV.y - lowerUV.y;

            lowerUV.y += ( uvheight * ( intersectRect.y0() - rect.y0() ) / rect.height() );
            upperUV.y += ( uvheight * ( intersectRect.y1() - rect.y1() ) / rect.height() );
        }
    }

	rect2dImpl(intersectRect.x0y0(), intersectRect.x1y0(), intersectRect.x0y1(), intersectRect.x1y1(), lowerUV, upperUV, color);
}

Vector2 Adorn::drawFont2D(const std::string& s,
    const Vector2& position, float size, bool autoScale, const Color4& color, const Color4& outline,
    Text::Font font, Text::XAlign xalign, Text::YAlign yalign,
	const Vector2& availableSpace, const Rect2D& clippingRect, const Rotation2D& rotation)
{
	return drawFont2DImpl(this, s, position, size, autoScale, color, outline, font, xalign, yalign, availableSpace, clippingRect, rotation);
}

}