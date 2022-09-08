#pragma once

#include "Util/TextureId.h"
#include "Util/MeshId.h"
#include "CharacterAppearance.h"

namespace RBX {

	class Humanoid;

	extern const char *const sCharacterMesh;
	class CharacterMesh
		 : public DescribedCreatable<CharacterMesh, CharacterAppearance, sCharacterMesh>
	{
	public:
		enum BodyPart
		{
			HEAD = 0,
			TORSO = 1,
			LEFTARM = 2,
			RIGHTARM = 3,
			LEFTLEG = 4,
			RIGHTLEG = 5
		};

		CharacterMesh();

		
		BodyPart getBodyPart() const { return bodyPart; }
		void setBodyPart(BodyPart value);

		
		int baseTextureAssetId;
		int overlayTextureAssetId;
		int meshAssetId;

		static Reflection::BoundProp<int> prop_baseTextureAssetId;
		static Reflection::BoundProp<int> prop_overlayTextureAssetId;
		static Reflection::BoundProp<int> prop_meshAssetId;

		TextureId getBaseTextureId() const;
		TextureId getOverlayTextureId() const;
		MeshId getMeshId() const;
	protected:
		virtual void applyByMyself(Humanoid* humanoid);
		/*override*/ void onPropertyChanged(const Reflection::PropertyDescriptor& descriptor);
	private:
		
		BodyPart bodyPart;
	};


} // namespace
