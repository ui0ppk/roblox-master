#include "stdafx.h"
#include "MaterialGenerator.h"

#include "GfxBase/PartIdentifier.h"
#include "GfxBase/GfxPart.h"

#include "TextureCompositor.h"
#include "TextureManager.h"
#include "VisualEngine.h"

#include "v8datamodel/PartInstance.h"
#include "v8datamodel/CharacterMesh.h"
#include "v8datamodel/Decal.h"
#include "v8datamodel/SpecialMesh.h"
#include "v8datamodel/ContentProvider.h"
#include "v8datamodel/Accoutrement.h"
#include "v8datamodel/BlockMesh.h"
#include "v8datamodel/CylinderMesh.h"
#include "v8datamodel/PartCookie.h"


#include "Material.h"
#include "ShaderManager.h"
#include "TextureManager.h"
#include "LightGrid.h"
#include "util/SafeToLower.h"

#include "GfxCore/Device.h"
#include "SceneManager.h"
#include "EnvMap.h"

#include "RenderQueue.h"

FASTFLAGVARIABLE(RenderMaterialsOnMobile, true)
FASTFLAGVARIABLE(ForceWangTiles, false)
FASTFLAG(GlowEnabled)

namespace RBX
{
namespace Graphics
{

// here's how our texture compositing setup works:
// there is a set of body meshes that covers a canvas area of 1024x512
static const int kTextureCompositWidth = 1024;
static const int kTextureCompositHeight = 512;

// we'd like a 256x512 strip on the side, which contains 1 256x256 slot and 4 128x128 slots
// this strip should make the base area less wide - therefore we make the canvas wider
// note that 256 pixels in texture space is more than that in canvas space
// X pixels in canvas space translate to X / (1024 + X) * 1024 in texture space, so X can be computed using the formula below
static const float kTextureCompositCanvasExtraSpace = 1024.f * 256.f / (1024.f - 256.f);
static const float kTextureCompositCanvasWidth = 1024.f + kTextureCompositCanvasExtraSpace;
static const float kTextureCompositCanvasHeight = 512.f;

// given a base texture size of 1024, we have to rescale the UVs to fit
static const float kTextureCompositBaseWidth = 1024.f / kTextureCompositCanvasWidth;
static const float kTextureCompositExtraWidth = kTextureCompositCanvasExtraSpace / kTextureCompositCanvasWidth;

// slot configuration in UV space; note that the arrangment is like this:
// 34
// 12
// 00
// 00
// with each digit corresponding to a 128x128 area in a 256x512 canvas
// Vector4 xy is UV offset, zw is UV scale; borders are not included in this table
static const G3D::Vector4 kTextureCompositSlotConfiguration[] =
{
    G3D::Vector4(kTextureCompositBaseWidth + 0.0f * kTextureCompositExtraWidth, 0.50f, 1.0f * kTextureCompositExtraWidth, 0.50f),
    G3D::Vector4(kTextureCompositBaseWidth + 0.0f * kTextureCompositExtraWidth, 0.25f, 0.5f * kTextureCompositExtraWidth, 0.25f),
    G3D::Vector4(kTextureCompositBaseWidth + 0.5f * kTextureCompositExtraWidth, 0.25f, 0.5f * kTextureCompositExtraWidth, 0.25f),
    G3D::Vector4(kTextureCompositBaseWidth + 0.0f * kTextureCompositExtraWidth, 0.00f, 0.5f * kTextureCompositExtraWidth, 0.25f),
    G3D::Vector4(kTextureCompositBaseWidth + 0.5f * kTextureCompositExtraWidth, 0.00f, 0.5f * kTextureCompositExtraWidth, 0.25f),
};

// one of the slots is used by the head, all other slots are used by accoutrements
static const size_t kTextureCompositAccoutrementCount = ARRAYSIZE(kTextureCompositSlotConfiguration) - 1;

// inside each slot we leave a small border of 8x8 pixels to deal with filtering
static const float kTextureCompositExtraBorderWidth = 8.f / kTextureCompositCanvasWidth;
static const float kTextureCompositExtraBorderHeight = 8.f / kTextureCompositCanvasHeight;

// diffuse map is always bound to stage 5
static const unsigned int kDiffuseMapStage = 5;

class TextureCompositingDescription
{
public:
    TextureCompositingDescription()
    {
        name.reserve(1024);
        layers.reserve(16);
        
        nameAppend("TexComp");
    }
    
    void add(const MeshId& mesh, const ContentId& texture, TextureCompositorLayer::CompositMode mode = TextureCompositorLayer::Composit_BlendTexture) 
    { 
        layers.push_back(TextureCompositorLayer(mesh, texture, mode));
        
        nameAppend(" T[");
        nameAppend(mesh.toString());
        nameAppend(":");
        nameAppend(texture.toString());
        nameAppend(":");
        nameAppend(mode);
        nameAppend("]");
    }

    void add(const MeshId& mesh, const BrickColor& color)
    {
        layers.push_back(TextureCompositorLayer(mesh, color.color3()));
        
        nameAppend(" C[");
        nameAppend(mesh.toString());
        nameAppend(":");
        nameAppend(color.asInt());
        nameAppend("]");
    }
    
    const std::string& getName() const
    {
        return name;
    }
    
    const std::vector<TextureCompositorLayer>& getLayers() const
    {
        return layers;
    }
    
private:
    std::string name;
    std::vector<TextureCompositorLayer> layers;
    
    void nameAppend(const char* value)
    {
        name += value;
    }
    
    void nameAppend(const std::string& value)
    {
        name += value;
    }
    
