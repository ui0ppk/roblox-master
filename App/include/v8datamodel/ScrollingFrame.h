/* Copyright 2003-2013 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/GuiObject.h"
#include "Util/SteppedInstance.h"
#include <boost/circular_buffer.hpp>

namespace RBX
{
	extern const char* const sScrollingFrame;
    
	class ScrollingFrame
		: public DescribedCreatable<ScrollingFrame, GuiObject, sScrollingFrame>
		, public IStepped
	{
	public:
		ScrollingFrame();
        
        enum ScrollingDirection
		{
			X = (1 << 0),
			Y = (1 << 1),
            XY = (1 << 2)
		};

		Rect2D getVerticalBarRect() const { return verticalBarSize; }
		Rect2D getHorizontalBarRect() const { return horizontalBarSize; }
        
        void setScrollingEnabled(bool value);
        bool getScrollingEnabled() const { return scrollingEnabled; }
        
		UDim2 getCanvasSize() const { return canvasSize; }
		void setCanvasSize(UDim2 value);

		Vector2 getCanvasPosition() const { return canvasPosition; }
		void luaSetCanvasPosition(Vector2 value);
		void setCanvasPosition(Vector2 value, bool printWarnings = true);

		int getScrollBarThickness() const { return scrollBarThickness; }
		void setScrollBarThickness(int value);

		TextureId getTopImage() const { return topImage; }
		void setTopImage(TextureId value);

		TextureId getMidImage() const { return midImage; }
		void setMidImage(TextureId value);

		TextureId getBottomImage() const { return bottomImage; }
		void setBottomImage(TextureId value);

		Vector2 getAbsoluteWindowSize() const;

		bool processInputFromDescendant(const shared_ptr<InputObject>& event);

		bool isTouchScrolling() { return touchScrolling; }

		bool hasInteractableDescendants();

		/////////////////////////////////////////////////////////////
		// GuiObject
		//
		/*override*/ Rect2D getClippedRect();
		/*override*/ GuiResponse process(const shared_ptr<InputObject>& event);
		/*override*/ Rect2D getCanvasRect() const;

		/////////////////////////////////////////////////////////////
		// Instance
		//
		/*override*/ void onPropertyChanged(const Reflection::PropertyDescriptor& descriptor);
    private:
		typedef DescribedCreatable<ScrollingFrame, GuiObject, sScrollingFrame> Super;
		

		RBX::Timer<RBX::Time::Fast> selectionTimer;

        // need to keep a reference to all images to stop flickering
        GuiDrawImage vTopImage;
        GuiDrawImage vMidImage;
        GuiDrawImage vBottomImage;
        
        GuiDrawImage hTopImage;
        GuiDrawImage hMidImage;
        GuiDrawImage hBottomImage;
        
		TextureId topImage;
		TextureId midImage;
		TextureId bottomImage;

		bool scrollingEnabled;

		int scrollBarThickness;

		UDim2 canvasSize;
		Vector2 canvasPosition;

        Vector2 lastInputPosition;
        bool scrollDragging;
        bool touchScrolling;
        ScrollingDirection scrollingDirection;
        
        Rect2D verticalBarSize;
        Rect2D horizontalBarSize;
        
        Vector2 sampledVelocity;
        boost::circular_buffer<std::pair<Vector2, float> > touchInputPositions;
        shared_ptr<InputObject> touchInput;
		bool isLeftMouseDown;

		bool needsRecalculate;

		int scrollDeltaMultiplier;

		Vector2 lastGamepadScrollDelta;

		RBX::Timer<RBX::Time::Fast> wheelUpTimer;
		RBX::Timer<RBX::Time::Fast> wheelDownTimer;
        RBX::Timer<RBX::Time::Fast> touchDeltaTimer;
        
        rbx::signals::scoped_connection inputEndedConnection;
		rbx::signals::scoped_connection guiServicePropertyChangedConnection;
		rbx::signals::scoped_connection selectionGainedConnection;
		rbx::signals::scoped_connection selectionLostConnection;

		Vector2 getScrollWheelDelta(bool scrollingDownValue);
        
		bool scrollByPosition(const Vector2& inputDelta, bool printWarnings = false);
		void resizeChildren();

        bool hasMouse() const;
        bool isTouchDevice() const;
        
        void handleInputDrag(const Vector2& inputPosition, const shared_ptr<InputObject>& event = shared_ptr<InputObject>());
        void handleInputBegin(const Vector2& inputPosition, const ScrollingDirection& dragDirection);

		shared_ptr<InputObject> leftMouseDownInput;

        void scrollDownWheel();
        void scrollUpWheel();

		void recalculateScroll();

		void stepGamepadScroll(const Stepped& event);
        
        void stepScrollInertia(const Stepped& event);
        void setScrollingInertia();
        
        bool isInsideDraggingRect(const Vector2& mousePosition);

		bool canScrollVertical() const;
		bool canScrollHorizontal() const;

		void renderScrollbarSection(Adorn* adorn, GuiDrawImage& drawImage, const TextureId& image, const char* context, const Rect2D& imageRect, const Rotation2D& rotation = Rotation2D());

		void renderVerticalScrollBar(Adorn* adorn);
		void renderHorizontalScrollBar(Adorn* adorn);

		Vector2 getFrameScrollingSize() const;
        Vector2 getCanvasScrollingSize() const;
        Vector2 calculateCanvasPositionClamped(Vector2 desiredValue) const;

		Vector2 getHorizontalScrollBarSize() const;
		Vector2 getVerticalScrollBarSize() const;

		Rect2D getCanvasRectLocal() const;

		bool canScrollInDirection(Vector2 direction);
		
		float getMinScrollBarSize() const;

		Vector2 getMaxSize() const;

		bool doProcessGamepadEvent(const shared_ptr<InputObject>& event, const bool inputOver);
		bool doProcessTouchEvent(const shared_ptr<InputObject>& event, const bool inputOver);
		bool doProcessMouseEvent(const shared_ptr<InputObject>& event, const bool inputOver);

		void globalInputEnded(const shared_ptr<Instance>& event);

		void selectionLost();
		void selectionGained();
		void selectedGuiObjectChanged(const RBX::Reflection::PropertyDescriptor* desc);

		/////////////////////////////////////////////////////////////
		// GuiBase2d
		//
		/*override*/ void handleResize(const Rect2D& viewport, bool force);
		/*override*/ void render2d(Adorn* adorn);

		void renderScrollbarLines(Adorn* adorn, const bool scrollVertical, const bool scrollHorizontal);
	
		Rect2D getVisibleRect2D() const;
        
        // IStepped
		/*override*/ void onStepped(const Stepped& event);
        
		//////////////////////////////////////////////////////////////
		//
		// Instance
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

	};
    
}