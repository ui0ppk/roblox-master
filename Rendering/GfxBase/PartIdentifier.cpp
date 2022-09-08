#include "GfxBase/PartIdentifier.h"

#include "Humanoid/Humanoid.h"

#include "v8datamodel/DataModelMesh.h"
#include "v8datamodel/FileMesh.h"
#include "v8datamodel/Decal.h"
#include "v8datamodel/Workspace.h"
#include "v8datamodel/PartInstance.h"
#include "v8datamodel/CharacterAppearance.h"
#include "v8datamodel/CharacterMesh.h"
#include "v8datamodel/Accoutrement.h"

#include "v8datamodel/PartCookie.h"

namespace RBX
{

    static const Vector3 humanoidPartScales[HumanoidIdentifier::PartType_Count] =
    {
        Vector3(1,1,1),  // PartType_Head
        Vector3(2,2,1),  // PartType_Torso
        Vector3(1,2,1),  // PartType_Arm
        Vector3(1,2,1),  // PartType_Leg
        Vector3(1,1,1)   // PartType_Unknown
    };

	// if the part is a humanoid, get further details with this.
	HumanoidIdentifier::HumanoidIdentifier(RBX::Humanoid* humanoid)
		: humanoid(humanoid)
		, head(0)
		, leftLeg(0)
		, rightLeg(0)
		, leftArm(0)
		, rightArm(0)
		, torso(0)
		, leftLegMesh(0)
		, rightLegMesh(0)
		, leftArmMesh(0)
		, rightArmMesh(0)
		, torsoMesh(0)
	{
		if (!humanoid)
			return;

		Instance* parent = humanoid->getParent();
		if (!parent)
			return;
				
		const Instances& children = *parent->getChildren();
				
		for (size_t i = 0; i < children.size(); ++i)
		{
			Instance* inst = children[i].get();
				
			if (PartInstance* part = Instance::fastDynamicCast<PartInstance>(inst))
			{
				const std::string& name = part->getName();
					
				if (name == "Head")
					head = part;
				else if (name == "Left Leg")
					leftLeg = part;
				else if (name == "Right Leg")
					rightLeg = part;
				else if (name == "Left Arm")
					leftArm = part;
				else if (name == "Right Arm")
					rightArm = part;
				else if (name == "Torso")
					torso = part;
			}
			else if (Clothing* c = Instance::fastDynamicCast<Clothing>(inst))
			{
				if (pants.isNull() && !c->outfit1.isNull())
					pants = c->outfit1;
				if (shirt.isNull() && !c->outfit2.isNull())
					shirt = c->outfit2;
			}
			else if (ShirtGraphic* s = Instance::fastDynamicCast<ShirtGraphic>(inst))
			{
				if (shirtGraphic.isNull() && !s->graphic.isNull())
					shirtGraphic = s->graphic;
			}
			else if (CharacterMesh* m = Instance::fastDynamicCast<CharacterMesh>(inst))
			{
				switch (m->getBodyPart())
				{
				case CharacterMesh::LEFTARM:
					leftArmMesh = m; break;
				case CharacterMesh::RIGHTARM:
					rightArmMesh = m; break;
				case CharacterMesh::LEFTLEG:
					leftLegMesh = m; break;
				case CharacterMesh::RIGHTLEG:
					rightLegMesh = m; break;
				case CharacterMesh::TORSO:
					torsoMesh = m; break;
				default:
					RBXASSERT(!"Unsupported body part type");
					break;
				}
			}
			else if (Accoutrement* a = Instance::fastDynamicCast<Accoutrement>(inst))
			{
				accoutrements.push_back(a);
			}
		}
	}

	CharacterMesh* HumanoidIdentifier::getRelevantMesh(RBX::PartInstance* bodyPart) const
	{
		if(bodyPart==leftLeg) return leftLegMesh;
		if(bodyPart==rightLeg) return rightLegMesh;
		if(bodyPart==leftArm) return leftArmMesh;
		if(bodyPart==rightArm) return rightArmMesh;
		if(bodyPart==torso) return torsoMesh;
		return NULL;
	}