    void nameAppend(int value)
    {
        char buf[32];
        sprintf(buf, "%d", value);
        
        name += buf;
    }
};

struct AccoutrementMesh
{
    PartInstance* part;
    FileMesh* mesh;
    float quality;
};

typedef AccoutrementMesh AccoutrementMeshes[kTextureCompositAccoutrementCount];

static float getAccoutrementQuality(Accoutrement* acc, PartInstance* part)
{
    const Vector3& location = acc->getAttachmentPos();
    const Vector3& extents = part->getPartSizeXml();
    
    // accoutrements are attached to top of the head; Y axis is reversed
    float attachmentTop = -location.y + extents.y / 2;
    float attachmentBottom = -location.y - extents.y / 2;
    
    // top is below the top of the head - not a hat
    // bottom is significantly below the bottom of the head (head is 1 unit high) - probably not a hat
    if (attachmentTop < 0.f || attachmentBottom < -1.75f) return 0.f;
    
    // surface area
    return extents.x * extents.y + extents.x * extents.z + extents.y * extents.z;
}

struct AccoutrementQualityComparator
{
    bool operator()(const AccoutrementMesh& lhs, const AccoutrementMesh& rhs) const
    {
        return lhs.quality < rhs.quality;
    }
};
    
struct AccoutrementMeshIdComparator
{
    bool operator()(const AccoutrementMesh& lhs, const AccoutrementMesh& rhs) const
    {
        return lhs.mesh->getMeshId() < rhs.mesh->getMeshId();
    }
};
    
static void getCompositedAccoutrements(AccoutrementMeshes& result, const HumanoidIdentifier& hi)
{
    size_t count = 0;
    
    for (size_t i = 0; i < hi.accoutrements.size(); ++i)
    {
        if (PartInstance* part = hi.accoutrements[i]->findFirstChildOfType<PartInstance>())
        {
            if (FileMesh* mesh = getFileMesh(part))
            {
                if (count < kTextureCompositAccoutrementCount && !mesh->getTextureId().isNull())
                {
                    result[count].part = part;
                    result[count].mesh = mesh;
                    result[count].quality = getAccoutrementQuality(hi.accoutrements[i], part);
                    count++;
                }
            }
        }
    }
    
    if (count > 1)
    {
        // sort all accoutrements by mesh id to keep order stable (this reduces rebaking)
        std::sort(&result[0], &result[count], AccoutrementMeshIdComparator());
        
        // put the best quality accoutrement to the front to make it use the HQ slot
        AccoutrementMesh* bestQuality = std::max_element(&result[0], &result[count], AccoutrementQualityComparator());
        
        if (bestQuality->quality > 0)
        {
            std::swap(result[0], *bestQuality);
        }
    }
}

static int getExtraSlot(PartInstance* part, const HumanoidIdentifier& hi, const AccoutrementMeshes& accoutrements)
{
    if (part == hi.head)
        return kTextureCompositAccoutrementCount;
        
    for (size_t i = 0; i < kTextureCompositAccoutrementCount; ++i)
        if (accoutrements[i].part == part)
            return i;
    
    return -1;
}

static std::pair<bool, G3D::Vector4> getPartCompositConfiguration(PartInstance* part, const HumanoidIdentifier& hi, const AccoutrementMeshes& accoutrements)
{
    // base part
    if (hi.leftArm == part || hi.leftLeg == part || hi.rightArm == part || hi.rightLeg == part || hi.torso == part)
        return std::make_pair(true, G3D::Vector4(0, 0, kTextureCompositBaseWidth, 1));
    
    // head and accoutrements occupy the rightmost column, with reversed order (bottom to top)
    int slot = getExtraSlot(part, hi, accoutrements);
    
    if (slot >= 0)
    {
        const G3D::Vector4& cfg = kTextureCompositSlotConfiguration[slot];
        G3D::Vector4 borderAdjustment(kTextureCompositExtraBorderWidth, kTextureCompositExtraBorderHeight, -2.f * kTextureCompositExtraBorderWidth, -2.f * kTextureCompositExtraBorderHeight);
        
        return std::make_pair(true, cfg + borderAdjustment);
    }
    
    return std::make_pair(false, G3D::Vector4());
}

static MeshId getExtraSlotMeshId(PartInstance* part, const HumanoidIdentifier& hi, const AccoutrementMeshes& accoutrements)
{
    int slotId = getExtraSlot(part, hi, accoutrements);
    RBXASSERT(slotId >= 0);
    
    return MeshId(format("rbxasset://fonts/CompositExtraSlot%d.mesh", slotId));
}

static void prepareHumanoidTextureCompositing(TextureCompositingDescription& desc, const HumanoidIdentifier& hi, const AccoutrementMeshes& accoutrements, CharacterMesh* mesh)
{
    if (hi.torso)
        desc.add(MeshId("rbxasset://fonts/CompositTorsoBase.mesh"), hi.torso->getColor());
        
    if (hi.leftArm)
        desc.add(MeshId("rbxasset://fonts/CompositLeftArmBase.mesh"), hi.leftArm->getColor());
        
    if (hi.rightArm)
        desc.add(MeshId("rbxasset://fonts/CompositRightArmBase.mesh"), hi.rightArm->getColor());
        
    if (hi.leftLeg)
        desc.add(MeshId("rbxasset://fonts/CompositLeftLegBase.mesh"), hi.leftLeg->getColor());
        
    if (hi.rightLeg)
        desc.add(MeshId("rbxasset://fonts/CompositRightLegBase.mesh"), hi.rightLeg->getColor());
        
    if (hi.head)
    {
        FileMesh* headMesh = getFileMesh(hi.head);
        MeshId slotMeshId = getExtraSlotMeshId(hi.head, hi, accoutrements);
        
        desc.add(slotMeshId, hi.head->getColor());
        
        if (headMesh && !headMesh->getTextureId().isNull())
            desc.add(slotMeshId, headMesh->getTextureId());
        
        if (hi.head->getChildren())
        {
            const Instances& children = *hi.head->getChildren();
            
            for (size_t i = children.size(); i > 0; --i)
                if (Decal* decal = Instance::fastDynamicCast<Decal>(children[i - 1].get()))
                    if (decal->getFace() == NORM_Z_NEG && !decal->getTexture().isNull())
                        desc.add(slotMeshId, decal->getTexture());
        }
    }
    
    if (mesh && !mesh->getBaseTextureId().isNull())
        desc.add(MeshId("rbxasset://fonts/CompositFullAtlasBaseTexture.mesh"), mesh->getBaseTextureId());
        
    if (!hi.pants.isNull())
        desc.add(MeshId("rbxasset://fonts/CompositPantsTemplate.mesh"), hi.pants);
        
    if (!hi.shirt.isNull())
        desc.add(MeshId("rbxasset://fonts/CompositShirtTemplate.mesh"), hi.shirt);
        
    if (!hi.shirtGraphic.isNull())
        desc.add(MeshId("rbxasset://fonts/CompositTShirt.mesh"), hi.shirtGraphic);
        
    if (mesh && !mesh->getOverlayTextureId().isNull())
        desc.add(MeshId("rbxasset://fonts/CompositFullAtlasOverlayTexture.mesh"), mesh->getOverlayTextureId());
        
    for (size_t i = 0; i < kTextureCompositAccoutrementCount; ++i)
    {
        if (accoutrements[i].mesh)
        {
            // Accoutrements do not use alpha blending; instead they use alpha test.
            // This means that instead of alpha blend compositing, we have to use a straight blit (so that color stays in tact).
            // In addition to that, texture compositor texture has 1-bit alpha, so the default alpha cutoff is 128;
            // to work with the existing assets, we have to decrease it - we do it using fixed-function 4x modulation,
            // which effectively changes the cutoff to 32.
            MeshId slotMeshId = getExtraSlotMeshId(accoutrements[i].part, hi, accoutrements);
        
            desc.add(slotMeshId, accoutrements[i].part->getColor());
            desc.add(slotMeshId, accoutrements[i].mesh->getTextureId(), TextureCompositorLayer::Composit_BlitTextureAlphaMagnify4x);
        }
    }
}

static std::pair<TextureRef, TextureCompositor::JobHandle> createHumanoidTextureComposit(VisualEngine* visualEngine,
    const HumanoidIdentifier& hi, const AccoutrementMeshes& accoutrements, CharacterMesh* mesh)
{
    Vector2 canvasSize(kTextureCompositCanvasWidth, kTextureCompositCanvasHeight);

    TextureCompositingDescription desc;
    prepareHumanoidTextureCompositing(desc, hi, accoutrements, mesh);

	TextureCompositor::JobHandle job = visualEngine->getTextureCompositor()->getJob(desc.getName(), hi.humanoid->getFullName() + " Clothes", kTextureCompositWidth, kTextureCompositHeight, canvasSize, desc.getLayers());
    TextureRef texture = visualEngine->getTextureCompositor()->getTexture(job);

    return std::make_pair(texture, job);
}

static std::pair<TextureRef, TextureCompositor::JobHandle> createHumanoidTextureCached(VisualEngine* visualEngine,
    const HumanoidIdentifier& hi, const AccoutrementMeshes& accoutrements, CharacterMesh* mesh,
    std::pair<Humanoid*, TextureCompositor::JobHandle>& compositCache)
{
    if (hi.humanoid == compositCache.first)
    {
        TextureRef texture = visualEngine->getTextureCompositor()->getTexture(compositCache.second);

        return std::make_pair(texture, compositCache.second);
    }
        
    std::pair<TextureRef, TextureCompositor::JobHandle> result = createHumanoidTextureComposit(visualEngine, hi, accoutrements, mesh);
    
    compositCache = std::make_pair(hi.humanoid, result.second);
    
    return result;
}

static std::pair<std::pair<TextureRef, TextureCompositor::JobHandle>, G3D::Vector4> createHumanoidTexture(VisualEngine* visualEngine,
    PartInstance* part, const HumanoidIdentifier& hi, unsigned int flags,
    std::pair<Humanoid*, TextureCompositor::JobHandle>& compositCache)
{
    // we only composit up to a certain number of mesh accoutrements
    AccoutrementMeshes accoutrements = {};

    if (flags & MaterialGenerator::Flag_UseCompositTextureForAccoutrements)
        getCompositedAccoutrements(accoutrements, hi);
    
    std::pair<bool, G3D::Vector4> cfg = getPartCompositConfiguration(part, hi, accoutrements);
    
    if (!cfg.first)
        return std::make_pair(std::make_pair(TextureRef(), TextureCompositor::JobHandle()), G3D::Vector4());
        
    // get mesh that's used as base/overlay texture source
    // heads/accoutrements use torso mesh to share the composit texture
    CharacterMesh* mesh =
        (part == hi.torso || part == hi.leftArm || part == hi.rightArm || part == hi.leftLeg || part == hi.rightLeg)
        ? hi.getRelevantMesh(part)
        : hi.torsoMesh;
    
    // it is possible to assemble a character using several meshes, in which case torso mesh textures are different from i.e. leg mesh textures,
    // making it impossible to use just one composit texture per character. we therefore cache composit texture by humanoid pointer, but only if the mesh has the same configuration as torso
    bool useCache = (mesh == hi.torsoMesh) || (mesh && hi.torsoMesh && mesh->getBaseTextureId() == hi.torsoMesh->getBaseTextureId() && mesh->getOverlayTextureId() == hi.torsoMesh->getOverlayTextureId());

    std::pair<TextureRef, TextureCompositor::JobHandle> texture =
        useCache
            ? createHumanoidTextureCached(visualEngine, hi, accoutrements, mesh, compositCache)
            : createHumanoidTextureComposit(visualEngine, hi, accoutrements, mesh);
    
    return std::make_pair(texture, cfg.second);
}

static bool isWangTilling(PartMaterial material)
{
    if (material == COBBLESTONE_MATERIAL || FFlag::ForceWangTiles)
        return true;
    return false;
}

static const char* getMaterialName(PartMaterial material)
{
    switch (material)
    {
	case PLASTIC_MATERIAL: return "Plastic";
	case SMOOTH_PLASTIC_MATERIAL: return "SmoothPlastic";
	case NEON_MATERIAL: return "Neon";
	case WOOD_MATERIAL: return "Wood";
    case WOODPLANKS_MATERIAL: return "WoodPlanks";
    case MARBLE_MATERIAL: return "Marble";
    case SLATE_MATERIAL: return "Slate";
    case CONCRETE_MATERIAL: return "Concrete";
    case GRANITE_MATERIAL: return "Granite";
    case BRICK_MATERIAL: return "Brick";
    case PEBBLE_MATERIAL: return "Pebble";
    case RUST_MATERIAL: return "Rust";
    case DIAMONDPLATE_MATERIAL: return "Diamondplate";
    case ALUMINUM_MATERIAL: return "Aluminum";
    case METAL_MATERIAL: return "Metal";
    case GRASS_MATERIAL: return "Grass";
	case SAND_MATERIAL: return "Sand";
	case FABRIC_MATERIAL: return "Fabric";
	case ICE_MATERIAL: return "Ice";
    case COBBLESTONE_MATERIAL: return "Cobblestone";

	default:
		RBXASSERT(false);
		return "Plastic";
	}
};

static int getMaterialId(PartMaterial material, bool reflectance)
{
    switch (material)
    {
    case PLASTIC_MATERIAL: return 0 + reflectance;
    case SMOOTH_PLASTIC_MATERIAL: return 2 + reflectance;
    case WOOD_MATERIAL: return 4;
    case WOODPLANKS_MATERIAL: return 5;
    case MARBLE_MATERIAL: return 6;
    case SLATE_MATERIAL: return 7;
    case CONCRETE_MATERIAL: return 8;
    case GRANITE_MATERIAL: return 9;
    case BRICK_MATERIAL: return 10;
    case PEBBLE_MATERIAL: return 11;
    case COBBLESTONE_MATERIAL: return 12;
    case RUST_MATERIAL: return 13;
    case DIAMONDPLATE_MATERIAL: return 14;
    case ALUMINUM_MATERIAL: return 15;
    case METAL_MATERIAL: return 16;
    case GRASS_MATERIAL: return 17;
    case SAND_MATERIAL: return 18;
    case FABRIC_MATERIAL: return 19;
    case ICE_MATERIAL: return 20;
	case NEON_MATERIAL: return 21;

    default:
		return -1;
    }
}

static Vector4 getLQMatFarTilingFactor(PartMaterial material)
{
    if (isWangTilling(material)) return Vector4(1,1,1,1);

    switch (material)
    {
    case GRANITE_MATERIAL:
    case SLATE_MATERIAL:
    case ALUMINUM_MATERIAL:
    case CONCRETE_MATERIAL:
    case ICE_MATERIAL:
    case GRASS_MATERIAL:
        return Vector4(0.25f,0.25f,1,1);

    case RUST_MATERIAL:
    case COBBLESTONE_MATERIAL:
        return Vector4(0.5f,0.5f,1,1);

    default:
        return Vector4(1,1,1,1);
    }
}

static PartInstance* getHumanoidFocusPart(const HumanoidIdentifier& hi)
{
    if (hi.torso) return hi.torso;
    if (hi.head) return hi.head;
    
    return NULL;
}

static bool forceFlatPlastic(DataModelMesh* specialShape)
{
    if (SpecialShape* shape = specialShape->fastDynamicCast<SpecialShape>())
    {
        return shape->getMeshType() == SpecialShape::HEAD_MESH;
    }
    else
    {
        return false;
    }
}

#ifdef RBX_PLATFORM_IOS
static const std::string kTextureExtension = ".pvr";
#elif defined(__ANDROID__)
static const std::string kTextureExtension = ".pvr";
#else
static const std::string kTextureExtension = ".dds";
#endif

static void setupShadowDepthTechnique(Technique& technique)
{
    // This really culls back faces because SM space has different handedness
	technique.setRasterizerState(RasterizerState::Cull_Front);
}

static void setupTechnique(Technique& technique, unsigned int flags, bool hasGlow = false)
{
    /* BLENDING and Glow
       We use alpha channel of render target to save the intensity of glow. This way our post-process glow can work even with semi-transparent
       objects occluding glowing objects. 0 = full glow, 1 = no glow (it is easier to implement it like this)
       So simply glowing objects add to the glow and non glowing remove the glow. We archive that by separate alpha blend.

       Note: NEON shader output 1 - fog.a * src.alpha (fog.a = 0 -> full fog and vice versa). Its pretty 
       
       Glow-Opaque
        - alpha = src.alpha // simply full glow darkened by alpha.
       Glow-Transparent
        - alpha = dst * src.alpha // decreases alpha value that is in FB by proportionaly to the alpha of glowing object
       Non Glow-Opaque
        - alpha = src.a = 1
       Non Glow-Transparent
        - alpha = src.alpha * (1 - dst.alpha) + dst // imagine this as dst + lerp(src.alpha, 0, dst.alpha). Higher values of src.alpha decreases glow intensity more then smaller ones.
     */

	if (flags & MaterialGenerator::Flag_Transparent)
	{	
        if (FFlag::GlowEnabled)
        {
            if (hasGlow) // this is regular alpha blend. Src.a is just inverted for reasons explained in comment couple lines above.
                technique.setBlendState(BlendState(BlendState::Factor_InvSrcAlpha, BlendState::Factor_SrcAlpha, BlendState::Factor_Zero, BlendState::Factor_SrcAlpha));
            else
                technique.setBlendState(BlendState(BlendState::Factor_SrcAlpha, BlendState::Factor_InvSrcAlpha, BlendState::Factor_InvDstAlpha, BlendState::Factor_One));
        }
        else
            technique.setBlendState(BlendState::Mode_AlphaBlend);

		
        technique.setDepthState(DepthState(DepthState::Function_LessEqual, false));
	}
	else if (FFlag::GlowEnabled && hasGlow)
	{
		technique.setBlendState(BlendState(BlendState::Factor_One, BlendState::Factor_Zero, BlendState::Factor_One, BlendState::Factor_Zero));
	}

    if (flags & (MaterialGenerator::Flag_ForceDecal | MaterialGenerator::Flag_ForceDecalTexture))
		technique.setRasterizerState(RasterizerState(RasterizerState::Cull_Back, -16));

}

static void setupPlasticTextures(VisualEngine* visualEngine, Technique& technique)
{
    TextureManager* tm = visualEngine->getTextureManager();
	technique.setTexture(5, tm->load(ContentId("rbxasset://textures/plastic/diffuse.dds"), TextureManager::Fallback_Gray), SamplerState::Filter_Anisotropic);
	technique.setTexture(6, tm->load(ContentId("rbxasset://textures/plastic/normal.dds"), TextureManager::Fallback_NormalMap), SamplerState::Filter_Anisotropic);
	technique.setTexture(8, tm->load(ContentId("rbxasset://textures/plastic/normaldetail" + kTextureExtension), TextureManager::Fallback_NormalMap), SamplerState::Filter_Linear);
}

static void setupSmoothPlasticTextures(VisualEngine* visualEngine, Technique& technique)
{
    TextureManager* tm = visualEngine->getTextureManager();

	technique.setTexture(5, tm->getFallbackTexture(TextureManager::Fallback_White), SamplerState::Filter_Linear);
}

static void setupComplexMaterialTextures(VisualEngine* visualEngine, Technique& technique, const std::string& materialName, const TextureRef* wangTileTex)
{
    TextureManager* tm = visualEngine->getTextureManager();
    std::string texturePath = "rbxasset://textures/" + materialName + "/";

	safeToLower(texturePath);

	technique.setTexture(5, tm->load(ContentId(texturePath + "diffuse" + kTextureExtension), TextureManager::Fallback_White), SamplerState::Filter_Anisotropic);
	technique.setTexture(6, tm->load(ContentId(texturePath + "normal" + kTextureExtension), TextureManager::Fallback_NormalMap), SamplerState::Filter_Anisotropic);
	technique.setTexture(7, tm->load(ContentId(texturePath + "specular" + kTextureExtension), TextureManager::Fallback_Black), SamplerState::Filter_Anisotropic);

	if (wangTileTex)
        technique.setTexture(8, *wangTileTex, SamplerState::Filter_Point);
    else
	    technique.setTexture(8, tm->load(ContentId(texturePath + "normaldetail" + kTextureExtension), TextureManager::Fallback_NormalMap), SamplerState::Filter_Anisotropic);
}

static void setupLQMaterialTextures(VisualEngine* visualEngine, Technique& technique, const std::string& materialName, const TextureRef* wangTileTex)
{
    TextureManager* tm = visualEngine->getTextureManager();
    std::string texturePath = "rbxasset://textures/" + materialName + "/";

    safeToLower(texturePath);

    technique.setTexture(5, tm->load(ContentId(texturePath + "diffuse" + kTextureExtension), TextureManager::Fallback_White), SamplerState::Filter_Linear);

    if (wangTileTex)
        technique.setTexture(8, *wangTileTex, SamplerState::Filter_Point);
}

static void setupCommonTextures(VisualEngine* visualEngine, Technique& technique)
{
    LightGrid* lightGrid = visualEngine->getLightGrid();
	SceneManager* sceneManager = visualEngine->getSceneManager();

	if (lightGrid && lightGrid->hasTexture())
	{
        technique.setTexture(1, lightGrid->getTexture(), SamplerState::Filter_Linear);
		technique.setTexture(2, lightGrid->getLookupTexture(), SamplerState(SamplerState::Filter_Point, SamplerState::Address_Clamp));
	}
	else
	{
        technique.setTexture(1, shared_ptr<Texture>(), SamplerState::Filter_Linear);
        technique.setTexture(2, shared_ptr<Texture>(), SamplerState::Filter_Point);
	}

	technique.setTexture(3, sceneManager->getShadowMap(), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));

