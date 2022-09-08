#pragma once

#include "Util/TextureId.h"
#include "Util/G3DCore.h"

namespace RBX
{
	class PartInstance;
	class Humanoid;
	class CharacterMesh;
	class Accoutrement;

		// if the part is a humanoid, get further details with this.
		class HumanoidIdentifier
		{
		public:
			explicit HumanoidIdentifier(RBX::Humanoid* humanoid);

			Humanoid* humanoid;

			PartInstance* head;
			PartInstance* leftLeg;
			PartInstance* rightLeg;
			PartInstance* leftArm;
			PartInstance* rightArm;
			PartInstance* torso;
			
			std::vector<Accoutrement*> accoutrements;
			
			TextureId pants;
			TextureId shirt;
			TextureId shirtGraphic;

			CharacterMesh* leftLegMesh;
			CharacterMesh* rightLegMesh;
			CharacterMesh* leftArmMesh;
			CharacterMesh* rightArmMesh;
			CharacterMesh* torsoMesh;

			bool isBodyPart(RBX::PartInstance* part) const;
			bool isBodyPartComposited(RBX::PartInstance* part) const;
			bool isPartComposited(RBX::PartInstance* part) const;
			bool isPartHead(RBX::PartInstance* part) const;

			// helper
			CharacterMesh* getRelevantMesh(RBX::PartInstance* bodyPart) const;

            enum BodyPartType
            {
                PartType_Head,
                PartType_Torso,
                PartType_Arm,
                PartType_Leg,
                PartType_Unknown,
                PartType_Count
            };

            BodyPartType getBodyPartType(RBX::PartInstance* bodyPart) const;
            Vector3 getBodyPartScale(RBX::PartInstance* bodyPart) const;
		};
}
