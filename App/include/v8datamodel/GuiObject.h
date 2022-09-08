#pragma once

#include "V8DataModel/GuiCore.h"
#include "V8DataModel/GuiBase2d.h"
#include "V8DataModel/EventReplicator.h"
#include "V8DataModel/TextService.h"
#include "gui/GuiEvent.h"
#include "Util/BrickColor.h"
#include "Util/UDim.h"
#include "Util/TextureId.h"
#include "Util/Rotation2D.h"
#include "Gui/GuiDraw.h"
#include "Script/ThreadRef.h"

namespace RBX
{
	class GuiDrawImage;
	class TweenService;
	class UserInputService;

	extern const char* const sGuiObject;

	class GuiObject 
		: public DescribedNonCreatable<GuiObject, GuiBase2d, sGuiObject>
	{
	private:
		typedef DescribedNonCreatable<GuiObject, GuiBase2d, sGuiObject> Super;
	public:
		enum ImageScale
		{
			SCALE_STRETCH = 0,
			SCALE_SLICED = 1
		};

		enum SizeConstraint
		{
			RELATIVE_XY = 0,
			RELATIVE_XX = 1,
			RELATIVE_YY = 2
		};

		enum TweenEasingDirection
		{
			EASING_DIRECTION_IN,
			EASING_DIRECTION_OUT,
			EASING_DIRECTION_IN_OUT
		};

		enum TweenEasingStyle
		{	
			EASING_STYLE_LINEAR,
			EASING_STYLE_SINE,
			EASING_STYLE_BACK,
			EASING_STYLE_QUAD,
			EASING_STYLE_QUART,
			EASING_STYLE_QUINT,
			EASING_STYLE_BOUNCE,
			EASING_STYLE_ELASTIC,
		};
		enum TweenStatus
		{
			TWEEN_CANCELED,
			TWEEN_COMPLETED,
		};

		struct Tween
		{
			UDim2 start;
			UDim2 end;
			float elapsedTime;
			float totalTime;
			TweenEasingDirection style;
			TweenEasingStyle variance;

			float delayTime;
			boost::function<void(TweenStatus)> callback;
			Tween(const UDim2& start, const UDim2& end, float time, TweenEasingDirection style, TweenEasingStyle variance, float delayTime)
				:start(start)
				,end(end)
				,elapsedTime(0)
				,delayTime(delayTime)
				,totalTime(time)
				,style(style)
				,variance(variance)
			{}
			bool isDone() const { return elapsedTime >= totalTime; } 
		};

		struct Tweens
		{
			scoped_ptr<Tween> sizeTween;
			scoped_ptr<Tween> positionTween;

			bool empty() const { return !sizeTween && !positionTween; }
		};

	private:
		static UDim2 TweenInterpolate(TweenEasingDirection style, TweenEasingStyle variance, 
			float elapsedTime, float totalTime, const UDim2& startValue, const UDim2& endValue);

		static void UpdateTween(Tween& tween, GuiObject* obj, boost::function<void(GuiObject*, UDim2)> updateFunc, float timeStep);

		bool descendantOfBillboardGui();

		void fireGenericInputEvent(const shared_ptr<InputObject>& event);
        void fireGestureEvent(const UserInputService::Gesture& gesture, const shared_ptr<const RBX::Reflection::ValueArray>& touchPositions, const shared_ptr<const Reflection::Tuple>& args);

		scoped_ptr<Tweens> tweens;

		bool selectionBox;
		
		boost::unordered_map<const shared_ptr<InputObject>,bool> interactedInputObjects;

		weak_ptr<GuiObject> nextSelectionUp;
		weak_ptr<GuiObject> nextSelectionDown;
		weak_ptr<GuiObject> nextSelectionLeft;
		weak_ptr<GuiObject> nextSelectionRight;

		shared_ptr<GuiObject> selectionImageObject;

		InputObject::UserInputType lastMouseDownType;

		// use for dragging gui objects
		bool draggable;
		bool dragging;
		Vector2 lastMousePosition;
		rbx::signals::scoped_connection draggingEndedConnection;
		shared_ptr<InputObject> draggingBeganInputObject;
		void draggingEnded(const shared_ptr<Instance>& event);

	public:
		bool tweenStep(const double& timeStep);

		bool tweenSizeAndPosition(UDim2 endSize, UDim2 endPosition, TweenEasingDirection style, TweenEasingStyle variance, float time, bool overwrite, Lua::WeakFunctionRef callback);
		bool tweenPosition(UDim2 endPosition, TweenEasingDirection style, TweenEasingStyle variance, float time, bool overwrite, Lua::WeakFunctionRef callback);
		bool tweenPosition(UDim2 endPosition, TweenEasingDirection style, TweenEasingStyle variance,float time,  bool overwrite, bool removeOnCallback);
		bool tweenPosition(UDim2 endPosition, TweenEasingDirection style, TweenEasingStyle variance,float time,  bool overwrite, bool removeOnCallback, TweenService* tweenService);
		bool tweenSize(UDim2 endSize, TweenEasingDirection style, TweenEasingStyle variance, float time, bool overwrite, Lua::WeakFunctionRef callback);
		
		bool tweenPositionDelay(UDim2 startValue, UDim2 endValue, float time, TweenEasingDirection style, TweenEasingStyle variance, float delay, bool overwrite, boost::function<void(TweenStatus)> callback);
		bool tweenPositionDelay(UDim2 startValue, UDim2 endValue, float time, TweenEasingDirection style, TweenEasingStyle variance, float delay, bool overwrite, boost::function<void(TweenStatus)> callback, TweenService* tweenService);
		bool tweenSizeDelay(UDim2 startValue, UDim2 endValue, float time, TweenEasingDirection style, TweenEasingStyle variance, float delay, bool overwrite, boost::function<void(TweenStatus)> callback);

		virtual Rect2D getClippedRect();

		static float convertFontSize(TextService::FontSize size)
		{
			switch(size)
			{
			case TextService::SIZE_8 : return 8;
			case TextService::SIZE_9 : return 9;
			case TextService::SIZE_10: return 10;
			case TextService::SIZE_11: return 11;
			case TextService::SIZE_12: return 12;
			case TextService::SIZE_14: return 14;
			case TextService::SIZE_18: return 18;
			case TextService::SIZE_24: return 24;
			case TextService::SIZE_36: return 36;
			case TextService::SIZE_48: return 48;
			case TextService::SIZE_28: return 28;
			case TextService::SIZE_32: return 32;
			case TextService::SIZE_42: return 42;
			case TextService::SIZE_60: return 60;
			case TextService::SIZE_96: return 96;
			default:
				RBXASSERT(0);
				return 0;
			}
		}

		GuiObject(const char* name, bool active);
        

        static const Reflection::PropDescriptor<GuiObject, bool> prop_Visible;
        static const Reflection::PropDescriptor<GuiObject, int>	 prop_ZIndex;

		void setSize(UDim2 value);
		UDim2 getSize() const { return size; }

		void setSizeConstraint(SizeConstraint value);
		SizeConstraint getSizeConstraint() const { return sizeConstraint; }

		void setPosition(UDim2 value);
		UDim2 getPosition() const { return position; }

        void setRotation(float value);
		float getRotation() const { return rotation.getValue(); }

        bool setAbsoluteRotation(const Rotation2D& value);
		const Rotation2D& getAbsoluteRotation() const { return absoluteRotation; }

		void setBorderSizePixel(int value);
		int getBorderSizePixel() const { return borderSizePixel; }

		void setDraggable(bool value);
		bool getDraggable() const { return draggable; }

		bool getClipping() const { return clipping; }
		void setClipping(bool value);

		// used to show a selection box around the gui (used when selected in tree)
		void setSelectionBox(bool value) { selectionBox = value; }
		bool getSelectionBox() const {return selectionBox; }

		GuiObject* getNextSelectionUp() const;
		void setNextSelectionUp(GuiObject* value);

		GuiObject* getNextSelectionDown() const;
		void setNextSelectionDown(GuiObject* value);

		GuiObject* getNextSelectionLeft() const;
		void setNextSelectionLeft(GuiObject* value);

		GuiObject* getNextSelectionRight() const;
		void setNextSelectionRight(GuiObject* value);

		GuiObject* firstAncestorClipping();

		void handleDrag(RBX::Vector2 mousePosition);
		void handleDragging(const shared_ptr<InputObject>& event);
		void handleDragBegin(RBX::Vector2 mousePosition);

		Gui::WidgetState getGuiState() const { return guiState; }
		void setGuiState(Gui::WidgetState state) { guiState = state; }

		void setZIndex(int value);

		void setBorderColor(BrickColor value);
		BrickColor getBorderColor() const { return BrickColor::closest(borderColor); }

		void setBorderColor3(Color3 value);
		Color3 getBorderColor3() const { return borderColor; }

		void setBackgroundColor(BrickColor value);
		BrickColor getBackgroundColor() const { return BrickColor::closest(backgroundColor); }

		void setBackgroundColor3(Color3 value);
		Color3 getBackgroundColor3() const { return backgroundColor; }

		void setVisible(bool value);
		bool getVisible() const { return visible; }
		void setActive(bool value);
		bool getActive() const { return active; }
		/*override*/ bool canProcessMeAndDescendants() const {return getVisible();}

		// determines whether a gamepad/keyboard can move the selection to this object
		void setSelectable(bool value);
		bool getSelectable() const { return selectable; }

		void setBackgroundTransparency(float value);
		float getBackgroundTransparency() const { return backgroundTransparency; }


		virtual void setTransparencyLegacy(float value) { setBackgroundTransparency(value); }
		float getTransparencyLegacy() const { return getBackgroundTransparency(); }

		bool isCurrentlyVisible();

		// Selection Events
		rbx::signal<void()> selectionGainedEvent;
		rbx::signal<void()> selectionLostEvent;

		// low level generic event signals
		rbx::signal<void(shared_ptr<Instance>)> inputBeganEvent;
		rbx::signal<void(shared_ptr<Instance>)> inputChangedEvent;
		rbx::signal<void(shared_ptr<Instance>)> inputEndedEvent;
        
        // touch gesture event signals
        rbx::signal<void(shared_ptr<const RBX::Reflection::ValueArray>)> tapGestureEvent;
        rbx::signal<void(shared_ptr<const RBX::Reflection::ValueArray>,float, float, InputObject::UserInputState)> pinchGestureEvent;
        rbx::signal<void(UserInputService::SwipeDirection, int)> swipeGestureEvent;
        rbx::signal<void(shared_ptr<const RBX::Reflection::ValueArray>, InputObject::UserInputState)> longPressGestureEvent;
        rbx::signal<void(shared_ptr<const RBX::Reflection::ValueArray>, float, float, InputObject::UserInputState)> rotateGestureEvent;
        rbx::signal<void(shared_ptr<const RBX::Reflection::ValueArray>, Vector2, Vector2, InputObject::UserInputState)> panGestureEvent;

		rbx::remote_signal<void(int, int)>  mouseEnterSignal;
		rbx::remote_signal<void(int, int)>  mouseLeaveSignal;
		rbx::remote_signal<void(int, int)>	mouseMovedSignal;
		rbx::remote_signal<void(int, int)>	mouseWheelForwardSignal;
		rbx::remote_signal<void(int, int)>	mouseWheelBackwardSignal;

		DECLARE_EVENT_REPLICATOR_SIG(GuiObject,MouseEnter,		 void(int,int));
		DECLARE_EVENT_REPLICATOR_SIG(GuiObject,MouseLeave,		 void(int,int));
		DECLARE_EVENT_REPLICATOR_SIG(GuiObject,MouseMoved,		 void(int,int));
		DECLARE_EVENT_REPLICATOR_SIG(GuiObject,MouseWheelForward,	 void(int,int));
		DECLARE_EVENT_REPLICATOR_SIG(GuiObject,MouseWheelBackward,	 void(int,int));

		rbx::remote_signal<void(int, int)> dragStoppedSignal;
		rbx::remote_signal<void(UDim2)> dragBeginSignal;

		DECLARE_EVENT_REPLICATOR_SIG(GuiObject,DragStopped,		 void(int, int));
		DECLARE_EVENT_REPLICATOR_SIG(GuiObject,DragBegin,		 void(UDim2));

		/////////////////////////////////////////////////////////////
		// Instance
		//
		/*override*/ bool askSetParent(const Instance* instance) const;
		/*override*/ void onAncestorChanged(const AncestorChanged& event);
		/*override*/ void onPropertyChanged(const Reflection::PropertyDescriptor& descriptor);
		/*override*/ int getPersistentDataCost() const 
		{
			return Super::getPersistentDataCost() + 6;
		}
		/////////////////////////////////////////////////////////////
		// GuiBase2d
		//
		/*override*/ bool recalculateAbsolutePlacement(const Rect2D& parentViewport);
        /*override*/ Vector2 getAbsolutePosition() const;

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// IAdornable
		/*override*/ void render2d(Adorn* adorn);
		void legacyRender2d(Adorn* adorn, const Rect2D& parentRectangle);

		void renderSelectionFrame(Adorn* adorn, GuiObject* selectionImageObject);
		virtual void renderStudioSelectionBox(Adorn* adorn);

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// GuiBase
		/*override*/ GuiResponse process(const shared_ptr<InputObject>& event);
        /*override*/ GuiResponse processGesture(const UserInputService::Gesture& gesture, const shared_ptr<const RBX::Reflection::ValueArray>& touchPositions, const shared_ptr<const Reflection::Tuple>& args);

		virtual GuiResponse preProcess(const shared_ptr<InputObject>& event) { return GuiResponse::notSunk(); };

		virtual void checkForResize();


		GuiObject* getSelectionImageObject() const {return selectionImageObject.get();}
		void setSelectionImageObject(GuiObject* value);

	protected:
		virtual void setServerGuiObject();
		bool serverGuiObject;

		bool clipping;

		Gui::WidgetState guiState;

		UDim2 size;
		UDim2 position;

        RotationAngle rotation;
        Rotation2D absoluteRotation;

		SizeConstraint sizeConstraint;
		
		int borderSizePixel;

		Color3 backgroundColor;
		Color3 borderColor;
		bool visible;
		bool active;
		bool selectable;
		float backgroundTransparency;

		//float getAlpha() const;
		float getRenderBackgroundAlpha() const;
		Color4 getRenderBackgroundColor4() const;
		
        float getFontSizeScale(bool _textScale, bool _textWrap, TextService::FontSize _fontSizeEnum, const Rect2D& rect);
		float getScaledFontSize(const Rect2D& rect, const std::string& _textName, TextService::Font _font, bool _textWrap, float _fontSize);

		void render2dImpl(	Adorn* adorn, 
							const Color4& _backgroundColor);
		void render2dImpl(	Adorn* adorn, 
							const Color4& _backgroundColor,
							Rect2D& rect);
		Vector2 render2dTextImpl(	Adorn* adorn, 
								const Color4& _backgroundColor,
								const std::string& textName,
								TextService::Font		_font, 
								TextService::FontSize fontSize,
								const Color4& textColor,
								const Color4& textStrokeColor,
								bool textWrap,
								bool textScale,
								TextService::XAlignment _xalign,
								TextService::YAlignment _yalign);
		Vector2 render2dTextImpl(	Adorn* adorn, 
                                const Rect2D& rectIn,
								const std::string& textName,
								TextService::Font		_font, 
								TextService::FontSize fontSize,
								const Color4& textColor,
								const Color4& textStrokeColor,
								bool textWrap,
								bool textScale,
								TextService::XAlignment _xalign,
								TextService::YAlignment _yalign);

		void render2dScale9Impl( Adorn* adorn,
				  				 const TextureId& texId,
								 const Vector2int16& scaleEdgeSize, 
								 const Vector2& minSize,
								 GuiDrawImage& guiImageDraw,
                                 Rect2D& rect,
								 GuiObject* clippingObject);
        
        void render2dScale9Impl2(Adorn* adorn,
                                 const TextureId& texId,
                                 GuiDrawImage& guiImageDraw,
                                 Rect2D& scaleRect,
                                 GuiObject* clippingObject,
                                 Color4& color,
								 Rect2D* overrideRect = NULL,
								 Rect2D* imageOffsetRect = NULL);

		virtual GuiResponse processTouchEvent(const shared_ptr<InputObject>& event);
		virtual GuiResponse processMouseEvent(const shared_ptr<InputObject>& event);
		virtual GuiResponse processMouseEventInternal(const shared_ptr<InputObject>& event, bool fireEvents);
		virtual GuiResponse preProcessMouseEvent(const shared_ptr<InputObject>& event) { return GuiResponse::notSunk(); };
		virtual GuiResponse processKeyEvent(const shared_ptr<InputObject>& event);
		virtual GuiResponse processGamepadEvent(const shared_ptr<InputObject>& event);

		static Rect2D Scale9Rect2D(const Rect2D& rect, float border, float minSize);
		void forceResize();

		bool mouseIsOver(const Vector2& mousePosition);
		bool isSelectedObject();
	};