	technique.setTexture(4, sceneManager->getEnvMap()->getTexture(), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));
}

static void setupMaterialTextures(VisualEngine* ve, Technique& technique, PartMaterial renderMaterial, const std::string& materialName, const TextureRef* wangTileTex)
{
	if (renderMaterial == PLASTIC_MATERIAL)
        setupPlasticTextures(ve, technique);
    else if (renderMaterial == SMOOTH_PLASTIC_MATERIAL || renderMaterial == NEON_MATERIAL)
        setupSmoothPlasticTextures(ve, technique);
    else
        setupComplexMaterialTextures(ve, technique, materialName, wangTileTex);
}

MaterialGenerator::TexturedMaterialCache::TexturedMaterialCache()
	: gcSizeLast(0)
{
}

MaterialGenerator::MaterialGenerator(VisualEngine* visualEngine)
    : visualEngine(visualEngine)
    , compositCache(NULL, TextureCompositor::JobHandle())
{
    wangTilesTex = visualEngine->getTextureManager()->load(ContentId("rbxasset://textures/wangIndex.dds"), TextureManager::Fallback_Black);
}

shared_ptr<Material> MaterialGenerator::createBaseMaterial(unsigned int flags)
{
	unsigned int cacheKey = flags & Flag_CacheMask;

    // Cache lookup
    if (baseMaterialCache[cacheKey])
        return baseMaterialCache[cacheKey];

    // Create material
    shared_ptr<Material> material(new Material());
    
	std::string vertexSkinning = (flags & Flag_Skinned) ? "Skinned" : "Static";

	if ((flags & (Flag_Transparent | Flag_ForceDecal | Flag_ForceDecalTexture)) == 0)
        if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram("Default" + vertexSkinning + "HQVS", "DefaultHQGBufferFS"))
        {
            Technique technique(program, 0);

            setupTechnique(technique, flags);

            technique.setTexture(0, TextureRef(), SamplerState::Filter_Linear);

			setupCommonTextures(visualEngine, technique);
            setupSmoothPlasticTextures(visualEngine, technique);

            material->addTechnique(technique);
        }

    if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram("Default" + vertexSkinning + "HQVS", "DefaultHQFS"))
    {
        Technique technique(program, 1);

		setupTechnique(technique, flags);

        technique.setTexture(0, TextureRef(), SamplerState::Filter_Linear);

        setupCommonTextures(visualEngine, technique);
		setupSmoothPlasticTextures(visualEngine, technique);

        material->addTechnique(technique);
    }

	if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgramOrFFP("Default" + vertexSkinning + "VS", "DefaultFS"))
    {
        Technique technique(program, 2);

		setupTechnique(technique, flags);

        technique.setTexture(0, TextureRef(), SamplerState::Filter_Linear);

        setupCommonTextures(visualEngine, technique);
		setupSmoothPlasticTextures(visualEngine, technique);

        material->addTechnique(technique);
    }

	if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram("DefaultShadow" + vertexSkinning + "VS", "DefaultShadowFS"))
    {
        Technique technique(program, 0, RenderQueue::Pass_Shadows);

        setupShadowDepthTechnique(technique);

        material->addTechnique(technique);
    }

    // Fast cache fill
    baseMaterialCache[cacheKey] = material;
        
    return material;
}