    HumanoidIdentifier::BodyPartType HumanoidIdentifier::getBodyPartType(RBX::PartInstance* bodyPart) const
    {
        if(bodyPart==leftLeg || bodyPart==rightLeg) return PartType_Leg;
        if(bodyPart==leftArm || bodyPart==rightArm) return PartType_Arm;
        if(bodyPart==torso) return PartType_Torso;
        if(bodyPart==head) return PartType_Head;
        
        return PartType_Unknown;
    }

    Vector3 HumanoidIdentifier::getBodyPartScale(RBX::PartInstance* bodyPart) const
    {
        return humanoidPartScales[getBodyPartType(bodyPart)];
    }

	bool HumanoidIdentifier::isPartHead(RBX::PartInstance* part) const
	{
		if (part != head)
			return false;

		if (RBX::DataModelMesh* specialShape = RBX::getSpecialShape(part))
		{
			bool hasFace = (part->getCookie() & PartCookie::HAS_DECALS) != 0;

			if (RBX::SpecialShape* shape = specialShape->fastDynamicCast<RBX::SpecialShape>())
			{
				// A real head or a file mesh - treat it as a head even if there is no face (might use a mesh texture)
				if (shape->getMeshType() == RBX::SpecialShape::HEAD_MESH || shape->getMeshType() == RBX::SpecialShape::FILE_MESH)
					return true;

				// Probably one of the heads from the store - all character heads have faces
				if (shape->getMeshType() == RBX::SpecialShape::SPHERE_MESH)
					return hasFace;

				// Unrecognized shape type - this is not a head from the store, so don't treat it as a head.
				return false;
			}
			else if (specialShape->fastDynamicCast<RBX::FileMesh>())
			{
				// A file mesh - treat it as a head even if there is no face (might use a mesh texture)
				return true;
			}
			else
			{
				// Probably one of the heads from the store - all character heads have faces
				return hasFace;
			}
		}

		// No special shape - should be a simple part
		return false;
	}

	bool HumanoidIdentifier::isBodyPart(RBX::PartInstance* part) const
	{
		if (part->getCookie() & PartCookie::IS_HUMANOID_PART)
			return (leftArm == part || leftLeg == part || rightArm == part || rightLeg == part || torso == part || head == part);
		else
			return false;
	}

	bool HumanoidIdentifier::isBodyPartComposited(RBX::PartInstance* part) const
	{
		bool noReflectance = part->getReflectance() <= 0.015f;

		// Heads are always composited as long as they are not transparent
		if (part == head)
			return part->getTransparencyUi() <= 0 && isPartHead(part) && noReflectance;

		// Body parts with special shapes are never composited to match the old behavior
		if (part->getCookie() & PartCookie::HAS_SPECIALSHAPE)
			return false;

		// Non-block parts are never composited to match the old behavior
		if (part->getPartType() != BLOCK_PART)
			return false;

		// Parts with meshes are always composited
		if (getRelevantMesh(part))
			return true;

		// Arms are always composited if there is a shirt
		if ((part == leftArm || part == rightArm) && !shirt.isNull())
			return true;

		// Legs are always composited if there are pants
		if ((part == leftLeg || part == rightLeg) && !pants.isNull())
			return true;

		// Torso is always composited if there is a shirt, pants or a t-shirt
		if (part == torso && (!pants.isNull() || !shirt.isNull() || !shirtGraphic.isNull()))
			return true;

        // Now we have a body part and we have a choice - we can composit it or skip compositing.
        // Compositing means that we lose materials; we also replace the body part with a prebaked mesh, so we lose the size information.
        // We also lose studs, but we care way less about those - so to improve batching, we'll composit plastic parts with expected size.
        return (part->getRenderMaterial() == PLASTIC_MATERIAL || part->getRenderMaterial() == SMOOTH_PLASTIC_MATERIAL) && noReflectance;
	}

	bool HumanoidIdentifier::isPartComposited(RBX::PartInstance* part) const
	{
		if (isBodyPart(part))
			return isBodyPartComposited(part);

		if (Instance::isA<Accoutrement>(part->getParent()))
			return true;

		return false;
	}
}
