#pragma once

#include "V8dataModel/FaceInstance.h"
#include "V8Tree/instance.h"
#include "Util/TextureId.h"

namespace RBX {

	extern const char* const sDecal;
	class Decal : public DescribedCreatable<Decal, FaceInstance, sDecal>
	{
		TextureId texture;
		float specular;
		float shiny;
		float transparency;
		float localTransparencyModifier;

	public:
		Decal(void);

		static const Reflection::PropDescriptor<Decal, TextureId> prop_Texture;
		const TextureId& getTexture() const { return texture; }
		void setTexture(TextureId value);

		static const Reflection::PropDescriptor<Decal, float> prop_Specular;
		float getSpecular() const { return specular; }
		void setSpecular(float value);

		static const Reflection::PropDescriptor<Decal, float> prop_Shiny;
		float getShiny() const { return shiny; }
		void setShiny(float value);

		static const Reflection::PropDescriptor<Decal, float> prop_Transparency;
		float getTransparencyUi() const;
		float getTransparency() const { return transparency; }
		void setTransparency(float value);

		static const Reflection::PropDescriptor<Decal, float> prop_LocalTransparencyModifier;
		float getLocalTransparencyModifier() const { return localTransparencyModifier; }
		void setLocalTransparencyModifier(float value);

	};

	extern const char* const sDecalTexture;
	class DecalTexture : public DescribedCreatable<DecalTexture, Decal, sDecalTexture>
	{
		G3D::Vector2 studsPerTile;
	public:
		DecalTexture(void);

		const G3D::Vector2& getStudsPerTile() {
			return studsPerTile;
		}

		static const Reflection::PropDescriptor<DecalTexture, float> prop_StudsPerTileU;
		float getStudsPerTileU() const { return studsPerTile.x; }
		void setStudsPerTileU(float value);

		static const Reflection::PropDescriptor<DecalTexture, float> prop_StudsPerTileV;
		float getStudsPerTileV() const { return studsPerTile.y; }
		void setStudsPerTileV(float value);
	};

}
