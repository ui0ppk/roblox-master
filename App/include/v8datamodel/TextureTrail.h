
#pragma once

#include "reflection/reflection.h"
#include "util/TextureId.h"
#include "V8DataModel/GuiBase3d.h"

namespace RBX {

	class Camera;
	class PartInstance;
	class Workspace;

	extern const char* const sTextureTrail;
	class TextureTrail 
		: public DescribedCreatable<TextureTrail, GuiBase3d, sTextureTrail> {
	private:
		typedef Reflection::RefPropDescriptor<TextureTrail, PartInstance> PartProp;

		static PartProp prop_From;
		static PartProp prop_To;
		static Reflection::PropDescriptor<TextureTrail, TextureId> prop_Texture;
		static Reflection::PropDescriptor<TextureTrail, Vector2> prop_TextureSize;
		static Reflection::PropDescriptor<TextureTrail, float> prop_Velocity;
		static Reflection::PropDescriptor<TextureTrail, float> prop_StudsBetweenTextures;
		static Reflection::PropDescriptor<TextureTrail, float> prop_CycleOffset;

	public:
		TextureTrail();

		void setFrom(PartInstance* value);
		PartInstance* getFrom() const;
		void setTo(PartInstance* value);
		PartInstance* getTo() const;
		void setTexture(TextureId value);
		TextureId getTexture() const;
		void setTextureSize(Vector2 value);
		Vector2 getTextureSize() const;
		void setVelocity(float velocity);
		float getVelocity() const;
		void setStudsBetweenTextures(float spacing);
		float getStudsBetweenTextures() const;
		void setCycleOffset(float cycleOffset);
		float getCycleOffset() const;

		// IAdornable
		/*override*/ void render3dAdorn(Adorn* adorn);

		// For floor wire
		static void renderInternal(
				const Workspace* workspace, const Camera* camera,
				const Vector3& fromPosition, const Vector3& toPosition,
				const TextureId& texture, std::string context, const Vector2& textureSize,
				float velocity, float studsBetweenTextures, float cycleOffset,
				const Color4& color, Adorn* adorn);
	protected:
		void setPartInstance(weak_ptr<PartInstance>& data,
				PartInstance* newValue, const PartProp& prop);
		static bool getPosition(const weak_ptr<PartInstance>& dataSource,
				Vector3* out);

		weak_ptr<PartInstance> from;
		weak_ptr<PartInstance> to;
		TextureId texture;
		Vector2 textureSize;
		float velocity;
		float studsBetweenTextures;
		float cycleOffset;
	};
}