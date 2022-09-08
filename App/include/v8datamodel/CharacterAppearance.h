#pragma once

#include "V8DataModel/GlobalSettings.h"
#include "Util/BrickColor.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/IModelModifier.h"
#include "Util/TextureId.h"

namespace RBX {

	class Humanoid;

	extern const char* const sCharacterAppearance;
	class RBXBaseClass CharacterAppearance
		:public DescribedNonCreatable<CharacterAppearance, Instance, sCharacterAppearance>
		,public IModelModifier
	{
	private:
		typedef Instance Super;

	public:
		virtual void apply();
	protected:
		virtual void onAncestorChanged(const AncestorChanged& event);
		virtual bool askSetParent(const Instance* instance) const;
	private:
		virtual void applyByMyself(Humanoid* humanoid) = 0;
	};

	class LegacyCharacterAppearance
		: public CharacterAppearance
	{
	private:
		typedef CharacterAppearance Super;

	public:
		
		// Hack: This apply() function is ugly, but necessary because
		//       this class changes properties of other objects (like
		//       Part colors and Decal images).  However, to avoid
		//       crosstalk the apply function should only do something
		//       in the backend case.
		/*override*/ void apply();
	};

	// Old-style T-shirts
	extern const char *const sShirtGraphic;
	class ShirtGraphic
		 : public DescribedCreatable<ShirtGraphic, LegacyCharacterAppearance, sShirtGraphic>
	{
	public:
		TextureId graphic;
		static Reflection::BoundProp<TextureId> prop_Graphic;
		ShirtGraphic();
	protected:
		/*override*/ void applyByMyself(Humanoid* humanoid);
	private:
		void dataChanged(const Reflection::PropertyDescriptor&) {
			CharacterAppearance::apply();
		}
	};

	extern const char *const sClothing;
	class Clothing
		 : public DescribedNonCreatable<Clothing, CharacterAppearance, sClothing>
	{
		friend class CharacterAppearance;
	public:
		TextureId outfit1;
		TextureId outfit2;

		static Reflection::BoundProp<TextureId> prop_outfit1;
		static Reflection::BoundProp<TextureId> prop_outfit2;

		Clothing();

		virtual TextureId getTemplate() const { RBXASSERT(false); return NULL; }

	protected:
		/*override*/ void applyByMyself(Humanoid* humanoid);
		void dataChanged(const Reflection::PropertyDescriptor&) {
			CharacterAppearance::apply();
		}
	};

	extern const char *const sPants;
	class Pants
		 : public DescribedCreatable<Pants, Clothing, sPants>
	{
	public:
		Pants();
		static Reflection::PropDescriptor<Pants, TextureId> prop_PantsTemplate;
		TextureId getTemplate() const { return outfit1; }
		void setTemplate(TextureId value);
	};

	extern const char *const sShirt;
	class Shirt
		 : public DescribedCreatable<Shirt, Clothing, sShirt>
	{
	public:
		Shirt();
		static Reflection::PropDescriptor<Shirt, TextureId> prop_ShirtTemplate;
		TextureId getTemplate() const { return outfit2; }
		void setTemplate(TextureId value);
	};

	extern const char *const sBodyColors;
	class BodyColors 
		 : public DescribedCreatable<BodyColors, LegacyCharacterAppearance, sBodyColors>
	{
		BrickColor headColor;
		BrickColor leftArmColor;
		BrickColor rightArmColor;
		BrickColor torsoColor;
		BrickColor leftLegColor;
		BrickColor rightLegColor;
	public:
		static Reflection::BoundProp<BrickColor> prop_HeadColor;
		static Reflection::BoundProp<BrickColor> prop_LeftArmColor;
		static Reflection::BoundProp<BrickColor> prop_RightArmColor;
		static Reflection::BoundProp<BrickColor> prop_TorsoColor;
		static Reflection::BoundProp<BrickColor> prop_LeftLegColor;
		static Reflection::BoundProp<BrickColor> prop_RightLegColor;
		BodyColors();
	private:
		virtual void applyByMyself(Humanoid* humanoid);
		void dataChanged(const Reflection::PropertyDescriptor&) {
			CharacterAppearance::apply();
		}
	};

	extern const char *const sSkin;
	class Skin 
		 : public DescribedCreatable<Skin, LegacyCharacterAppearance, sSkin>
	{
		BrickColor skinColor;
	public:
		static Reflection::BoundProp<BrickColor> prop_skinColor;
		Skin();
	private:
		virtual void applyByMyself(Humanoid* humanoid);
		void dataChanged(const Reflection::PropertyDescriptor&) {
			CharacterAppearance::apply();
		}
	};

} // namespace
