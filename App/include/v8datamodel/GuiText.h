#pragma once

#include "Util/BrickColor.h"
#include "Util/TextureId.h"
#include "Gui/ProfanityFilter.h"
#include "Util/ContentFilter.h"
#include "GfxBase/Typesetter.h"
#include "V8DataModel/TextService.h"
#include "security/SecurityContext.h"
#include "Network/Players.h"

#define category_Text       "Text"

namespace RBX {

class GuiTextMixin
{
public:
		GuiTextMixin(const std::string& text, const Color3& textColor)
			: text(text)
			, fontSize(TextService::SIZE_8)
			, textColor(textColor)
			, textTransparency(0)
			, textWrap(false)
			, textScale(false)
			, filterState(ContentFilter::Waiting)
			, xAlignment(TextService::XALIGNMENT_CENTER)
			, yAlignment(TextService::YALIGNMENT_CENTER)
			, font(TextService::FONT_LEGACY)
			, textStrokeTransparency(1.0f)
			, textStrokeColor(0,0,0)
		{}

	const std::string& getText() const { return text; }

	TextService::FontSize getFontSize() const { return fontSize; }

	TextService::Font getFont() const { return font; }

	BrickColor getTextColor() const { return BrickColor::closest(textColor); }

	Color3 getTextColor3() const { return textColor; }

	float getTextTransparency() const { return textTransparency; }

	float getTextStrokeTransparency() const { return textStrokeTransparency; }

	Color3 getTextStrokeColor3() const { return textStrokeColor; }

	bool getTextWrap() const { return textWrap; }

	bool getTextScale() const { return textScale; }

	TextService::XAlignment getXAlignment() const { return xAlignment; }
	
	TextService::YAlignment getYAlignment() const { return yAlignment; }

protected:
		float getRenderTextAlpha(float transparency) const
		{
			return G3D::clamp(1.0f - transparency, 0.0f, 1.0f);
		}
		Color4 getRenderTextColor4() const
		{
			return Color4(textColor,getRenderTextAlpha(getTextTransparency()));
		}
		Color4 getRenderTextStrokeColor4() const
		{
			return Color4(textStrokeColor,getRenderTextAlpha(getTextStrokeTransparency()));
		}