shared_ptr<Material> MaterialGenerator::createRenderMaterial(unsigned int flags, PartMaterial renderMaterial, bool reflectance)
{
	int materialId = getMaterialId(renderMaterial, reflectance);
	if (materialId < 0) return shared_ptr<Material>();

	RBXASSERT(materialId >= 0 && materialId < ARRAYSIZE(renderMaterialCache));

	unsigned int cacheKey = flags & Flag_CacheMask;
    
    // Fast cache lookup
    if (renderMaterialCache[materialId][cacheKey])
        return renderMaterialCache[materialId][cacheKey];
    
    // Create material
    shared_ptr<Material> material(new Material());
    
    ContentId studs("rbxasset://textures/studs.dds");

	TextureManager* tm = visualEngine->getTextureManager();

	std::string materialName = getMaterialName(renderMaterial);
    
    bool isWang = isWangTilling(renderMaterial);
    bool hasGlow = renderMaterial == NEON_MATERIAL;

	std::string materialNameReflectance = materialName + (reflectance ? "Reflection" : "");

	std::string vertexSkinning = (flags & Flag_Skinned) ? "Skinned" : "Static";
    std::string vertexShader =
		(renderMaterial == SMOOTH_PLASTIC_MATERIAL || renderMaterial == NEON_MATERIAL)
        ? "Default" + vertexSkinning + "HQVS"
        : "Default" + vertexSkinning + "SurfaceHQVS";

	if ((flags & Flag_Transparent) == 0)
        if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram(vertexShader, "Default" + materialNameReflectance + "HQGBufferFS"))
        {
            Technique technique(program, 0);

            setupTechnique(technique, flags, hasGlow);

            technique.setTexture(0, tm->load(studs, TextureManager::Fallback_Gray), SamplerState::Filter_Anisotropic);

            setupCommonTextures(visualEngine, technique);
            setupMaterialTextures(visualEngine, technique, renderMaterial, materialName, isWang ? &wangTilesTex : NULL);

            material->addTechnique(technique);
        }

    if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram(vertexShader, "Default" + materialNameReflectance + "HQFS"))
    {
        Technique technique(program, 1);

		setupTechnique(technique, flags, hasGlow);

        technique.setTexture(0, tm->load(studs, TextureManager::Fallback_Gray), SamplerState::Filter_Anisotropic);

        setupCommonTextures(visualEngine, technique);
        setupMaterialTextures(visualEngine, technique, renderMaterial, materialName, isWang ? &wangTilesTex : NULL);

        material->addTechnique(technique);
    }

    // LOD shaders for non-plastic materials use low-quality plastic shaders; plastic has reflection even in lod
	std::string lodVS = std::string("Default") + vertexSkinning + (reflectance ? "Reflection" : "") + "VS";
    std::string lodFS = ""; 

    switch (renderMaterial)
    {
    case RBX::PLASTIC_MATERIAL:
    case RBX::SMOOTH_PLASTIC_MATERIAL:
        {
            lodFS = reflectance ? "Default" + materialNameReflectance + "FS" : "DefaultPlasticFS";
            break;
        }
    case RBX::NEON_MATERIAL:
        {
            lodFS = "DefaultNeonFS";
            break;
        }
    default:
        {
            if (isWang)
            {
                if (visualEngine->getShaderManager()->getProgram(lodVS, "LowQMaterialWangFS"))
                    lodFS = "LowQMaterialWangFS";
                else
                    lodFS = "LowQMaterialWangFallbackFS";
            }
            else
                lodFS = "LowQMaterialFS";

            break;
        }
    }

    if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgramOrFFP(lodVS, lodFS))
    {
        Technique technique(program, 2);

        setupTechnique(technique, flags, hasGlow);

        technique.setTexture(0, tm->load(ContentId(studs), TextureManager::Fallback_Gray), SamplerState::Filter_Linear);

        setupCommonTextures(visualEngine, technique);
        
        if( renderMaterial == PLASTIC_MATERIAL || renderMaterial == SMOOTH_PLASTIC_MATERIAL || renderMaterial == NEON_MATERIAL)
            setupSmoothPlasticTextures(visualEngine, technique);
        else
        {
            setupLQMaterialTextures(visualEngine, technique, materialName, isWang ? &wangTilesTex : NULL);
            technique.setConstant( "LqmatFarTilingFactor", getLQMatFarTilingFactor(renderMaterial) );
        }

        material->addTechnique(technique); 
    }

   	if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram("DefaultShadow" + vertexSkinning + "VS", "DefaultShadowFS"))
    {
        Technique technique(program, 0, RenderQueue::Pass_Shadows);

        setupShadowDepthTechnique(technique);

        material->addTechnique(technique);
    }

    // Fast cache fill
	renderMaterialCache[materialId][cacheKey] = material;
        
    return material;
}

