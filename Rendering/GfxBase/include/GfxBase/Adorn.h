#pragma once

#include <string>

#include "GfxBase/Type.h"
#include "GfxBase/TextureProxyBase.h"
#include "Util/Extents.h"
#include "Util/G3DCore.h"
#include "Util/ContentId.h"
#include "Util/Rotation2D.h"
#include "rbx/signal.h"
#include "rbx/Declarations.h"

namespace RBX {

class I3DLinearFunc;
class RenderCaps;
class Camera;

struct Canvas
{
	Canvas(Vector2 viewPort)
		:size(viewPort)
	{}
	Vector2 size;
	Vector2 toPixelSize(const Vector2& percent) const;		// std screen is 100% wide and 75% tall
	int normalizedFontSize(int fontSize) const;
};

// RBX::Adorn is a base class used to decorate other objects using 2D or 3D
// basic shapes.
class RBXBaseClass Adorn
{
public:
    enum Material
	{
        Material_Default,
        Material_NoLighting,
        Material_SelfLit,
        Material_SelfLitHighlight,
        Material_AALine,
        Material_Outline,

        Material_Count
	};

	Adorn(): ignoreTexture(false), vr(false), currentMaterial(Material_Default) {}

	Canvas getCanvas() const { return getViewport().wh(); }

	bool isVR() const { return vr; }

    virtual const Camera* getCamera() const = 0;

	virtual ~Adorn() {}

	virtual TextureProxyBaseRef createTextureProxy(const ContentId& id,
		bool& waiting, bool bBlocking = false, const std::string& context = "") = 0;

	// Listen to this signal if you need a hint about when to release your
	// TextureProxys.
    virtual rbx::signal<void()>& getUnbindResourcesSignal() = 0;

	// Called to perform any preparations before the render pass begins.
	virtual void prepareRenderPass() {}

	// Called to perform any cleanup after every 2D/3D render pass.
	virtual void finishRenderPass() {}

	virtual void preSubmitPass() {}
	virtual void postSubmitPass() {}

    virtual bool useFontSmoothScalling() { return false; }

    void setMaterial(Material material_)
    {
        currentMaterial = material_;
    }
    
    Material getMaterial() const { return currentMaterial; }

	/////////////////////////////////////////////////
	//
	// Viewport
	// Returns the Adorn's viewport area.
	//
	// Note that this viewport doesn't always represent the area where the
	// game is being displayed.
	virtual Rect2D getViewport() const = 0;

	// Hack - buffering the GuiRect here - ultimately need to clip the
	// "User Gui Space" with the ROBLOX Gui stuff.
	void setUserGuiInset(const Vector4& value) { userGuiInset = value; }

	Rect2D getUserGuiRect() const
	{
		Rect2D vp = getViewport();
		return Rect2D::xyxy(vp.x0() + userGuiInset.x, vp.y0() + userGuiInset.y, vp.x1() - userGuiInset.z,
			vp.y1() - userGuiInset.w);
		
	}

	/////////////////////////////////////////////////
	//
	// Textures, Draw Rectangles, Lines

	void setIgnoreTexture(bool ignore) {ignoreTexture = ignore;}
	bool getIgnoreTexture() const {return ignoreTexture;}

	// Sets the texture used by the Adorn.
	virtual void setTexture(
		int id,
		const RBX::TextureProxyBaseRef& texture) = 0;

	// Gets the size of the texture being used by the Adorn.
	virtual Rect2D getTextureSize(
		const RBX::TextureProxyBaseRef& texture) const = 0;

	// Draws a line on the screen.
	virtual void line2d(
		const Vector2& p0,
		const Vector2& p1,
		const Color4& color) = 0;

	// Draws a hollow rectangle on the screen.
	void outlineRect2d(const Rect2D& rect, float thick, const Color4& color);
	void outlineRect2d(const Rect2D& rect, float thick, const Color4& color, const Rotation2D& rotation);
	void outlineRect2d(const Rect2D& rect, float thick, const Color4& color, const Rect2D& clipRect);

	// Draws a solid rectangle on the screen.
	void rect2d(const Rect2D& rect, const Color4& color);
	void rect2d(const Rect2D& rect, const Color4& color, const Rotation2D& rotation);
	void rect2d(const Rect2D& rect, const Color4& color, const Rect2D& clipRect);

	void rect2d(const Rect2D& rect, const Vector2& texul, const Vector2& texbr, const Color4& color);
	void rect2d(const Rect2D& rect, const Vector2& texul, const Vector2& texbr, const Color4& color, const Rotation2D& rotation);
	void rect2d(const Rect2D& rect, const Vector2& texul, const Vector2& texbr, const Color4& color, const Rect2D& clipRect);

    // Rectangle drawing implementation
	virtual void rect2dImpl(const Vector2& x0y0, const Vector2& x1y0, const Vector2& x0y1, const Vector2& x1y1, const Vector2& tex0, const Vector2& tex1, const Color4 & color) = 0;

	/////////////////////////////////////////////////
	//
	// Draw Fonts

	// Retrieves the boundaries (in pixels) of a string, if it were to be
	// drawn on screen.
	virtual Vector2 get2DStringBounds(
		const std::string&	s,
        float				size,
		Text::Font			font = Text::FONT_LEGACY,
		const Vector2&		availableSpace = Vector2::zero()) const = 0;

