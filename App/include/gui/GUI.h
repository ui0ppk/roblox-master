/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/InputObject.h"
#include "Gui/GuiEvent.h"
#include "Gui/Layout.h"
#include "V8Tree/Instance.h"
#include "GfxBase/Type.h"
#include "GfxBase/Adorn.h"

namespace RBX {


extern const char* const sGuiItem;
class GuiItem : public DescribedNonCreatable<GuiItem, Instance, sGuiItem>
{
private:
	typedef Instance Super;
	shared_ptr<GuiItem>		focus;	// TODO: It should be safe to make this a bald pointer, since focus is always a child
	Vector2 guiSize;

	GuiResponse processNonFocus(const shared_ptr<InputObject>& event);
	void switchFocus(GuiItem* item);

	// Instance
	/*override*/ void onDescendantRemoving(const shared_ptr<Instance>& instance);
	/*override*/ bool askAddChild(const Instance* instance) const {
		return Instance::fastDynamicCast<GuiItem>(instance)!=0;
	}
	/*override*/ const RBX::Name& getClassName() const	{return RBX::Name::getNullName(); }

protected:
	GuiItem* getFocus() {return focus.get();}
	void loseFocus() {
		if (focus) {
			focus->onLoseFocus();
		}
		focus.reset();
	}
	
	Rect getMyRect(Canvas canvas) const;				// myPosition, myPosition + mySize

	Rect2D getMyRect2D(Canvas canvas) const {
		return getMyRect(canvas).toRect2D();
	}

	void label2d(
		Adorn* adorn, 
		const std::string& label,
		const Color4& fill,
		const Color4& border,
		Text::XAlign align = Text::XALIGN_LEFT) const;

	/////////////////////////////////////////////////////////////
	//
	// GUI Item virtuals

	virtual void onLoseFocus()	{}		
	virtual bool canLoseFocus()	{return false;}	 // in general, Gui Items don't lose focus while processing

	// standard virtual overrides....
	virtual Vector2 getPosition(Canvas canvas) const {
		RBXASSERT(getGuiParent());
		return getGuiParent()->getChildPosition(this, canvas);
	}

	virtual Vector2 getChildPosition(const GuiItem* child, Canvas canvas) const {		// This should always be override if ever used
		RBXASSERT(0);
		return Vector2::zero();
	}

	// used internally - could these be protected?
	virtual int getFontSize() const	{return 12;}

	virtual bool isVisible() const	{return true;}

	virtual std::string getTitle() {return getName();}
	
public:
	virtual Vector2 getSize(Canvas canvas) const					{return getGuiSize();}

	virtual GuiResponse process(const shared_ptr<InputObject>& event);

	virtual void render2d(Adorn* adorn) {}


	GuiItem();
	~GuiItem();

	void addGuiItem(shared_ptr<GuiItem> guiItem)	{guiItem->setParent(this);}

	void setGuiSize(const Vector2& size)	{guiSize = size;}
	const Vector2& getGuiSize() const		{return guiSize;}

	GuiItem* getGuiParent();
	const GuiItem* getGuiParent() const;
	GuiItem* getGuiItem(int index);
	const GuiItem* getGuiItem(int index) const;

	static const Color4& disabledFill();
	static const Color4& translucentBackdrop();// background of the palette
	static const Color4& translucentDebugBackdrop();// background of debug menu palette
	static const Color4& menuSelect();
};

extern const char* const sGuiRoot;

class GuiRoot : 
	public Reflection::Described<GuiRoot, sGuiRoot, GuiItem, Reflection::ClassDescriptor::INTERNAL_LOCAL, RBX::Security::LocalUser> 
{
public:
	GuiRoot();

	/*override*/ void render2d(Adorn* adorn);

	void render2dItem(Adorn* adorn, GuiItem* guiItem);

	// GuiItem
	/*override*/ Vector2 getSize(Canvas canvas) const {
		// shouldn't be called - implies item doesn't know it's top level
		RBXASSERT(false);
		return canvas.size;
	}

protected:
	virtual bool askSetParent(const Instance* instance) const {
		return true;
	}
};


class TopMenuBar : public GuiItem {
private:
	void init();

protected:
	Color4					backdropColor;
	Layout::Style				layoutStyle;
	bool						visible;