shared_ptr<Material> MaterialGenerator::createTexturedMaterial(const TextureRef& texture, const std::string& textureName, unsigned int flags)
{
	unsigned int cacheKey = flags & Flag_CacheMask;
    
    // Fast cache lookup
	TexturedMaterialMap::iterator it = texturedMaterialCache[cacheKey].map.find(textureName);

	if (it != texturedMaterialCache[cacheKey].map.end())
        return it->second;

	shared_ptr<Material> baseMaterial = createBaseMaterial(flags);

    if (!baseMaterial)
        return shared_ptr<Material>();

    shared_ptr<Material> material(new Material());

	unsigned int diffuseMapStage = visualEngine->getDevice()->getCaps().supportsFFP ? 0 : kDiffuseMapStage;

    SamplerState::Filter filter = (flags & (Flag_ForceDecal | Flag_ForceDecalTexture)) ? SamplerState::Filter_Anisotropic : SamplerState::Filter_Linear;
    SamplerState::Address address = (flags & Flag_ForceDecal) ? SamplerState::Address_Clamp : SamplerState::Address_Wrap;

	const std::vector<Technique>& techniques = baseMaterial->getTechniques();

	for (size_t i = 0; i < techniques.size(); ++i)
	{
        Technique t = techniques[i];

		t.setTexture(diffuseMapStage, texture, SamplerState(filter, address));

		material->addTechnique(t);
	}
    
    // Fast cache fill
	texturedMaterialCache[cacheKey].map[textureName] = material;

    return material;
}

