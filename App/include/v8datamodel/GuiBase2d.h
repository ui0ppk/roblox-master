#pragma once

#include "GuiBase.h"

namespace RBX {

	extern const char* const sGuiBase2d;

	//A set of base functionality used by all Lua bound Gui objects
	class GuiBase2d	: public DescribedNonCreatable<GuiBase2d, GuiBase, sGuiBase2d>
	{
	public:
		GuiBase2d(const char* name);

		virtual bool isGuiLeaf() const { return false; }

		virtual Vector2 getAbsolutePosition() const { return absolutePosition; }
		bool setAbsolutePosition(const Vector2& value, bool fireChangedEvent = true);

		Vector2 getAbsoluteSize() const { return absoluteSize; }
		bool setAbsoluteSize(const Vector2& value, bool fireChangedEvent = true);

		Rect2D getRect2D() const;		// four integers - maybe we should store size, position as a Rect2d (g3d version) or Rect (our version)?
		Rect2D getRect2DFloat() const;
		virtual Rect2D getChildRect2D() const { return getRect2DFloat(); }
		virtual Rect2D getCanvasRect() const { return getChildRect2D(); }


		virtual void handleResize(const Rect2D& viewport, bool force);

		virtual bool recalculateAbsolutePlacement(const Rect2D& viewport);

		/////////////////////////////////////////////////////////////
		// Instance
		//
		/*override*/ bool askAddChild(const Instance* instance) const;

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// GuiBase
		/*implement*/ virtual bool canProcessMeAndDescendants() const { return true; }
		/*implement*/ virtual int getZIndex() const { return zIndex; }
		/*implement*/ virtual GuiQueue getGuiQueue() const { return guiQueue; }

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// IAdornable
		/*override*/ bool shouldRender2d() const { return false; } // explicit render traversal by ScreenGui or other.

		/*override*/ bool isVisible(const Rect2D& rect) const { return rect.intersects(getRect2D()); }

		static Reflection::PropDescriptor<GuiBase2d, Vector2> prop_AbsoluteSize;
		static Reflection::PropDescriptor<GuiBase2d, Vector2> prop_AbsolutePosition;


	protected:
		void recursiveRender2d(Adorn* adorn);
		void setGuiQueue(GuiQueue queue) { guiQueue = queue; }

		Vector2 absolutePosition;
		Vector2 absolutePositionFloat;
		Vector2 absoluteSize;
		Vector2 absoluteSizeFloat;
		int zIndex;
		GuiQueue guiQueue;

	private:
		typedef DescribedNonCreatable<GuiBase2d, GuiBase, sGuiBase2d> Super;

		static void RecursiveRenderChildren(shared_ptr<RBX::Instance> instance, Adorn* adorn);
	};
}
