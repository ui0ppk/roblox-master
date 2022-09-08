#include "GfxBase/AdornBillboarder2D.h"

namespace RBX {

AdornBillboarder2D::AdornBillboarder2D(Adorn* parent, const Rect2D& viewport, const Vector2& screenOffset)
    : parent(parent)
	, viewport(viewport)
    , screenOffset(screenOffset)
{
}

Rect2D AdornBillboarder2D::getViewport() const 
{ 
	return viewport;
}

void AdornBillboarder2D::line2d(const Vector2& p0, const Vector2& p1, const Color4& color)
{
    parent->line2d(p0 + screenOffset, p1 + screenOffset, color);
}

void AdornBillboarder2D::rect2dImpl(
    const Vector2& x0y0, const Vector2& x1y0, const Vector2& x0y1, const Vector2& x1y1,
    const Vector2& tex0, const Vector2& tex1, const Color4 & color)
{
    parent->rect2dImpl(x0y0 + screenOffset, x1y0 + screenOffset, x0y1 + screenOffset, x1y1 + screenOffset, tex0, tex1, color);
}

}