MaterialGenerator::Result MaterialGenerator::createDefaultMaterial(PartInstance* part, unsigned int flags, PartMaterial renderMaterial)
{
    PartMaterial actualRenderMaterial = renderMaterial;

#if defined(RBX_PLATFORM_IOS) || defined(__ANDROID__)
    if (!FFlag::RenderMaterialsOnMobile)
    {
        // Force everything to smooth plastic to reduce texture memory
        actualRenderMaterial = SMOOTH_PLASTIC_MATERIAL;
    }
#endif

    unsigned int features = renderMaterial == NEON_MATERIAL ? RenderQueue::Features_Glow : 0;

    // Reflectance is only supported for plastic
	if ((renderMaterial == SMOOTH_PLASTIC_MATERIAL || renderMaterial == PLASTIC_MATERIAL) && part->getReflectance() > 0.015f)
    {
        return Result(createRenderMaterial(flags, actualRenderMaterial, true), 0, features);
    }
    else
    {
        return Result(createRenderMaterial(flags, actualRenderMaterial, false), (flags & Flag_Transparent) ? 0 : Result_PlasticLOD, features);
    }
}

MaterialGenerator::Result MaterialGenerator::createMaterialForPart(PartInstance* part, const HumanoidIdentifier* hi, unsigned int flags)
{
    if (hi && (flags & Flag_UseCompositTexture))
    {
        std::pair<std::pair<TextureRef, TextureCompositor::JobHandle>, G3D::Vector4> htp = createHumanoidTexture(visualEngine, part, *hi, flags, compositCache);
        
        if (htp.first.first.getTexture())
        {
            shared_ptr<Material> material = createTexturedMaterial(htp.first.first, visualEngine->getTextureCompositor()->getTextureId(htp.first.second), flags);
            
            // attach focus part to texture to make sure texture has an appropriate priority
            visualEngine->getTextureCompositor()->attachInstance(htp.first.second, shared_from(getHumanoidFocusPart(*hi)));
            
            return Result(material, Result_UsesTexture | Result_UsesCompositTexture, 0, htp.second);
        }
    }
    
    DataModelMesh* specialShape = getSpecialShape(part);

    if (FileMesh* fileMesh = getFileMesh(specialShape))
    {
		const TextureId& textureId = fileMesh->getTextureId();

		TextureRef texture = textureId.isNull() ? TextureRef() : visualEngine->getTextureManager()->load(textureId, TextureManager::Fallback_Gray, fileMesh->getFullName() + ".TextureId");

        return texture.getTexture()
			? Result(createTexturedMaterial(texture, textureId.toString(), flags), Result_UsesTexture)
            : createDefaultMaterial(part, flags, SMOOTH_PLASTIC_MATERIAL);
    }
    
    if ((flags & Flag_DisableMaterialsAndStuds) != 0 || (specialShape != NULL && forceFlatPlastic(specialShape)))
        return createDefaultMaterial(part, flags, SMOOTH_PLASTIC_MATERIAL);
    else
        return createDefaultMaterial(part, flags, part->getRenderMaterial());
}

