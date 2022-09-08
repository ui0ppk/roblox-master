#pragma once

#include "g3d/Vector4.h"

#include "TextureCompositor.h"

#include "v8datamodel/PartInstance.h"

namespace RBX
{
	class Decal;
	class Humanoid;
	class HumanoidIdentifier;
}

namespace RBX
{
namespace Graphics
{

static const unsigned kMatCacheSize = 32;

class VisualEngine;
class Material;
class TextureRef;

class MaterialGenerator
{
public:
    enum MaterialFlags
    {
        Flag_Skinned = 1 << 0,
        Flag_Transparent = 1 << 1,
        Flag_AlphaKill = 1 << 2,
        Flag_ForceDecal = 1 << 3,
        Flag_ForceDecalTexture = 1 << 4,
        Flag_UseCompositTexture = 1 << 5,
        Flag_UseCompositTextureForAccoutrements = 1 << 6,
        Flag_DisableMaterialsAndStuds = 1 << 7,

		Flag_CacheMask = Flag_Skinned | Flag_Transparent | Flag_AlphaKill | Flag_ForceDecal | Flag_ForceDecalTexture
    };
    
    enum ResultFlags
    {
        Result_UsesTexture = 1 << 0,
        Result_UsesCompositTexture = 1 << 1,
        Result_PlasticLOD = 1 << 2,
    };

    struct Result
    {
        explicit Result(const shared_ptr<Material>& material = shared_ptr<Material>(), unsigned int flags = 0, unsigned int features = 0, const G3D::Vector4& uvOffsetScale = G3D::Vector4(0, 0, 1, 1))
            : material(material)
            , flags(flags)
            , features(features)
            , uvOffsetScale(uvOffsetScale)
        {
            
        }
        
        shared_ptr<Material> material;
        unsigned int flags;
        unsigned int features;
        G3D::Vector4 uvOffsetScale;
    };
    
    MaterialGenerator(VisualEngine* visualEngine);
    
    Result createMaterial(PartInstance* part, Decal* decal, const HumanoidIdentifier* hi, unsigned int flags);

    void invalidateCompositCache();

    void garbageCollectIncremental();
    void garbageCollectFull();

    static Vector2int16 getSpecular(PartMaterial material);
    static float getTiling(PartMaterial material);

	static unsigned int createFlags(bool skinned, RBX::PartInstance* part, const HumanoidIdentifier* humanoidIdentifier, bool& ignoreDecalsOut);

    static unsigned int getDiffuseMapStage();

private:
    VisualEngine* visualEngine;

    typedef boost::unordered_map<std::string, shared_ptr<Material> > TexturedMaterialMap;
	
	struct TexturedMaterialCache
	{
        TexturedMaterialMap map;

        size_t gcSizeLast;
        std::string gcKeyNext;

		TexturedMaterialCache();
	};

    shared_ptr<Material> baseMaterialCache[Flag_CacheMask + 1]; // an entry for each supported flag combination
    shared_ptr<Material> renderMaterialCache[kMatCacheSize][Flag_CacheMask + 1]; // an entry for each render material and each supported flag combination
	TexturedMaterialCache texturedMaterialCache[Flag_CacheMask + 1]; // an entry for each supported flag combination

    std::pair<Humanoid*, TextureCompositor::JobHandle> compositCache;
    
    shared_ptr<Material> createBaseMaterial(unsigned int flags);
    shared_ptr<Material> createRenderMaterial(unsigned int flags, PartMaterial renderMaterial, bool reflectance);
    shared_ptr<Material> createTexturedMaterial(const TextureRef& texture, const std::string& textureName, unsigned int flags);

    Result createDefaultMaterial(PartInstance* part, unsigned int flags, PartMaterial renderMaterial);
    Result createMaterialForPart(PartInstance* part, const HumanoidIdentifier* hi, unsigned int flags);
    Result createMaterialForDecal(Decal* decal, unsigned int flags);

    TextureRef  wangTilesTex;
};

}
}