	// Draws a string using this Adorn.
	Vector2 drawFont2D(
        const std::string&  s,
        const Vector2&      position,
        float               size,
        bool                autoScale,
        const Color4&       color   = Color3::black(),
        const Color4&       outline = Color4::clear(),
		Text::Font			font = Text::FONT_LEGACY,
		Text::XAlign        xalign  = Text::XALIGN_LEFT,
		Text::YAlign        yalign  = Text::YALIGN_TOP,
		const Vector2&		availableSpace = Vector2::zero(),
		const Rect2D&		clippingRect = RBX::Rect2D::xyxy(-1,-1,-1,-1),
		const Rotation2D&   rotation = Rotation2D());

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
		const Rotation2D&   rotation) = 0;

	/////////////////////////////////////////////////
	//
	// 3D stuff - procedural

	virtual void line3d(
		const Vector3&	startPoint,
		const Vector3&	endPoint,
		const RBX::Color4& color) = 0;

    virtual void line3dAA(
        const Vector3& startPoint, 
        const Vector3& endPoint, 
        const RBX::Color4& color, 
        float thickness,
		int zIndex,
        bool alwaysOnTop) = 0;

	// Sets the adorn's coordinate frame.
	virtual void setObjectToWorldMatrix(
		const CoordinateFrame&	c) = 0;

	// Draws an axis aligned bounding box on the adorn.
	virtual void box(
        const AABox&		box,
        const Color4&       solidColor = Color4(1,.2f,.2f,.5f))  = 0;

	// Draws a box on the adorn.
	void box(
        const Extents&		extents,
        const Color4&       solidColor = Color4(1,.2f,.2f,.5f))
	{
		AABox aaBox(extents.min(), extents.max());
		box(aaBox, solidColor);
	}

	virtual void box(
		const CoordinateFrame& cFrame,
		const Vector3& size,
		const Color4& color,
		int zIndex,
		bool alwaysOnTop) = 0;

	// Draws a sphere on the adorn.
	virtual void sphere(
        const Sphere&       sphere,
        const Color4&       solidColor = Color4(1, 1, 0, .5f))  = 0;

	virtual void sphere(
		const CoordinateFrame& cFrame, 
		float radius, 
		const Color4& color, 
		int zIndex, 
		bool alwaysOnTop) = 0;

	// Draws an explosion on the adorn.
	virtual void explosion(const Sphere& sphere) = 0;

	virtual void cylinder(
		const CoordinateFrame& cFrame,
		float radius,
		float height,
		const Color4& color,
		int zIndex,
		bool alwaysOnTop) = 0;

	// Draws a cylinder along the adorn's x axis.
	virtual void cylinderAlongX(
		float				radius,
		float				length,
		const Color4&		solidColor,
		bool				cap = true)  = 0;

	virtual void cone(
		const CoordinateFrame& cFrame, 
		float radius, 
		float height, 
		const Color4& color, 
		int zIndex, 
		bool alwaysOnTop) = 0;

	// Draws a ray from the adorn.
	virtual void ray(
        const RbxRay&			ray,
        const Color4&       color = Color3::orange())  = 0;

	// Draws the x, y, z axis of the adorn.
	virtual void axes(
        const Color4&       xColor = Color3::red(),
        const Color4&       yColor = Color3::green(),
        const Color4&       zColor = Color3::blue(),
        float               scale = 1.0f)  = 0;


	// Draws a quad on the adorn.
	//
	// v0 The first point to form the quad.
	// v1 The second point to form the quad.
	// v2 The third point to form the quad.
	// v3 The fourth point to form the quad.
	// color The color used to draw the quad.
	// v0tex UV coordinates used on the first polygon.
	// v2tex UV coordinates used on the second polygon.
	// opt The material options to use when drawing the quad.
	virtual void quad(
		const Vector3&		v0,
		const Vector3&		v1,
		const Vector3&		v2,
		const Vector3&		v3,
		const Color4&		color = Color3::blue(),
		const Vector2&		v0tex = Vector2::zero(),
		const Vector2&      v2tex = Vector2::zero(),
		int					zIndex = -1,
		bool				alwaysOnTop = false) = 0;

	// Draws a convex 3D polygon on the adorn.
	//
	// v The set of points that compose the polygon.
	// countv The number of points that compose the polygon.
	// color The color used when drawing the polygon.
	// opt The material options to use when drawing the polygon.
	virtual void convexPolygon(
		const Vector3* v,
		int countv,
		const Color4&		color) = 0;

	// Draws a convex 2D polygon on the adorn.
	//
	// v The set of points that compose the polygon.
	// countv The number of points that compose the polygon.
	// color The color used when drawing the polygon.
	// opt The material options to use when drawing the polygon.
	virtual void convexPolygon2d(
		const Vector2* v,
		int countv,
		const Color4&		color) = 0;

	// Evaluates extrusion, calling trajectory and profile func with domain [0..1].
	// if closeTrajectory or closeProfile is true, func is only evaluated to [0..1[,
	// and evaluation for f(0) is used again for f(1).
	// Future: if closeTrajectory != closeProfile, that could indicate we need
	// caps at the end.
	virtual void extrusion(RBX::I3DLinearFunc* trajectory, int trajectorysegments,
						   RBX::I3DLinearFunc* profile, int profilesegments,
						   const Color4& color, bool closeTrajectory = true,
						   bool closeProfile = true) = 0;
    
    virtual bool isVisible(const Extents& extents, const CoordinateFrame& cframe) { return true; }

	static const int maximumZIndex = 10;

protected:	
	Vector4 userGuiInset;
	bool ignoreTexture;
	bool vr;
    Material currentMaterial;
};

}  // namespace RBX