MaterialGenerator::Result MaterialGenerator::createMaterialForDecal(Decal* decal, unsigned int flags)
{
	const TextureId& textureId = decal->getTexture();

	TextureRef texture = textureId.isNull() ? TextureRef() : visualEngine->getTextureManager()->load(textureId, TextureManager::Fallback_BlackTransparent, decal->getFullName() + ".Texture");

	return texture.getTexture()
        ? Result(createTexturedMaterial(texture, textureId.toString(), flags), Result_UsesTexture)
        : Result();
}

MaterialGenerator::Result MaterialGenerator::createMaterial(PartInstance* part, Decal* decal, const HumanoidIdentifier* hi, unsigned int flags)
{
    if (decal)
    {
        if (decal->isA<DecalTexture>())
            return createMaterialForDecal(decal, flags | Flag_ForceDecalTexture);
        else
            return createMaterialForDecal(decal, flags | Flag_ForceDecal);
    }
    else
        return createMaterialForPart(part, hi, flags);
}

void MaterialGenerator::invalidateCompositCache()
{
	compositCache = std::make_pair(static_cast<Humanoid*>(NULL), TextureCompositor::JobHandle());
}

void MaterialGenerator::garbageCollectIncremental()
{
	for (unsigned int i = 0; i < ARRAYSIZE(texturedMaterialCache); ++i)
	{
		TexturedMaterialCache& cache = texturedMaterialCache[i];

        // To catch up with allocation rate we need to visit the number of allocated elements since last run plus a small constant
		size_t visitCount = std::min(cache.map.size(), std::max(cache.map.size(), cache.gcSizeLast) - cache.gcSizeLast + 8);

		TexturedMaterialMap::iterator it = cache.map.find(cache.gcKeyNext);

        for (size_t j = 0; j < visitCount; ++j)
		{
            if (it == cache.map.end())
                it = cache.map.begin();

			if (it->second.unique())
				it = cache.map.erase(it);
			else
                ++it;
		}

		cache.gcSizeLast = cache.map.size();
		cache.gcKeyNext = (it == cache.map.end()) ? "" : it->first;
	}
}