	/*override*/ Vector2 getChildPosition(const GuiItem* child, Canvas canvas) const;

public:
	TopMenuBar() {init();}
	TopMenuBar(
		const std::string& _title, 
		Layout::Style layoutStyle,
		bool translucentBackdrop = false);
	TopMenuBar(
		const std::string& _title, 
		Layout::Style _layoutStyle,
		Color4 _backdropColor);

	/*override*/ GuiResponse process(const shared_ptr<InputObject>& event);
	/*override*/ void render2d(Adorn* adorn);
	/*override*/ Vector2 getSize(Canvas canvas) const;
	/*override*/ bool isVisible() const {return visible;}

	void setVisible(bool _set)		{visible = _set;}
};



class RelativePanel : public TopMenuBar {
protected:
	Rect::Location xLocation;
	Rect::Location yLocation;
	Vector2int16 offset;

	void init(const Layout& layout);

public:
	RelativePanel()							{init(Layout());}
	RelativePanel(const Layout& layout)		{init(layout);}

	virtual Vector2 getPosition(Canvas canvas) const;
};

// This will become the new unified widget - menu, button, all...
//

class UnifiedWidget : public GuiItem {
public:
	enum MenuState {NOTHING, HOVER, SHOWN_APPEARING, SHOWN};		// + focus if child is shown

private:
	typedef GuiItem Super;
	MenuState menuState;

	GuiResponse processShown_InTitle(const shared_ptr<InputObject>& event);
	GuiResponse processShown_OutOfTitle(const shared_ptr<InputObject>& event);

	GuiResponse processNothing(const shared_ptr<InputObject>& event);
	GuiResponse processHover(const shared_ptr<InputObject>& event);
	GuiResponse processShown(const shared_ptr<InputObject>& event);
	GuiResponse processKey(const shared_ptr<InputObject>& event);

	void render2dChildren(Adorn* adorn);

	/*override*/ Vector2 getChildPosition(const GuiItem* child, Canvas canvas) const;
	/*override*/ bool canLoseFocus()		{return true;}
	/*override*/ void onLoseFocus();
	/*override*/ int getFontSize() const	{return 8;}

	void init();

protected:
	bool showChildren() {return (menuState >= SHOWN_APPEARING);}

	virtual void onMenuStateChanged() {}
	virtual Vector2 firstChildPosition(Canvas canvas) const;
	virtual Vector2 childOffset() const;
	virtual void render2dMe(Adorn* adorn); 

public:
	UnifiedWidget() {init();}
	UnifiedWidget(const std::string& title);

	MenuState getMenuState() const					{return menuState;}
	void setMenuState(MenuState value);

	/*override*/ GuiResponse process(const shared_ptr<InputObject>& event);
	/*override*/ void render2d(Adorn* adorn);
};





class TextDisplay : public GuiItem {
private:
	typedef GuiItem Super;

	void init();

protected:
	std::string			label;
	int					fontSize;
	Color4				fontColor;
	Color4				borderColor;
	Text::XAlign		align;
	bool				visible;

	/*override*/ int getFontSize() const				{return fontSize;}
	/*override*/ bool isVisible() const					{return visible;}

	const std::string& getLabel() const					{return label;}

public:
	TextDisplay() {init();}
	TextDisplay(const std::string& title, const std::string& _label);

	/*override*/ void render2d(Adorn* adorn);
	/*override*/ Vector2 getSize(Canvas canvas) const;

	void setLabel(const std::string& _label)			{label = _label;}
	void setFontSize(int _fontSize) {
		fontSize = _fontSize; 
		setGuiSize(Vector2(fontSize*10.0f, fontSize*2.0f));
	}
	void setFontColor(const Color4& _color)		{fontColor = _color;}
	void setBorderColor(const Color4& _color)	{borderColor = _color;}
	void setAlign(Text::XAlign _align)			{align = _align;}
	void setVisible(bool _visible)				{visible = _visible;}
};


} // namespace