	extern const char* const sGuiButton;

	class GuiButton
		: public DescribedNonCreatable<GuiButton, GuiObject, sGuiButton>
	{
	private:
		typedef DescribedNonCreatable<GuiButton, GuiObject, sGuiButton> Super;
        bool clicked;
        bool shouldFireClickedEvent;
        
        weak_ptr<InputObject> lastSelectedObjectEvent;
        
		GuiResponse checkForSelectedObjectClick(const shared_ptr<InputObject>& event);
	public:
		GuiButton(const char* name);

		enum Style
		{
			CUSTOM_STYLE						= 0,
			ROBLOX_RED_STYLE					= 1,
			ROBLOX_GREY_STYLE					= 2,
			ROBLOX_BUTTON_ROUND_STYLE			= 3,
			ROBLOX_BUTTON_ROUND_DEFAULT_STYLE	= 4,
			ROBLOX_BUTTON_ROUND_DROPDOWN_STYLE	= 5,
		};

		static const Reflection::PropDescriptor<GuiButton, bool> prop_Modal;

		rbx::remote_signal<void()> mouseButton1ClickSignal;
        rbx::remote_signal<void()> mouseButton2ClickSignal;
		rbx::remote_signal<void(int, int)> mouseButton1DownSignal;
		rbx::remote_signal<void(int, int)>	mouseButton1UpSignal;
		rbx::remote_signal<void(int, int)>	mouseButton2DownSignal;
		rbx::remote_signal<void(int, int)>	mouseButton2UpSignal;

		DECLARE_EVENT_REPLICATOR_SIG(GuiButton,MouseButton1Click, void());
		DECLARE_EVENT_REPLICATOR_SIG(GuiButton,MouseButton2Click, void());
		DECLARE_EVENT_REPLICATOR_SIG(GuiButton,MouseButton1Down, void(int,int));
		DECLARE_EVENT_REPLICATOR_SIG(GuiButton,MouseButton1Up,	 void(int,int));
		DECLARE_EVENT_REPLICATOR_SIG(GuiButton,MouseButton2Down, void(int,int));
		DECLARE_EVENT_REPLICATOR_SIG(GuiButton,MouseButton2Up,	 void(int,int));

		bool getAutoButtonColor() const { return autoButtonColor; }
		void setAutoButtonColor(bool value);

		bool getSelected() const { return selected; }
		void setSelected(bool value);

		Style getStyle() const { return style; }
		void setStyle(Style value);

		/*override*/ bool isGuiLeaf() const { return true; }

		/////////////////////////////////////////////////////////////
		// Instance
		//
		/*override*/ void onPropertyChanged(const Reflection::PropertyDescriptor& descriptor);

		/////////////////////////////////////////////////////////////
		// GuiBase2d
		//
		/*override*/ GuiResponse processMouseEvent(const shared_ptr<InputObject>& event);
		/*override*/ GuiResponse processGamepadEvent(const shared_ptr<InputObject>& event);
		/*override*/ GuiResponse processKeyEvent(const shared_ptr<InputObject>& event);
		/*override*/ GuiResponse processTouchEvent(const shared_ptr<InputObject>& event);
		/*override*/ Rect2D getChildRect2D() const;

		void render2dButtonImpl(Adorn* adorn, Rect2D& rect);

		bool getClicked() {return clicked;}
		void setClicked(bool value) {clicked = value;}

		bool getModal() const { return modal; }
		void setModal(bool value);

		void setVerb(std::string verbString);
		Verb* getVerb() { return verb; }

	protected:
		/*override*/ void setServerGuiObject();

		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

		bool autoButtonColor;
		bool selected;
		bool modal;
		Style style;
		Verb* verb;
		std::string verbToSet;


		boost::scoped_array<GuiDrawImage> images;
	};


	extern const char* const sGuiLabel;

	class GuiLabel
		: public DescribedNonCreatable<GuiLabel, GuiObject, sGuiLabel>
	{
	public:
		GuiLabel(const char* name);

		/*override*/ bool isGuiLeaf() const { return true; }
	};
}
