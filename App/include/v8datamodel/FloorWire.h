
#pragma once

#include "reflection/reflection.h"
#include "util/TextureId.h"
#include "V8DataModel/GuiBase3d.h"

namespace RBX {

	class ContactManager;
	class PartInstance;
	class Workspace;

	extern const char* const sFloorWire;
	class FloorWire 
		: public DescribedCreatable<FloorWire, GuiBase3d, sFloorWire> {
	private:
		typedef Reflection::RefPropDescriptor<FloorWire, PartInstance> PartProp;
		static const float kMinDistanceFromBlocks;
		static const int kMaxSegments;

		static PartProp prop_From;
		static PartProp prop_To;
		static Reflection::PropDescriptor<FloorWire, TextureId> prop_Texture;
		static Reflection::PropDescriptor<FloorWire, Vector2> prop_TextureSize;
		static Reflection::PropDescriptor<FloorWire, float> prop_Velocity;
		static Reflection::PropDescriptor<FloorWire, float> prop_StudsBetweenTextures;
		static Reflection::PropDescriptor<FloorWire, float> prop_CycleOffset;
		static Reflection::PropDescriptor<FloorWire, float> prop_WireRadius;

	public:
		FloorWire();

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
		void setWireRadius(float wireRadius);
		float getWireRadius() const;
		
		// IAdornable
		/*override*/ void render3dAdorn(Adorn* adorn);
	protected:
		void setPartInstance(weak_ptr<PartInstance>& data,
				PartInstance* newValue, const PartProp& prop);
		static void computeSurfacePosition(const shared_ptr<PartInstance>& part,
				const RbxRay& ray, Vector3* out);
		bool incrementalBuildSegments(const Workspace* workspace,
				const ContactManager* contactManager, const Vector3& dest,
				bool moveInX, std::vector<Vector3>* out);
		void buildTrailSegments(const Workspace* workspace,
				const shared_ptr<PartInstance>& from,
				const shared_ptr<PartInstance>& to,
				std::vector<Vector3>* out);
		void drawSegments(const Workspace* workspace, const Camera* camera,
				const std::vector<Vector3>& segments, Adorn* adorn);

		weak_ptr<PartInstance> from;
		weak_ptr<PartInstance> to;
		TextureId texture;
		Vector2 textureSize;
		float velocity;
		float studsBetweenTextures;
		float cycleOffset;
		float wireRadius;
	};
}