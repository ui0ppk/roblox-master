#include "stdafx.h"

#include "V8DataModel/TextService.h"

FASTFLAGVARIABLE(TypesettersReleaseResources, true);
FASTFLAGVARIABLE(UseDynamicTypesetterUTF8, false)

namespace RBX
{
const char* const sTextService = "TextService";

namespace Reflection {
template<>
EnumDesc<TextService::FontSize>::EnumDesc()
:EnumDescriptor("FontSize")
{
	addPair(TextService::SIZE_8 , "Size8");
	addPair(TextService::SIZE_9 , "Size9");
	addPair(TextService::SIZE_10, "Size10");
	addPair(TextService::SIZE_11, "Size11");
	addPair(TextService::SIZE_12, "Size12");
	addPair(TextService::SIZE_14, "Size14");
	addPair(TextService::SIZE_18, "Size18");
	addPair(TextService::SIZE_24, "Size24");
	addPair(TextService::SIZE_36, "Size36");
	addPair(TextService::SIZE_48, "Size48");
	addPair(TextService::SIZE_28, "Size28");
	addPair(TextService::SIZE_32, "Size32");
	addPair(TextService::SIZE_42, "Size42");
	addPair(TextService::SIZE_60, "Size60");
	addPair(TextService::SIZE_96, "Size96");
}


template<>
EnumDesc<TextService::Font>::EnumDesc()
:EnumDescriptor("Font")
{
	addPair(TextService::FONT_LEGACY, "Legacy");
	addPair(TextService::FONT_ARIAL, "Arial");
	addPair(TextService::FONT_ARIALBOLD, "ArialBold");
	addPair(TextService::FONT_SOURCESANS, "SourceSans");
	addPair(TextService::FONT_SOURCESANSBOLD, "SourceSansBold");
	addPair(TextService::FONT_SOURCESANSLIGHT, "SourceSansLight");
	addPair(TextService::FONT_SOURCESANSITALIC, "SourceSansItalic");
}

template<>
TextService::Font& Variant::convert<TextService::Font>(void)
{
	return genericConvert<TextService::Font>();
}

template<>
EnumDesc<TextService::XAlignment>::EnumDesc()
:EnumDescriptor("TextXAlignment")
{
	addPair(TextService::XALIGNMENT_LEFT,   "Left");
	addPair(TextService::XALIGNMENT_CENTER, "Center");
	addPair(TextService::XALIGNMENT_RIGHT,  "Right");
}

template<>
EnumDesc<TextService::YAlignment>::EnumDesc()
:EnumDescriptor("TextYAlignment")
{
	addPair(TextService::YALIGNMENT_TOP,	"Top");
	addPair(TextService::YALIGNMENT_CENTER,"Center");
	addPair(TextService::YALIGNMENT_BOTTOM,"Bottom");
}
}//namespace Reflection

template<>
bool StringConverter<TextService::Font>::convertToValue(const std::string& text, TextService::Font& value)
{
	return Reflection::EnumDesc<TextService::Font>::singleton().convertToValue(text.c_str(),value);
}

static Reflection::BoundFuncDesc<TextService, Vector2(std::string, int, TextService::Font, Vector2)> func_getTextSize(&TextService::getTextSize, "GetTextSize", "string", "fontSize", "font", "frameSize", Security::RobloxScript);

TextService::Font TextService::FromTextFont(Text::Font font)
{
	switch(font)
	{
	case Text::FONT_LEGACY:		return FONT_LEGACY;
	case Text::FONT_ARIAL:		return FONT_ARIAL;
	case Text::FONT_ARIALBOLD:	return FONT_ARIALBOLD;
	case Text::FONT_SOURCESANS:		return FONT_SOURCESANS;
	case Text::FONT_SOURCESANSBOLD:	return FONT_SOURCESANSBOLD;
	case Text::FONT_SOURCESANSLIGHT:	return FONT_SOURCESANSLIGHT;
	case Text::FONT_SOURCESANSITALIC:	return FONT_SOURCESANSITALIC;
	default:
		RBXASSERT(0);
		return FONT_LEGACY;
	}
}
Text::Font TextService::ToTextFont(Font font)
{
	switch(font)
	{
	case FONT_LEGACY:		return Text::FONT_LEGACY;
	case FONT_ARIAL:		return Text::FONT_ARIAL;
	case FONT_ARIALBOLD:	return Text::FONT_ARIALBOLD;
	case FONT_SOURCESANS:		return Text::FONT_SOURCESANS;
	case FONT_SOURCESANSBOLD:	return Text::FONT_SOURCESANSBOLD;
	case FONT_SOURCESANSLIGHT:	return Text::FONT_SOURCESANSLIGHT;
	case FONT_SOURCESANSITALIC:	return Text::FONT_SOURCESANSITALIC;
	default:
		RBXASSERT(0);
		return Text::FONT_LEGACY;
	}
}

Text::XAlign TextService::ToTextXAlign(XAlignment xalign)
{

	switch(xalign){
		case TextService::XALIGNMENT_LEFT:
			return Text::XALIGN_LEFT;		
		case TextService::XALIGNMENT_RIGHT:
			return Text::XALIGN_RIGHT;	
		case TextService::XALIGNMENT_CENTER:
			return Text::XALIGN_CENTER;	
		default:
			RBXASSERT(0);
			return Text::XALIGN_LEFT;
	}
}
Text::YAlign TextService::ToTextYAlign(YAlignment yalign)
{
	switch(yalign){
		case TextService::YALIGNMENT_TOP:
			return Text::YALIGN_TOP;
		case TextService::YALIGNMENT_CENTER:
			return Text::YALIGN_CENTER;	
		case TextService::YALIGNMENT_BOTTOM:
			return Text::YALIGN_BOTTOM;	
		default:
			RBXASSERT(0);
			return Text::YALIGN_TOP;
	}
}

TextService::TextService()
	:Super()
{
	this->setName(sTextService);

	clearTypesetters();
}

void TextService::clearTypesetters()
{
    if (FFlag::TypesettersReleaseResources)
    {
        if (m_typesetters.get())
        {
            for (size_t i = 0; i < FONT_LAST; ++i)
            {
                m_typesetters[i].get()->releaseResources();
            }
        }
        else
        {
            m_typesetters.reset(new shared_ptr<Typesetter>[FONT_LAST]);
        }
    }
    else
    {
        m_typesetters.reset(new shared_ptr<Typesetter>[FONT_LAST]);
    }
}
void TextService::registerTypesetter(Font font, shared_ptr<RBX::Typesetter> typesetter)
{
	RBXASSERT(font < FONT_LAST);
	m_typesetters[font] = typesetter;
}

Typesetter* TextService::getTypesetter(Font font)
{
	RBXASSERT(font < FONT_LAST);
	return m_typesetters[font].get();
}

Vector2 TextService::getTextSize(std::string text, int fontSize, Font font, Vector2 frameSize)
{
	if (font >= FONT_LAST || font < FONT_LEGACY)
	{
		return Vector2::zero();
	}

	if (Typesetter* typesetter = getTypesetter(font))
	{
		return typesetter->measure(text, (float) fontSize, frameSize);
	}

	return Vector2::zero();
}

}
