#pragma once

#include "V8Tree/Service.h"
#include "V8Tree/Instance.h"
#include "GfxBase/Typesetter.h"

namespace RBX {
	class PartInstance;

	extern const char *const sTextService ;

	class TextService 
		: public DescribedNonCreatable<TextService, Instance, sTextService, Reflection::ClassDescriptor::INTERNAL_LOCAL>
		, public Service
	{
	private:
		typedef DescribedNonCreatable<TextService, Instance, sTextService, Reflection::ClassDescriptor::INTERNAL_LOCAL> Super;
		boost::scoped_array<shared_ptr<Typesetter> > m_typesetters;
	public:
		enum XAlignment
		{
			XALIGNMENT_LEFT   = 0,
			XALIGNMENT_RIGHT  = 1,
			XALIGNMENT_CENTER = 2
		};

		enum YAlignment
		{
			YALIGNMENT_TOP    = 0,
			YALIGNMENT_CENTER = 1,
			YALIGNMENT_BOTTOM = 2
		};

		enum FontSize
		{
			SIZE_8 = 0,
			SIZE_9 = 1,
			SIZE_10 = 2,
			SIZE_11 = 3,
			SIZE_12 = 4,
			SIZE_14 = 5,
			SIZE_18 = 6,
			SIZE_24 = 7,
			SIZE_36 = 8, 
			SIZE_48 = 9,

			SIZE_28 = 10,
			SIZE_32 = 11,
			SIZE_42 = 12,
			SIZE_60 = 13,
			SIZE_96 = 14,

			// ADD NEW FONT SIZES ABOVE HERE, AND UPDATE BELOW


			SIZE_SMALLEST = SIZE_8,
			SIZE_LARGEST = SIZE_96
		};

		enum Font
		{
			FONT_LEGACY = 0,
			FONT_ARIAL = 1,
			FONT_ARIALBOLD = 2,
			FONT_SOURCESANS = 3,
			FONT_SOURCESANSBOLD = 4,
			FONT_SOURCESANSLIGHT = 5,
			FONT_SOURCESANSITALIC = 6,

			FONT_LAST = 7
		};

		static Font FromTextFont(Text::Font font);


		static Text::Font ToTextFont(Font font);
		static Text::XAlign ToTextXAlign(XAlignment xalign);
		static Text::YAlign ToTextYAlign(YAlignment xalign);
		TextService();


		void registerTypesetter(Font font, shared_ptr<RBX::Typesetter> typesetter);
		void clearTypesetters();

		Vector2 getTextSize(std::string text, int fontSize, Font font, Vector2 frameSize);

		Typesetter* getTypesetter(Font font);
	};
}