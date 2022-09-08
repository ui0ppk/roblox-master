#include "GfxBase/AdornBillboarder.h"
#include "GfxBase/ViewportBillboarder.h"
#include "V8DataModel/Camera.h"

namespace RBX {

AdornBillboarder::AdornBillboarder(Adorn* parent, const ViewportBillboarder& viewportBillboarder)
	: parent(parent)
    , viewport(viewportBillboarder.getViewport())
    , alwaysOnTop(viewportBillboarder.alwaysOnTop)
{
    parent->setObjectToWorldMatrix(viewportBillboarder.getCoordinateFrame());
}

AdornBillboarder::AdornBillboarder(Adorn* parent, const Rect2D& viewport, const CoordinateFrame& transform, bool alwaysOnTop)
    : parent(parent)
    , viewport(viewport)
    , alwaysOnTop(alwaysOnTop)
{
    parent->setObjectToWorldMatrix(transform);
}

Rect2D AdornBillboarder::getViewport() const 
{ 
	return viewport;
}

void AdornBillboarder::line2d(const Vector2& p0, const Vector2& p1, const Color4& color)
{
    Vector3 p03D = Vector3(p0, 0);
    Vector3 p13D = Vector3(p1, 0);

    p03D.y *= -1;
    p13D.y *= -1;

    parent->line3d(p03D, p13D, color);
}

void AdornBillboarder::rect2dImpl(
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

void AdornBillboarder::convexPolygon2d(
			const Vector2* v,
			int countv,
			const Color4& color)
{
#ifdef _WIN32
	Vector3* v3d = (Vector3*)_alloca(sizeof(Vector3) * countv);
#else
	Vector3* v3d = (Vector3*)alloca(sizeof(Vector3) * countv);
#endif

	for(int i = 0; i < countv; ++i)
	{
		v3d[i] = Vector3(v[i].x, -v[i].y, 0); /*neg y : convert from ui space (0,0 top left) to math space */
	}

	parent->convexPolygon(v3d, countv, color);
}

}