		ContentFilter::FilterResult filterState;
		std::string text;
		TextService::FontSize fontSize;
		Color3 textColor;
		float textTransparency;
		Color3 textStrokeColor;
		float textStrokeTransparency;
		bool textWrap;
		bool textScale;
		TextService::XAlignment xAlignment;
		TextService::YAlignment yAlignment;
		TextService::Font font;
};

#define DECLARE_GUI_TEXT_MIXIN()							\
/*override*/ void checkForResize();							\
void setText(std::string value);							\
void setFontSize(TextService::FontSize value);				\
void setFont(TextService::Font value);						\
void setTextColor(BrickColor value);						\
void setTextColor3(Color3 value);							\
void setTextTransparency(float value);						\
void setTextStrokeTransparency(float value);				\
void setTextStrokeColor3(Color3 value);						\
void setTextWrap(bool value);								\
void setTextScale(bool value);								\
void setXAlignment(TextService::XAlignment value);			\
void setYAlignment(TextService::YAlignment value);			\
int getPosInString(RBX::Vector2 cursorPos) const;			\
Vector2 getTextBounds() const;								\
bool getTextFits() const;									\
/*override */ void setTransparencyLegacy(float value);		\
/*override*/ int getPersistentDataCost() const;				

#define IMPLEMENT_GUI_TEXT_MIXIN(Class)																																																	\
REFLECTION_BEGIN();                                                                                                                                                                                                                     \
static const Reflection::PropDescriptor<Class, std::string> prop_Text("Text", category_Text, &Class::getText, &Class::setText);																											\
static const Reflection::EnumPropDescriptor<Class, TextService::FontSize>	prop_FontSize("FontSize", category_Text, &Class::getFontSize, &Class::setFontSize);																			\
static const Reflection::EnumPropDescriptor<Class, TextService::Font>	prop_Font("Font", category_Text, &Class::getFont, &Class::setFont);																								\
static const Reflection::PropDescriptor<Class, BrickColor>  prop_TextColor("TextColor", category_Text, &Class::getTextColor, &Class::setTextColor, Reflection::PropertyDescriptor::LEGACY_SCRIPTING);									\
static const Reflection::PropDescriptor<Class, Color3>	    prop_TextColor3("TextColor3", category_Text, &Class::getTextColor3, &Class::setTextColor3);																					\
static const Reflection::PropDescriptor<Class, float> prop_TextTransparency("TextTransparency", category_Text, &Class::getTextTransparency, &Class::setTextTransparency);																\
static const Reflection::PropDescriptor<Class, bool> prop_TextWrap("TextWrapped", category_Text, &Class::getTextWrap, &Class::setTextWrap);																								\
static const Reflection::PropDescriptor<Class, bool> prop_depTextWrap("TextWrap", category_Text, &Class::getTextWrap, &Class::setTextWrap, Reflection::PropertyDescriptor::Attributes::deprecated(prop_TextWrap, Reflection::PropertyDescriptor::UI));	\
static const Reflection::PropDescriptor<Class, bool> prop_TextScale("TextScaled", category_Text, &Class::getTextScale, &Class::setTextScale);																							\
static const Reflection::EnumPropDescriptor<Class, TextService::XAlignment> prop_TextXAlignment("TextXAlignment", category_Text, &Class::getXAlignment, &Class::setXAlignment);															\
static const Reflection::EnumPropDescriptor<Class, TextService::YAlignment> prop_TextYAlignment("TextYAlignment", category_Text, &Class::getYAlignment, &Class::setYAlignment);															\
static const Reflection::PropDescriptor<Class, Vector2> prop_TextBounds("TextBounds", category_Text, &Class::getTextBounds, NULL, Reflection::PropertyDescriptor::UI);																	\
static const Reflection::PropDescriptor<Class, bool>    prop_TextFits("TextFits", category_Text, &Class::getTextFits, NULL, Reflection::PropertyDescriptor::UI);																		\
static const Reflection::PropDescriptor<Class, Color3>  prop_TextStrokeColor3("TextStrokeColor3", category_Text, &Class::getTextStrokeColor3, &Class::setTextStrokeColor3);							\
static const Reflection::PropDescriptor<Class, float>   prop_TextStrokeTransparency("TextStrokeTransparency", category_Text, &Class::getTextStrokeTransparency, &Class::setTextStrokeTransparency);	\
REFLECTION_END();                                                                                                                                                                                   \
                            																																				\
void Class::checkForResize()																																				\
{																																											\
	Super::checkForResize();																																				\
	raisePropertyChanged(prop_TextBounds);																																	\
	raisePropertyChanged(prop_TextFits);																																	\
}																																											\
                            																																				\
void Class::setText(std::string value)																																		\
{																																											\
	if(value.size() > ContentFilter::MAX_CONTENT_FILTER_SIZE){																												\
		value = value.substr(0, ContentFilter::MAX_CONTENT_FILTER_SIZE);																									\
	}																																										\
	if(!ProfanityFilter::ContainsProfanity(value) || getRobloxLocked()){																									\
		if(GuiTextMixin::text != value){																																	\
			bool didTextFit = getTextFits();																																\
			GuiTextMixin::text = value;																																		\
			filterState = ContentFilter::Waiting;																															\
			raisePropertyChanged(prop_Text);																																\
			raisePropertyChanged(prop_TextBounds);																															\
			if(didTextFit != getTextFits())																																	\
				raisePropertyChanged(prop_TextFits);																														\
		}																																									\
	}																																										\
}																																											\
                            																																				\
void Class::setFontSize(TextService::FontSize value)																														\
{																																											\
	if (GuiTextMixin::fontSize != value) {																																	\
		GuiTextMixin::fontSize = value;																																		\
		raisePropertyChanged(prop_FontSize);																																\
		raisePropertyChanged(prop_TextBounds);																																\
	}																																										\
}																																											\
                            																																				\
void Class::setFont(TextService::Font value)																																\
{																																											\
	if (GuiTextMixin::font != value) {																																		\
		GuiTextMixin::font = value;																																			\
		raisePropertyChanged(prop_Font);																																	\
		raisePropertyChanged(prop_TextBounds);																																\
	}																																										\
}																																											\
                            																																				\
void Class::setTextColor(BrickColor value)																																	\
{																																											\
	setTextColor3(value.color3());																																			\
}																																											\
                            																																				\
void Class::setTextColor3(Color3 value)																																		\
{																																											\
	if(GuiTextMixin::textColor != value){																																	\
		GuiTextMixin::textColor = value;																																	\
		raisePropertyChanged(prop_TextColor);																																\
		raisePropertyChanged(prop_TextColor3);																																\
	}																																										\
}																																											\
                            																																				\
void Class::setTextTransparency(float value)																																\
{																																											\
	if(GuiTextMixin::textTransparency != value){																															\
		GuiTextMixin::textTransparency = value;																																\
		raisePropertyChanged(prop_TextTransparency);																														\
	}																																										\
}																																											\
																																											\
void Class::setTextStrokeTransparency(float value)																															\
{																																											\
	if(GuiTextMixin::textStrokeTransparency != value){																														\
		GuiTextMixin::textStrokeTransparency = value;																														\
		raisePropertyChanged(prop_TextStrokeTransparency);																													\
	}																																										\
}																																											\
																																											\
void Class::setTextStrokeColor3(Color3 value)																																\
{																																											\
	if(GuiTextMixin::textStrokeColor != value){																																\
		GuiTextMixin::textStrokeColor = value;																																\
		raisePropertyChanged(prop_TextStrokeColor3);																														\
	}																																										\
}																																											\
                            																																				\
void Class::setTextWrap(bool value)																																			\
{																																											\
	if(GuiTextMixin::textWrap != value){																																	\
		bool didTextFit = getTextFits();																																	\
		GuiTextMixin::textWrap = value;																																		\
		raisePropertyChanged(prop_TextWrap);																																\
		raisePropertyChanged(prop_TextBounds);																																\
		if(didTextFit != getTextFits())																																		\
			raisePropertyChanged(prop_TextFits);																															\
	}																																										\
}																																											\
void Class::setTextScale(bool value)																																		\
{																																											\
	if(GuiTextMixin::textScale != value){																																	\
		GuiTextMixin::textScale = value;																																	\
		raisePropertyChanged(prop_TextScale);																																\
		if(value)																																							\
			setTextWrap(value);																																				\
		else																																								\
		{																																									\
			raisePropertyChanged(prop_TextBounds);																															\
			raisePropertyChanged(prop_TextFits);																															\
		}																																									\
	}																																										\
}																																											\
                            																																				\
void Class::setXAlignment(TextService::XAlignment value)																													\
{																																											\
	if(GuiTextMixin::xAlignment != value){																																	\
		bool didTextFit = getTextFits();																																	\
		GuiTextMixin::xAlignment = value;																																	\
		raisePropertyChanged(prop_TextXAlignment);																															\
		raisePropertyChanged(prop_TextBounds);																																\
		if(didTextFit != getTextFits())																																		\
			raisePropertyChanged(prop_TextFits);																															\
	}																																										\
}																																											\
                            																																				\
void Class::setYAlignment(TextService::YAlignment value)																													\
{																																											\
	if(GuiTextMixin::yAlignment != value){																																	\
		bool didTextFit = getTextFits();																																	\
		GuiTextMixin::yAlignment = value;																																	\
		raisePropertyChanged(prop_TextYAlignment);																															\
		raisePropertyChanged(prop_TextBounds);																																\
		if(didTextFit != getTextFits())																																		\
			raisePropertyChanged(prop_TextFits);																															\
	}																																										\
}																																											\
                            																																				\
void Class::setTransparencyLegacy(float value)																																\
{																																											\
	setTextTransparency(value);																																				\
	Super::setTransparencyLegacy(value);																																	\
}																																											\
                            																																				\
Vector2 Class::getTextBounds()	const																																		\
{																																											\
	if(Network::Players::frontendProcessing(this, false))																													\
		if(TextService* textService = ServiceProvider::create<TextService>(this))																							\
		{																																									\
			if(Typesetter* typesetter = textService->getTypesetter(GuiTextMixin::font))																		                \
				return typesetter->measure(	GuiTextMixin::text,																												\
											GuiObject::convertFontSize(GuiTextMixin::fontSize),																				\
											GuiTextMixin::textWrap ? getRect2D().wh() : Vector2::zero());																	\
		}																																									\
																																											\
	return Vector2::zero();																																					\
}																																											\
                            																																				\
int Class::getPosInString(RBX::Vector2 cursorPos) const																														\
{																																											\
	if(Network::Players::frontendProcessing(this, false))																													\
		if(TextService* textService = ServiceProvider::create<TextService>(this))																							\
		{																																									\
			if(Typesetter* typesetter = textService->getTypesetter(GuiTextMixin::font))																		                \
			{																																								\
				Vector2 pos(0,0);																																			\
				Rect2D rect(getRect2D());																																	\
																																											\
				switch(getXAlignment())																																		\
				{																																							\
					case TextService::XALIGNMENT_LEFT:																														\
						pos.x = rect.x0();																																	\
						break;																																				\
					case TextService::XALIGNMENT_RIGHT:																														\
						pos.x = rect.x1();																																	\
						break;																																				\
					case TextService::XALIGNMENT_CENTER:																													\
						pos.x = rect.center().x;																															\
						break;																																				\
				}																																							\
				switch(getYAlignment())																																		\
				{																																							\
					case TextService::YALIGNMENT_TOP:																														\
						pos.y = rect.y0();																																	\
						break;																																				\
					case TextService::YALIGNMENT_CENTER:																													\
						pos.y = rect.center().y;																															\
						break;																																				\
					case TextService::YALIGNMENT_BOTTOM:																													\
						pos.y = rect.y1();																																	\
						break;																																				\
				}																																							\
				return typesetter->getCursorPositionInText(	getText(),																										\
															pos,																											\
															GuiObject::convertFontSize(GuiTextMixin::fontSize),																\
															TextService::ToTextXAlign(GuiTextMixin::getXAlignment()),														\
															TextService::ToTextYAlign(GuiTextMixin::getYAlignment()),														\
															GuiTextMixin::textWrap ? getRect2D().wh() : Vector2::zero(),													\
															absoluteRotation,																								\
															cursorPos																										\
															);																												\
			}																																								\
		}																																									\
																																											\
	return -1;																																								\
}																																											\
                            																																				\
bool Class::getTextFits() const																																				\
{																																											\
	if(Network::Players::frontendProcessing(this, false))																													\
		if(TextService* textService = ServiceProvider::create<TextService>(this))																							\
		{																																									\
			if(Typesetter* typesetter = textService->getTypesetter(GuiTextMixin::font)){																	                \
				bool result = false;																																		\
				Vector2 bounds = typesetter->measure(	GuiTextMixin::text,																									\
														GuiObject::convertFontSize(GuiTextMixin::fontSize),																	\
														GuiTextMixin::textWrap ? getRect2D().wh() : Vector2::zero(),														\
														&result);																											\
				return result && (bounds.x < getRect2D().width());																											\
			}																																								\
		}																																									\
	return false;																																							\
}																																											\
                            																																				\
int Class::getPersistentDataCost() const																																	\
{																																											\
	return Super::getPersistentDataCost() + Instance::computeStringCost(getText());																							\
}																																											\

}  // namespace RBX
