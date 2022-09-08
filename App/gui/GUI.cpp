#include "stdafx.h"
#include "Gui/GUI.h"

namespace RBX {

/////////////////////////////////////////////////////////////////////////////////////
//
// GuiItem
//

const char* const sGuiItem	= "GuiItem";

const Color4& GuiItem::disabledFill()		     {static Color4 c(.7f,.7f,.7f,.5f); return c;}
const Color4& GuiItem::translucentBackdrop()     {static Color4 c(.6f,.6f,.6f,.6f);	return c;}
const Color4& GuiItem::translucentDebugBackdrop(){static Color4 c(.0f,.0f,.0f,.6f); return c;}
const Color4& GuiItem::menuSelect()              {static Color4 c(.7f,.7f,.7f,1.0f);return c;}

const char* const sGuiRoot = "GuiRoot";

GuiItem::GuiItem()
{
	setName("Unnamed GuiItem");
	FASTLOG1(FLog::GuiTargetLifetime, "GuiItem created: %p", this);
}

GuiItem::~GuiItem()
{
	FASTLOG1(FLog::GuiTargetLifetime, "GuiItem destroyed: %p", this);
}


GuiItem* GuiItem::getGuiParent()
{
	return Instance::fastDynamicCast<GuiItem>(this->getParent());
}

const GuiItem* GuiItem::getGuiParent() const		
{
	return Instance::fastDynamicCast<GuiItem>(this->getParent());
}

GuiItem* GuiItem::getGuiItem(int index) 
{
	return Instance::fastDynamicCast<GuiItem>(this->getChild(index));
}

const GuiItem* GuiItem::getGuiItem(int index) const	
{
	return Instance::fastDynamicCast<GuiItem>(this->getChild(index));
}

void GuiItem::onDescendantRemoving(const shared_ptr<Instance>& instance)
{
	if (instance == focus)
		loseFocus();

	Super::onDescendantRemoving(instance);
}



Rect GuiItem::getMyRect(Canvas canvas) const	
{
	Vector2 pos = getPosition(canvas);
	return Rect(pos, pos + this->getSize(canvas));
}


GuiResponse GuiItem::processNonFocus(const shared_ptr<InputObject>& event)
{
	for (size_t i = 0; i < numChildren(); ++i) {
		if (GuiItem* item = getGuiItem(i)) {
			if (item != focus.get() && item != NULL) {
				GuiResponse itemResponse = item->process(event);
				if (itemResponse.wasSunk()) 
				{
					this->loseFocus();
					focus = shared_from(item);
					focus->loseFocus();		// make sure no focus has no focus of its own
					return itemResponse;
				}
			}
		}
	}
	return GuiResponse::notSunk();
}



// Throw away idle events
GuiResponse GuiItem::process(const shared_ptr<InputObject>& event)
{
	// flush idle events
	if (event->isMouseEvent() && event->getUserInputType() == InputObject::TYPE_MOUSEIDLE)
	{
		return GuiResponse::notSunk();
	}

	// if focus.....
	if (focus)
	{
		if (focus->canLoseFocus())
		{ 
			GuiResponse nonFocus = processNonFocus(event);
			if (nonFocus.wasSunk()) 
				return nonFocus;
		}

		GuiResponse focusResponse = focus->process(event);
		if (focusResponse.wasSunk())
		{
			return focusResponse;
		}
		else 
		{
			loseFocus();
		}
	}

	RBXASSERT(focus == NULL);
	return processNonFocus(event);
}


void GuiItem::label2d(
	Adorn* adorn,
	const std::string& label,
	const Color4& fill,
	const Color4& border,
	Text::XAlign align) const
{
	if (label.length()) {
		Rect myRect = this->getMyRect(adorn->getCanvas());
		Vector2 s = myRect.size();
		Vector2 pos = myRect.center();
		switch (align)
		{
		default:
		case Text::XALIGN_LEFT:
			pos.x = myRect.low.x + (0.1f * s.x);
			break;

		case Text::XALIGN_RIGHT:
			pos.x = myRect.high.x - (0.1f * s.x);
			break;

		case Text::XALIGN_CENTER:		// pos.x == center
			break;
		}

		adorn->drawFont2D(		label,
								pos,
								(float)getFontSize(),		// size
                                false,
								fill,
								border,
								Text::FONT_LEGACY,
								align,
								Text::YALIGN_CENTER );

	}
}

/////////////////////////////////////////////////////////////////////////////////////
//
// GuiRoot
//

GuiRoot::GuiRoot() 
{
	setName("GuiRoot");
}

Vector2 Canvas::toPixelSize(const Vector2& percent) const		// std screen is 100% wide and 75% tall
{
	Vector2 answer;
	if (size.y > (size.x * 0.75f))	// bound by x - y too big
	{

		answer = (0.01f * percent) * Vector2(size.x, size.x * 0.75f);
	}
	else
	{
		answer = (0.01f * percent) * Vector2(size.y * 1.33f, size.y);
	}
	return answer;
}

int Canvas::normalizedFontSize(int fontSize) const
{
	static Vector2 percentSize(100.0f, 75.0f);

	return Math::iFloor(fontSize * toPixelSize(percentSize).x / 1000.0f);
}

void GuiRoot::render2d(Adorn* adorn)
{
	for (size_t i = 0; i < numChildren(); i++) {
		if (GuiItem* item = getGuiItem(i))
			item->render2d(adorn);
	}
}

void GuiRoot::render2dItem(Adorn* adorn, GuiItem* guiItem)
{
	guiItem->render2d(adorn);
}

/////////////////////////////////////////////////////////////////////////////////////
//
// ScreenPanel
//

void RelativePanel::init(const Layout& layout) 
{
	setName("RelativePanel");
	layoutStyle = layout.layoutStyle;
	backdropColor = layout.backdropColor;
	xLocation = layout.xLocation;
	yLocation = layout.yLocation;
	offset = layout.offset;
}


Vector2 RelativePanel::getPosition(Canvas canvas) const
{
	Rect canvasRect(Vector2::zero(), canvas.size);
	Rect me(Vector2::zero(), getSize(canvas));
			
	canvasRect = canvasRect.inset(offset);
	return canvasRect.positionChild(me, xLocation, yLocation).low;
}

/////////////////////////////////////////////////////////////////////////////////////
//
// TopMenuBar
//
void TopMenuBar::init()
{
	layoutStyle = Layout::HORIZONTAL;
	backdropColor = Color4::clear();
	visible = true;
}

TopMenuBar::TopMenuBar(
	const std::string& _title, 
	Layout::Style _layoutStyle,
	bool translucentBackdrop)
{
	init();
	setName(_title);
	layoutStyle = _layoutStyle;
	if (translucentBackdrop) {
		backdropColor = GuiItem::translucentBackdrop();
	}
}

TopMenuBar::TopMenuBar(
	const std::string& _title, 
	Layout::Style _layoutStyle,
	Color4 _backdropColor)
{
	init();
	setName(_title);
	layoutStyle = _layoutStyle;
	backdropColor = _backdropColor;
}

GuiResponse TopMenuBar::process(const shared_ptr<InputObject>& event)
{
	if (isVisible()) 
	{
		GuiResponse childResponse = GuiItem::process(event);
		if (childResponse.wasSunk()) {
			return childResponse;
		}

		// if not a transparent background, grab all mouse events
		else if (	(backdropColor.a > 0.0)
				&&	event->isMouseEvent() 
				&&	getMyRect(Canvas(event->getWindowSize())).pointInRect(event->get2DPosition())) 
		{
			return GuiResponse::sunk();
		}
	}
	return GuiResponse::notSunk();
}


Vector2 TopMenuBar::getSize(Canvas canvas) const
{
	Vector2 answer;		// set to zero

	for (size_t i = 0; i < numChildren(); ++i) {
		if (const GuiItem* item = getGuiItem(i))
		{
			Vector2 childSize = item->getSize(canvas);
			switch (layoutStyle)
			{
			case Layout::HORIZONTAL:
				answer.x += childSize.x;
				answer.y = std::max(answer.y, childSize.y);
				break;
			case Layout::VERTICAL:
				answer.x = std::max(answer.x, childSize.x);
				answer.y += childSize.y;
				break;
			default:
				RBXASSERT(0);
				break;
			}
		}
	}
	return answer;
}


Vector2 TopMenuBar::getChildPosition(const GuiItem* child, Canvas canvas) const
{
	Vector2 childPosition = this->getPosition(canvas);
	Vector2 mySize = this->getSize(canvas);

	for (size_t i = 0; i < numChildren(); ++i) {
		const GuiItem* myChild = getGuiItem(i);
		if (myChild)
		{
			Vector2 myChildSize = myChild->getSize(canvas);

			if (myChild == child) {
				int perp = (layoutStyle + 1) % 2;
				// center it
				childPosition[perp] += 0.5f * (mySize[perp] - myChildSize[perp]);
				return childPosition;
			}
			switch (layoutStyle)
			{
			case Layout::HORIZONTAL:
			case Layout::VERTICAL:
				childPosition[layoutStyle] += myChild->getSize(canvas)[layoutStyle];		// x, y
				break;

			default:
				RBXASSERT(0);
				break;
			}
		}
	}

	RBXASSERT(0);
	return childPosition;
}

void TopMenuBar::render2d(Adorn* adorn)
{
	if (isVisible()) {
		if (backdropColor != Color4::clear()) {
			adorn->rect2d(getMyRect2D(adorn->getCanvas()), backdropColor);
		}

		for (size_t i = 0; i < numChildren(); i++) {
			if (GuiItem* item = getGuiItem(i))
				item->render2d(adorn);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////
//
// UnifiedWidget
//
void UnifiedWidget::init()
{
	setGuiSize(Vector2(140, 24));
	menuState = NOTHING;
}

UnifiedWidget::UnifiedWidget(const std::string& title)
{
	init();
	setName(title);
}

void UnifiedWidget::render2dMe(Adorn* adorn)
{
	adorn->rect2d(getMyRect2D(adorn->getCanvas()), (menuState != NOTHING) ? menuSelect() : Color3::white());

	adorn->outlineRect2d(getMyRect2D(adorn->getCanvas()), 1, Color3::black());

	label2d(
		adorn,
		getName(), 
		Color3::black(),
		Color4::clear(),
		Text::XALIGN_CENTER
	);
}

void UnifiedWidget::render2dChildren(Adorn* adorn)
{
	if (showChildren())
	{
		for (size_t i = 0; i < numChildren(); i++) {
			if (GuiItem* item = getGuiItem(i)) {
				item->render2d(adorn);
			}
		}
	}
}

void UnifiedWidget::render2d(Adorn* adorn)
{
	if (isVisible()) 
	{
		render2dMe(adorn);
		render2dChildren(adorn);
	}
}

// each item is 24 units high

Vector2 UnifiedWidget::firstChildPosition(Canvas canvas) const
{
	Vector2 childPosition = getPosition(canvas);
	Vector2 mySize = getSize(canvas);

	childPosition.x += (mySize.x + 4);

	int canvasHeight = (int)canvas.size.y;
	int minY = 20;
	int maxY = canvasHeight - 100;
	int slotHeight = (int)mySize.y + 4;
	int slots = (maxY - minY) / slotHeight;
	int desiredCenter = (int)childPosition.y;
	int desiredCenterSlot = (desiredCenter - minY) / slotHeight;

	int desiredSlots = this->numChildren();
	int topCenterSlot = desiredSlots / 2;
	int bottomCenterSlot = (slots - topCenterSlot);

	desiredCenterSlot = std::max(topCenterSlot, desiredCenterSlot);
	desiredCenterSlot = std::min(bottomCenterSlot, desiredCenterSlot);

	childPosition.y = (float)(((desiredCenterSlot - topCenterSlot)* slotHeight) + minY);
	return childPosition;
}

Vector2 UnifiedWidget::childOffset() const
{
//	return Vector2(0, -28);
	return Vector2(0, 28);
}

Vector2 UnifiedWidget::getChildPosition(const GuiItem* child, Canvas canvas) const
{
	Vector2 position = firstChildPosition(canvas);
	
	for (size_t i = 0; i < numChildren(); ++i) {
		if (const GuiItem* item = getGuiItem(i)) {
			if (item == child) {
				return position;
			}
			position += childOffset();
		}
	}

	RBXASSERT(0);
	return position;
}


void UnifiedWidget::onLoseFocus()
{
	menuState = NOTHING; 
	Super::onLoseFocus();
}


void UnifiedWidget::setMenuState(MenuState value)
{
	if (menuState != value)
	{
		menuState = value;
		onMenuStateChanged();
	}
}

GuiResponse UnifiedWidget::processKey(const shared_ptr<InputObject>& event)
{
	return GuiResponse::notSunk();;
}


GuiResponse UnifiedWidget::processShown_InTitle(const shared_ptr<InputObject>& event)
{
	loseFocus();

	if (menuState == SHOWN)
	{
		if (event->isLeftMouseUpEvent())
		{
			setMenuState(HOVER);
		}
		return GuiResponse::sunk();
	}

	RBXASSERT(menuState == SHOWN_APPEARING);

	if (event->isLeftMouseUpEvent())
	{
		setMenuState(SHOWN);
	}
	return GuiResponse::sunk();
}



GuiResponse UnifiedWidget::processShown(const shared_ptr<InputObject>& event)
{
	if (getMyRect(Canvas(event->getWindowSize())).pointInRect(event->get2DPosition()))
	{
		return processShown_InTitle(event);
	}

	else 
	{
		return processShown_OutOfTitle(event);
	}
}




GuiResponse UnifiedWidget::processHover(const shared_ptr<InputObject>& event)
{
	RBXASSERT(menuState == HOVER);
	loseFocus();

	if (getMyRect(Canvas(event->getWindowSize())).pointInRect(event->get2DPosition()))
	{
		if (event->isLeftMouseDownEvent())
		{
			setMenuState(SHOWN_APPEARING);	
			return GuiResponse::sunk();
		}
		else if(event->isLeftMouseUpEvent())
		{
			setMenuState(SHOWN);	
			return GuiResponse::sunk();
		}
		return GuiResponse::sunk();
	}
	else
	{
		setMenuState(NOTHING);
		return GuiResponse::notSunk();
	}
}


GuiResponse UnifiedWidget::processNothing(const shared_ptr<InputObject>& event)
{
	RBXASSERT(menuState == NOTHING);
	loseFocus();

	if (getMyRect(Canvas(event->getWindowSize())).pointInRect(event->get2DPosition()))
	{
		switch (event->getUserInputType()) 
		{
			case InputObject::TYPE_MOUSEMOVEMENT:			// coming in with a mouse down or up?
				setMenuState(HOVER);
				return GuiResponse::sunk();
			case InputObject::TYPE_MOUSEBUTTON1:
				RBXASSERT(event->isLeftMouseDownEvent() || event->isLeftMouseUpEvent());
				setMenuState(SHOWN_APPEARING);	
				return GuiResponse::sunk();
			default:
				return GuiResponse::notSunk();
		}
	}
	else
	{
		setMenuState(NOTHING);
		return GuiResponse::notSunk();
	}
}


GuiResponse UnifiedWidget::processShown_OutOfTitle(const shared_ptr<InputObject>& event)
{
	UnifiedWidget* oldMenuChild = Instance::fastDynamicCast<UnifiedWidget>(getFocus());

	switch (event->getUserInputType()) 
	{
		// Mouse move - child can handle - always used
		case InputObject::TYPE_MOUSEMOVEMENT:	
			{
				MenuState oldState = oldMenuChild ? oldMenuChild->getMenuState() : NOTHING;

				GuiItem::process(event);

				if (oldMenuChild) {
					if (UnifiedWidget* newMenuChild = Instance::fastDynamicCast<UnifiedWidget>(getFocus())) {
						if (newMenuChild != oldMenuChild) {
							newMenuChild->setMenuState(oldState);
						}
					}
				}
				return GuiResponse::sunk();
			}

		// UP or Down - if not used, lose focus and hide our menu
		case InputObject::TYPE_MOUSEBUTTON1:
			{			
				RBXASSERT(event->isLeftMouseDownEvent() || event->isLeftMouseUpEvent());
				GuiResponse answer = GuiItem::process(event);

				if (!answer.wasSunk()) {
					RBXASSERT(getFocus() == NULL);
					setMenuState(NOTHING);
				}

				if (answer.wasSunkAndFinished()) {
					loseFocus();
					setMenuState(NOTHING);
				}
		
				return answer;
			}
            
        default:
            break;
	}

	RBXASSERT(0);
	return GuiResponse::notSunk();
}



GuiResponse UnifiedWidget::process(const shared_ptr<InputObject>& event)
{
	if (isVisible())
	{
		if (event->isMouseEvent()) 
		{
			switch (menuState)
			{
			case UnifiedWidget::NOTHING:			return processNothing(event);
			case UnifiedWidget::HOVER:				return processHover(event);
			case UnifiedWidget::SHOWN_APPEARING:	
			case UnifiedWidget::SHOWN:				return processShown(event);		// with or without focus....
			}
		}
		else 
		{
			return processKey(event);
		}
	}
	return GuiResponse::notSunk();
}



///////////////////////////////////////////////////////////////////////////
//
// TextDisplay
//
void TextDisplay::init()
{
	setGuiSize(Vector2(180, 30));
	fontSize = 18;
	fontColor = Color3::purple();
	borderColor = Color4::clear();
	align = Text::XALIGN_LEFT;
	visible = true;
}

TextDisplay::TextDisplay(const std::string& title, const std::string& _label) 
{
	init();
	setName(title);
	label = _label;
}


void TextDisplay::render2d(Adorn* adorn)
{
	if (isVisible())
	{
		label2d(
			adorn,
			getLabel(),
			fontColor,
			borderColor,
			align);
	}
}

Vector2 TextDisplay::getSize(Canvas canvas) const					
{
	if (isVisible())
	{
		return Super::getSize(canvas);
	}
	return Vector2::zero();
}

} // namespace