void MaterialGenerator::garbageCollectFull()
{
	for (unsigned int i = 0; i < ARRAYSIZE(texturedMaterialCache); ++i)
	{
		TexturedMaterialCache& cache = texturedMaterialCache[i];

		for (TexturedMaterialMap::iterator it = cache.map.begin(); it != cache.map.end(); )
			if (it->second.unique())
				it = cache.map.erase(it);
			else
                ++it;

		cache.gcSizeLast = cache.map.size();
        cache.gcKeyNext = "";
	}
}

Vector2int16 MaterialGenerator::getSpecular(PartMaterial material)
{
    switch (material)
    {
    case PLASTIC_MATERIAL: return Vector2int16(102, 9);
    case SMOOTH_PLASTIC_MATERIAL: return Vector2int16(191, 81);
	case NEON_MATERIAL: return Vector2int16(191, 81);
    case WOOD_MATERIAL: return Vector2int16(64, 32);
    case WOODPLANKS_MATERIAL: return Vector2int16(71, 53);
    case MARBLE_MATERIAL: return Vector2int16(179, 54);
    case SLATE_MATERIAL: return Vector2int16(36, 20);
    case CONCRETE_MATERIAL: return Vector2int16(38, 22);
    case GRANITE_MATERIAL: return Vector2int16(48, 24);
    case BRICK_MATERIAL: return Vector2int16(33, 44);
    case PEBBLE_MATERIAL: return Vector2int16(18, 22);
    case COBBLESTONE_MATERIAL: return Vector2int16(54, 22);
    case RUST_MATERIAL: return Vector2int16(89, 103);
    case DIAMONDPLATE_MATERIAL: return Vector2int16(230, 160);
    case ALUMINUM_MATERIAL: return Vector2int16(238, 240);
    case METAL_MATERIAL: return Vector2int16(204, 120);
    case GRASS_MATERIAL: return Vector2int16(43, 18);
    case SAND_MATERIAL: return Vector2int16(18, 6);
    case FABRIC_MATERIAL: return Vector2int16(8, 16);
    case ICE_MATERIAL: return Vector2int16(255, 190);

    default:
        RBXASSERT(0); // You missed new material
        return Vector2int16(0, 50);
    }
}

unsigned int MaterialGenerator::createFlags(bool skinned, RBX::PartInstance* part, const HumanoidIdentifier* hi, bool& ignoreDecalsOut)
{
	bool useCompositTexture = hi && hi->humanoid && hi->isPartComposited(part);
	ignoreDecalsOut = false;

	unsigned int materialFlags = skinned ? MaterialGenerator::Flag_Skinned : 0;

	if (hi && part == hi->head &&  hi->isPartHead(part))
	{
		// Heads don't support materials/studs
		materialFlags |= MaterialGenerator::Flag_DisableMaterialsAndStuds;
	}

	if (useCompositTexture)
	{
		materialFlags |= MaterialGenerator::Flag_UseCompositTexture;

		// Bake all accoutrements in the same composit texture
		materialFlags |= MaterialGenerator::Flag_UseCompositTextureForAccoutrements;

		// If torso has a tshirt, ignore all decals
		if (part == hi->torso && !hi->shirtGraphic.isNull())
			ignoreDecalsOut = true;

		// Ignore head decals since they are composited
		if (part == hi->head)
			ignoreDecalsOut = true;
	}

	if (part->getTransparencyUi() > 0)
	{
		materialFlags |= MaterialGenerator::Flag_Transparent;
	}
	else if (useCompositTexture)
	{
		// Some accoutrements need alpha kill (i.e. feather on a hat)
		// We enable it on all composited materials to batch body parts and accoutrements together
		materialFlags |= MaterialGenerator::Flag_AlphaKill;
	}

	return materialFlags;
}

float MaterialGenerator::getTiling(PartMaterial material)
{
    switch (material)
    {
    case PLASTIC_MATERIAL: return 1.3f;
    case SMOOTH_PLASTIC_MATERIAL: return 1.f;
	case NEON_MATERIAL: return 1.f;
    case WOOD_MATERIAL: return 0.2f;
    case WOODPLANKS_MATERIAL: return 0.2f;
    case MARBLE_MATERIAL: return 0.1f;
    case SLATE_MATERIAL: return 0.1f;
    case CONCRETE_MATERIAL: return 0.15f;
    case GRANITE_MATERIAL: return 0.1f;
    case BRICK_MATERIAL: return 0.125f;
    case PEBBLE_MATERIAL: return 0.1f;
    case RUST_MATERIAL: return 0.1f;
    case DIAMONDPLATE_MATERIAL: return 0.2f;
    case ALUMINUM_MATERIAL: return 0.1f;
    case METAL_MATERIAL: return 0.2f;
    case GRASS_MATERIAL: return 0.15f;
    case SAND_MATERIAL: return 0.1f;
    case FABRIC_MATERIAL: return 0.15f;
    case ICE_MATERIAL: return 0.1f;
    case COBBLESTONE_MATERIAL:
        {
            if (isWangTilling(material))
                return 0.27f * 0.25f;
            else
                return 0.2f;
        }
    default:
        RBXASSERT(0); // You missed new material
        return 1.f;
    }
}

unsigned int MaterialGenerator::getDiffuseMapStage()
{
	return kDiffuseMapStage;
}

}
}
