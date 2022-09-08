#include "stdafx.h"

#include "V8DataModel/Folder.h"
#include "V8DataModel/GuiBase2d.h"

namespace RBX{

const char* const  sGuiBase2d = "GuiBase2d";

//Read only properties for Absolute position/Absolute size
Reflection::PropDescriptor<GuiBase2d, Vector2> GuiBase2d::prop_AbsoluteSize("AbsoluteSize", category_Data, &GuiBase2d::getAbsoluteSize, NULL, Reflection::PropertyDescriptor::UI);
Reflection::PropDescriptor<GuiBase2d, Vector2> GuiBase2d::prop_AbsolutePosition("AbsolutePosition", category_Data, &GuiBase2d::getAbsolutePosition, NULL, Reflection::PropertyDescriptor::UI);


GuiBase2d::GuiBase2d(const char* name)
: Super(name)
, zIndex(GuiBase::minZIndex2d())
, guiQueue(GUIQUEUE_GENERAL)
{}

bool GuiBase2d::recalculateAbsolutePlacement(const Rect2D& viewport)
{
	bool result = false;
	result = setAbsolutePosition(viewport.x0y0());
	result = setAbsoluteSize(viewport.wh()) || result;
	return result;
}

void GuiBase2d::RecursiveRenderChildren(shared_ptr<RBX::Instance> instance, Adorn* adorn)
{
	if(RBX::GuiBase2d* guiBase = Instance::fastDynamicCast<RBX::GuiBase2d>(instance.get())){
		guiBase->recursiveRender2d(adorn);
	}
}

void GuiBase2d::recursiveRender2d(Adorn* adorn)
{
	render2d(adorn);

	visitChildren(boost::bind(&GuiBase2d::RecursiveRenderChildren, _1, adorn));		
}

static void ResizeChildren(shared_ptr<RBX::Instance> instance, const Rect2D& viewport, bool force)
{
	if(RBX::GuiBase2d* guiBase = Instance::fastDynamicCast<RBX::GuiBase2d>(instance.get())){
		guiBase->handleResize(viewport, force);
	} else if (RBX::Folder* f = Instance::fastDynamicCast<RBX::Folder>(instance.get())) {
		f->visitChildren(boost::bind(&ResizeChildren, _1, viewport, force));
	}
}
void GuiBase2d::handleResize(const Rect2D& viewport, bool force)
{
	//First we handle our own resize
	if(recalculateAbsolutePlacement(viewport) || force){
		//If our absolute size/position changed in any way, we have to resize our children too
		visitChildren(boost::bind(&ResizeChildren, _1, getChildRect2D(), force));
	}
}

bool GuiBase2d::setAbsolutePosition(const Vector2& value, bool fireChangedEvent)
{
	if(absolutePositionFloat != value){
		absolutePositionFloat = value;
		absolutePosition = Math::roundVector2(value);
		if (fireChangedEvent)
		{
			raisePropertyChanged(prop_AbsolutePosition);
		}
		return true;
	}
	return false;
}
bool GuiBase2d::setAbsoluteSize(const Vector2& value, bool fireChangedEvent)
{
	if(absoluteSizeFloat != value){
		absoluteSizeFloat = value;
		absoluteSize = Math::roundVector2(value);
		if (fireChangedEvent)
		{
			raisePropertyChanged(prop_AbsoluteSize);
		}
		return true;
	}
	return false;
}

//Only Gui items can descend from Gui items... no funny business
bool GuiBase2d::askAddChild(const Instance* instance) const
{
	return (Instance::fastDynamicCast<GuiBase2d>(instance) != NULL);
}

Rect2D GuiBase2d::getRect2D() const		// four integers - maybe we should store size, position as a Rect2d (g3d version) or Rect (our version)?
{
	return Rect2D::xywh(absolutePosition, absoluteSize);
}

Rect2D GuiBase2d::getRect2DFloat() const
{
	return Rect2D::xywh(absolutePositionFloat, absoluteSizeFloat);
}

} // Namespace
