#include "stdafx.h"
#include "GeometryGenerator.h"

#include "GfxBase/AsyncResult.h"
#include "GfxBase/PartIdentifier.h"
#include "humanoid/Humanoid.h"
#include "VisualEngine.h"
#include "GfxBase/RenderCaps.h"
#include "GfxCore/Device.h"

#include "util/MeshId.h"
#include "Util.h"
#include "util/Lcmrand.h"
#include "util/NormalId.h"

#include "v8datamodel/CharacterMesh.h"
#include "v8datamodel/SpecialMesh.h"
#include "v8datamodel/FileMesh.h"
#include "v8datamodel/ContentProvider.h"
#include "v8datamodel/MeshContentProvider.h"
#include "v8datamodel/BlockMesh.h"
#include "v8datamodel/CylinderMesh.h"
#include "v8datamodel/Decal.h"
#include "v8datamodel/ExtrudedPartInstance.h"
#include "v8datamodel/PartCookie.h"
#include "v8datamodel/Lighting.h"
#include "v8world/Primitive.h"
#include "V8DataModel/PartOperation.h"
#include "V8DataModel/PartOperationAsset.h"
#include "V8DataModel/SolidModelContentProvider.h"

#include "g3d/g3dmath.h"

#include "GfxBase/FileMeshData.h"
#include "GfxBase/GfxPart.h"

#include "MaterialGenerator.h"
#include "V8World/TriangleMesh.h"
#include "BulletCollision/CollisionShapes/btConvexHullShape.h"
#include "BulletCollision/CollisionShapes/btShapeHull.h"
#include "BulletCollision/CollisionShapes/btConvexPolyhedron.h"
#include "Extras/GIMPACTUtils/btGImpactConvexDecompositionShape.h"

FASTFLAGVARIABLE(NoRandomColorsWithoutOutlines, true)
FASTFLAGVARIABLE(FixMeshOffset, false)
FASTFLAG(FixGlowingCSG)
FASTFLAG(StudioCSGAssets)
FASTFLAG(CSGLoadBlocking)
FASTFLAG(CSGPhysicsLevelOfDetailEnabled)

#define TESSELATION_PIECE 8.0f
#define MAX_TESSELATION 50.0f

#define NO_OUTLINES 10.0f


namespace RBX
{
namespace Graphics
{
	static const boost::shared_ptr<FileMeshData> kDummyMeshData(new FileMeshData());
	
	static const float kNormalOffsetScale = 0.02f;
	
	static const Vector3 kNormalOffsetTable[32] =
	{
		Vector3(-0.5237f, +0.7208f, -0.5878f) * kNormalOffsetScale,
		Vector3(-0.5590f, +0.7694f, -0.5878f) * kNormalOffsetScale,
		Vector3(-0.2668f, +0.3673f, -0.5878f) * kNormalOffsetScale,
		Vector3(+0.1816f, -0.2500f, -0.5878f) * kNormalOffsetScale,
		Vector3(-0.8800f, +0.1394f, -0.9877f) * kNormalOffsetScale,
		Vector3(-0.9393f, +0.1488f, -0.9877f) * kNormalOffsetScale,
		Vector3(-0.4484f, +0.0710f, -0.9877f) * kNormalOffsetScale,
		Vector3(+0.3052f, -0.0483f, -0.9877f) * kNormalOffsetScale,
		Vector3(-0.7208f, -0.5237f, -0.8090f) * kNormalOffsetScale,
		Vector3(-0.7694f, -0.5590f, -0.8090f) * kNormalOffsetScale,
		Vector3(-0.3673f, -0.2668f, -0.8090f) * kNormalOffsetScale,
		Vector3(+0.2500f, +0.1816f, -0.8090f) * kNormalOffsetScale,
		Vector3(-0.1394f, -0.8800f, -0.1564f) * kNormalOffsetScale,
		Vector3(-0.1488f, -0.9393f, -0.1564f) * kNormalOffsetScale,
		Vector3(-0.0710f, -0.4484f, -0.1564f) * kNormalOffsetScale,
		Vector3(+0.0483f, +0.3052f, -0.1564f) * kNormalOffsetScale,
		Vector3(+0.5237f, -0.7208f, +0.5878f) * kNormalOffsetScale,
		Vector3(+0.5590f, -0.7694f, +0.5878f) * kNormalOffsetScale,
		Vector3(+0.2668f, -0.3673f, +0.5878f) * kNormalOffsetScale,
		Vector3(-0.1816f, +0.2500f, +0.5878f) * kNormalOffsetScale,
		Vector3(+0.8800f, -0.1394f, +0.9877f) * kNormalOffsetScale,
		Vector3(+0.9393f, -0.1488f, +0.9877f) * kNormalOffsetScale,
		Vector3(+0.4484f, -0.0710f, +0.9877f) * kNormalOffsetScale,
		Vector3(-0.3052f, +0.0483f, +0.9877f) * kNormalOffsetScale,
		Vector3(+0.7208f, +0.5237f, +0.8090f) * kNormalOffsetScale,
		Vector3(+0.7694f, +0.5590f, +0.8090f) * kNormalOffsetScale,
		Vector3(+0.3673f, +0.2668f, +0.8090f) * kNormalOffsetScale,
		Vector3(-0.2500f, -0.1816f, +0.8090f) * kNormalOffsetScale,
		Vector3(+0.1394f, +0.8800f, +0.1564f) * kNormalOffsetScale,
		Vector3(+0.1488f, +0.9393f, +0.1564f) * kNormalOffsetScale,
		Vector3(+0.0710f, +0.4484f, +0.1564f) * kNormalOffsetScale,
		Vector3(-0.0483f, -0.3052f, +0.1564f) * kNormalOffsetScale,
	};
	
	static const float kUniformRandomTable[64] =
	{
		0.827323f, 0.526485f, 0.916254f, 0.225426f, 0.969743f, 0.069476f, 0.423928f, 0.794976f,
		0.629325f, 0.707109f, 0.997080f, 0.371298f, 0.537122f, 0.692187f, 0.309150f, 0.207611f,
		0.347299f, 0.680269f, 0.854916f, 0.321542f, 0.848045f, 0.825441f, 0.699820f, 0.365232f,
		0.391758f, 0.982131f, 0.961126f, 0.854716f, 0.847131f, 0.427982f, 0.901692f, 0.705821f,
		0.230473f, 0.367524f, 0.324393f, 0.797571f, 0.048550f, 0.245296f, 0.805075f, 0.484867f,
		0.740669f, 0.375044f, 0.792494f, 0.275678f, 0.915932f, 0.376178f, 0.625879f, 0.648950f,
		0.213511f, 0.454967f, 0.172181f, 0.734572f, 0.339370f, 0.280913f, 0.053203f, 0.001882f,
		0.826664f, 0.551022f, 0.833667f, 0.987611f, 0.108350f, 0.569212f, 0.947845f, 0.201343f,
	};
	
	struct UVAtlasInfo
	{
		bool tileX;
		float scaleY, offsetY;
		
		UVAtlasInfo(bool tileX, float scaleY, float offsetY): tileX(tileX), scaleY(scaleY), offsetY(offsetY)
		{
		}
	};

	struct FACEDESC
	{
		unsigned start0, start1;
		unsigned end0, end1;
		bool dirZ;
		bool tangentZ;
	};

	inline float transformUV(float v, float scale, float offset)
	{
		const float threshold = 1e-2f;
		
		if (v < -threshold || v > 1.f + threshold)
			return (v - floorf(v)) * scale + offset;
		else
			return v * scale + offset;
	}
	
	inline Vector2 transformUV(const Vector2& uv, const GeometryGenerator::Options& options)
	{
		return
			(options.flags & GeometryGenerator::Flag_TransformUV)
				? Vector2(transformUV(uv.x, options.uvframe.z, options.uvframe.x), transformUV(uv.y, options.uvframe.w, options.uvframe.y))
				: uv;
	}

	inline UVAtlasInfo getStudsAtlasInfo(SurfaceType type)
	{
		switch(type)
		{
		case STUDS:
			return UVAtlasInfo(true, 0.25f, 0.0f);
			break;
		case WELD:
			return UVAtlasInfo(true, 0.25f, 0.25f);
			break;
		case INLET:
			return UVAtlasInfo(true, 0.25f, 0.5f);
			break;
		case UNIVERSAL:
			return UVAtlasInfo(true, 0.25f, 0.75f);
			break;
		case GLUE:
			return UVAtlasInfo(true, 0.25f, 0.25f);
			break;
		default:
			return UVAtlasInfo(false, 0.0f, 0.0f);
		}
	}
	
#ifdef _MSC_VER
	__forceinline
#else
	inline
#endif
	void fillVertex(
		GeometryGenerator::Vertex& r, 
		const Vector3& pos, 
		const Vector3& normal, 
		const Vector2& uv, 
		const Vector2& uvStuds, 
		const Color4uint8& color, 
		const Color4uint8& extra, 
		const Vector3& tangent, 
		const Vector4& edgeDistances = Vector4(NO_OUTLINES, NO_OUTLINES, NO_OUTLINES, NO_OUTLINES))
	{
		r.pos = pos;
		r.normal = normal;
		r.uv = uv;
		r.uvStuds = uvStuds;
		r.color = color;
		r.extra = extra;
		r.tangent = tangent;
		r.edgeDistances = edgeDistances;
	}
	
	inline void fillQuadIndices(unsigned short* r, size_t offset, unsigned int i00, unsigned int i10, unsigned int i01, unsigned int i11)
	{
		r[0] = offset + i00;
		r[1] = offset + i10;
		r[2] = offset + i01;
		
		r[3] = offset + i01;
		r[4] = offset + i10;
		r[5] = offset + i11;
	}
	
	inline void extendBounds(Vector3& boundsMin, Vector3& boundsMax, const Vector3& pos)
	{
		boundsMin = boundsMin.min(pos);
		boundsMax = boundsMax.max(pos);
	}

	inline void extendBoundsBlock(Vector3& boundsMin, Vector3& boundsMax, const Vector3& center, const Vector3& size, 
		const Vector3& axisX, const Vector3& axisY, const Vector3& axisZ )
	{
		Vector3 extent = size * 0.5f;

		Vector3 boundsExtent = Vector3(
			fabsf(axisX.x) * extent.x + fabsf(axisY.x) * extent.y + fabsf(axisZ.x) * extent.z,
			fabsf(axisX.y) * extent.x + fabsf(axisY.y) * extent.y + fabsf(axisZ.y) * extent.z,
			fabsf(axisX.z) * extent.x + fabsf(axisY.z) * extent.y + fabsf(axisZ.z) * extent.z);

		boundsMin = boundsMin.min(center - boundsExtent);
		boundsMax = boundsMax.max(center + boundsExtent);
	}
	
	// We have 1 default non-mesh head and 16 non-mesh heads that you can buy
	// All future heads will be mesh heads; but we have to support legacy heads as well
	// They were baked once into mesh files (with decal UVs and some vertex welding).
	
	// Here's a table of cylinder head meshes: id, bevel, bevel roundness, bulge, LODx, LODy, offset, scale
	// A		0		0	0.5		1	2	0,0.1,0		1.25,1.25,1.25
	// C		0.1		0	0.5		1	2	0,0.1,0		1.25,1.25,1.25
	// D		0.4		0	0		2	2	0,0.05,0	1.4,1.4,1.35
	// E		0.66	0	0.5		2	2	0,0.05,0	1.4,1.6,1.35
	// F		0		0	0		1	0	0,0.1,0		1.25,1.25,1.25
	// G		0		0	1		2	2	0,0.1,0		1.25,1.25,1.25
	// H		0.2		0	1		2	2	0,0.1,0		1.3,1.3,1.3
	// N		0.4		1	0		2	2	0,0.05,0	1.4,1.4,1.35
	// O		0.2		0	0.5		1	2	0,0.1,0		1.3,1.3,1.3
	// P		0.1		0	0		2	2	0,0.1,0		1.25,1.25,1.25
	
	// Here's a table of block head meshes: id, bevel, bevel roundness, bulge, LODx, LODy, offset, scale
	// B		0		0	0		2	2	0,0.05,0	0.56,1.1,1.1
	// I		0.5		0	0		2	2	0,0.05,0	0.8,1.3,1.2
	// J		0.3		0	0		2	2	0,0.1,0		0.63,1.3,1.1
	// M		0.05	0	0		2	2	0,0.05,0	0.56,1.1,1.1
	static MeshId getHeadMeshId(PartInstance* part, DataModelMesh* specialShape)
	{
		// Note that bevel, roundness and bulge are unique enough to determine the cylinder/block variant, so we'll use just that
	#define LOOKUP(ifBevel, ifRoundness, ifBulge, head) \
		if (G3D::fuzzyEq(bevel, ifBevel) && G3D::fuzzyEq(roundness, ifRoundness) && G3D::fuzzyEq(bulge, ifBulge)) return MeshId("rbxasset://fonts/head" head ".mesh")
	
		if (specialShape)
		{
			if (SpecialShape* shape = specialShape->fastDynamicCast<SpecialShape>())
			{
				if (shape->getMeshType() == SpecialShape::HEAD_MESH)
				{
					// Return the default head; scaling is taken care of in getMeshScale
					return MeshId("rbxasset://fonts/head.mesh");
				}
				else if (shape->getMeshType() == SpecialShape::SPHERE_MESH)
				{
					// Return the sphere head; scaling is taken care of in getMeshScale
					return MeshId("rbxasset://fonts/headL.mesh");
				}
			}
			else if (CylinderMesh* shape = specialShape->fastDynamicCast<CylinderMesh>())
			{
				float bevel = shape->getBevel(), roundness = shape->getRoundness(), bulge = shape->getBulge();
				
				LOOKUP(0.0,		0.0,		0.5,	"A");
				LOOKUP(0.1,		0.0,		0.5,	"C");
				LOOKUP(0.4,		0.0,		0.0,	"D");
				LOOKUP(0.66,	0.0,		0.5,	"E");
				LOOKUP(0.0,		0.0,		0.0,	"F");
				LOOKUP(0.0,		0.0,		1.0,	"G");
				LOOKUP(0.2,		0.0,		1.0,	"H");
				LOOKUP(0.4,		1.0,		0.0,	"N");
				LOOKUP(0.2,		0.0,		0.5,	"O");
				LOOKUP(0.1,		0.0,		0.0,	"P");
			}
			else if (BlockMesh* shape = specialShape->fastDynamicCast<BlockMesh>())
			{
				float bevel = shape->getBevel(), roundness = shape->getRoundness(), bulge = shape->getBulge();
				
				LOOKUP(0.0,		0.0,		0.0,	"B");
				LOOKUP(0.5,		0.0,		0.0,	"I");
				LOOKUP(0.3,		0.0,		0.0,	"J");
				LOOKUP(0.05,	0.0,		0.0,	"M");
			}
		}
		
		// fallback for all other head types
		return MeshId("rbxasset://fonts/head.mesh");
		
	#undef LOOKUP
	}
	
	static MeshId getMeshIdForBodyPart(const HumanoidIdentifier& hi, PartInstance* part, DataModelMesh* specialShape, unsigned int flags)
	{
		if (CharacterMesh* charmesh = hi.getRelevantMesh(part))
			return charmesh->getMeshId();
		else if (part == hi.torso && (flags & GeometryGenerator::Resource_SubstituteBodyParts))
			return MeshId("rbxasset://fonts/torso.mesh");
		else if (part == hi.leftArm && (flags & GeometryGenerator::Resource_SubstituteBodyParts))
			return MeshId("rbxasset://fonts/leftarm.mesh");
		else if (part == hi.rightArm && (flags & GeometryGenerator::Resource_SubstituteBodyParts))
			return MeshId("rbxasset://fonts/rightarm.mesh");
		else if (part == hi.leftLeg && (flags & GeometryGenerator::Resource_SubstituteBodyParts))
			return MeshId("rbxasset://fonts/leftleg.mesh");
		else if (part == hi.rightLeg && (flags & GeometryGenerator::Resource_SubstituteBodyParts))
			return MeshId("rbxasset://fonts/rightleg.mesh");
		else if (part == hi.head && (flags & GeometryGenerator::Resource_SubstituteBodyParts))
			return getHeadMeshId(part, specialShape);
		else
			return MeshId();
	}
	
	static float getPartTransparency(PartInstance* part)
	{
		// Reflectance affects Transparency for plastic parts - part with Transparency at 0.99 and Reflectance at 1 should still be somewhat visible
		return (part->getRenderMaterial() == PLASTIC_MATERIAL)
			? part->getTransparencyUi() * (1 - getPartReflectance(part))
			: part->getTransparencyUi();
	}
	
	static Color4uint8 getBaseColor(PartInstance* part, Decal* decal, DataModelMesh* specialShape, const GeometryGenerator::Options& options)
	{
		float transparency = decal ? decal->getTransparencyUi() : getPartTransparency(part);
		unsigned char alpha = 255 - static_cast<int>(G3D::clamp(transparency, 0.f, 1.f) * 255.f);
		
		// Decal never takes part/mesh colors
		if (decal)
			return Color4uint8(255, 255, 255, alpha);
		
		// Requested colors from mesh
		if (options.flags & GeometryGenerator::Flag_VertexColorMesh)
		{
			FileMesh* fileMesh = Instance::fastDynamicCast<FileMesh>(specialShape);
			
			if (fileMesh && !fileMesh->getTextureId().isNull())
				return Color4uint8(Color3uint8(Color3(specialShape->getVertColor())), alpha);
			else
				return Color4uint8(255, 255, 255, alpha);
		}
			
		// Requested colors from part
		if (options.flags & GeometryGenerator::Flag_VertexColorPart)
			return Color4uint8(part->getColor().color3uint8(), alpha);
			
		return Color4uint8(255, 255, 255, alpha);
	}

	static Color4uint8 getColor(PartInstance* part, Decal* decal, DataModelMesh* specialShape, const GeometryGenerator::Options& options, unsigned int randomSeed, bool isBlock)
	{
		Color4uint8 diffuse = getBaseColor(part, decal, specialShape, options);
		
		Color4uint8 color =
			(isBlock && (options.flags & GeometryGenerator::Flag_RandomizeBlockAppearance))
				? Color4uint8(
					(diffuse.r & 0xf8) | (randomSeed & 7),
					(diffuse.g & 0xf8) | (randomSeed & 7),
					(diffuse.b & 0xf8) | (randomSeed & 7),
					diffuse.a)
				: diffuse;

		return
			(options.flags & GeometryGenerator::Flag_DiffuseColorBGRA)
				? Color4uint8(color.b, color.g, color.r, color.a)
				: color;
	}
	
	static Color4uint8 getExtra(PartInstance* part, const GeometryGenerator::Options& options)
	{
		Vector2int16 specular = MaterialGenerator::getSpecular(part->getRenderMaterial());
        int reflectance = getPartReflectance(part) * 255;
		
		return
			(options.flags & GeometryGenerator::Flag_ExtraIsZero)
			? Color4uint8(0, 0, 0, 0)
			: Color4uint8(options.extra, specular.x, specular.y, reflectance);
	}

	static Vector3 getMeshScale(PartInstance* part, DataModelMesh* specialShape, const HumanoidIdentifier* humanoidIdentifier)
	{
    	if (specialShape)
		{
			// File meshes don't inherit part size and just use custom scale
			if (part->getCookie() & PartCookie::HAS_FILEMESH)
				return specialShape->getScale();
				
			// If it's not a file mesh, it must be a substituted body part.
			// These have a constant scale with the exception of the head mesh and sphere mesh
			if (SpecialShape* shape = specialShape->fastDynamicCast<SpecialShape>())
			{
				if (shape->getMeshType() == SpecialShape::HEAD_MESH)
				{
					// For heads, the scale works separately for XZ and Y
					Vector3 size = part->getPartSizeXml() * shape->getScale();
					float scaleXZ = std::min(size.x, size.z);
					float scaleY = size.y;
					
					// Default head is baked with uniform scale 1.25
					return Vector3(scaleXZ, scaleY, scaleXZ) / 1.25f;
				}
				else if (shape->getMeshType() == SpecialShape::SPHERE_MESH)
				{
					Vector3 size = part->getPartSizeXml() * shape->getScale();
					
					// Default head is baked with scale 1.5 1.45 1.45
					return size / Vector3(1.5f, 1.45f, 1.45f);
				}
			}
		}
		
        if (humanoidIdentifier && humanoidIdentifier->getBodyPartType(part) != HumanoidIdentifier::PartType_Unknown)
        {
            if (humanoidIdentifier->isPartHead(part))
            {
                Vector3 size = part->getPartSizeXml();
                float scaleXZ = std::min(size.x, size.z);
                return Vector3(scaleXZ, size.y, scaleXZ);
            }
            else
            {
                Vector3 standardPartSize = humanoidIdentifier->getBodyPartScale(part);
                Vector3 partSize = part->getPartSizeXml();
                return partSize / standardPartSize;
            }
        }

		return Vector3::one();
	}

	template<bool vertical> Vector3 verticalRotate(const Vector3& pos)
	{
		return vertical ? Vector3(pos.y, -pos.x, pos.z) : pos;
	}

	template<bool vertical> Vector2 verticalRotateDecal(const Vector2& pos)
	{
		return vertical ? Vector2(-pos.y, pos.x) : pos;
	}

	template<bool vertical> NormalId verticalRotate(NormalId id) {
		if (!vertical)
			return id;

		switch(id)
		{
		case NORM_X: return NORM_Y;
		case NORM_Y: return NORM_X_NEG;
		case NORM_X_NEG: return NORM_Y_NEG;
		case NORM_Y_NEG: return NORM_X;
		default:
			return id;
		}
	}


	template<bool vertical> Vector2 getDecalUVVertical(Decal* decal, const Vector3& size, bool ignoreSurfaceType)	
	{
		if (!ignoreSurfaceType)
		{
			if (DecalTexture* texture = decal->fastDynamicCast<DecalTexture>())
			{
				switch (verticalRotate<vertical>(texture->getFace()))
				{
				case NORM_X: return Vector2(size.z, size.y) / texture->getStudsPerTile();
				case NORM_Y: return Vector2(-size.x, size.z) / texture->getStudsPerTile();
				case NORM_Z: return Vector2(size.x, size.y) / texture->getStudsPerTile();
				
				case NORM_X_NEG: return Vector2(-size.z, size.y) / texture->getStudsPerTile();
				case NORM_Y_NEG: return Vector2(size.x, -size.z) / texture->getStudsPerTile();
				case NORM_Z_NEG: return Vector2(-size.x, size.y) / texture->getStudsPerTile();
                        
                default: return Vector2();
				}
			}
		}
		
		return Vector2(1.0f, 1.0f);
	}

	inline Vector2 getDecalUV(Decal* decal, const Vector3& size, bool ignoreSurfaceType)
	{
		return getDecalUVVertical<false>(decal, size, ignoreSurfaceType);
	}

	static Vector2 getSurfaceOffset(const Vector3& size, unsigned int randomSeed, PartMaterial material)
	{
        int randomU = (randomSeed >> 3) & 63;
        int randomV = (randomSeed >> 9) & 63;

        if (material == BRICK_MATERIAL || material == WOODPLANKS_MATERIAL)
        {
            // We want the stud-aligned bricks to roughly match between parts, so we're only shifting the tiling in 1 stud increments
            return Vector2(randomU, randomV);
        }
        else
        {
            float maxOffset = (size.x + size.y + size.z) / 2;
            
            return maxOffset * Vector2(kUniformRandomTable[randomU] * 2 - 1, kUniformRandomTable[randomV] * 2 - 1);
        }
	}

    static float getSurfaceTiling(PartMaterial material)
    {
        return MaterialGenerator::getTiling(material);
    } 

	static Vector4 computeOutlineOffsetPart(NormalId face, PartInstance* part)
	{
		if(part->getSurfaceType(face) == NO_SURFACE_NO_OUTLINES)
			return Vector4(NO_OUTLINES, NO_OUTLINES, NO_OUTLINES, NO_OUTLINES);


		static const NormalId adjTable [6][4] = {
			{ NORM_Z_NEG, NORM_Z, NORM_Y, NORM_Y_NEG }, // NormX
			{ NORM_X_NEG, NORM_X, NORM_Z, NORM_Z_NEG }, // NormY
			{ NORM_X, NORM_X_NEG, NORM_Y, NORM_Y_NEG }, // NormZ
			{ NORM_Z, NORM_Z_NEG, NORM_Y, NORM_Y_NEG }, // NormX_Neg
			{ NORM_X_NEG, NORM_X, NORM_Z_NEG, NORM_Z }, // NormY_Neg
			{ NORM_X_NEG, NORM_X, NORM_Y, NORM_Y_NEG }, // NormZ_Neg
		};

		Vector4 result;

		result.x = part->getSurfaceType(adjTable[face][0]) == NO_SURFACE_NO_OUTLINES ? NO_OUTLINES : 0;
		result.y = part->getSurfaceType(adjTable[face][1]) == NO_SURFACE_NO_OUTLINES ? NO_OUTLINES : 0;
		result.z = part->getSurfaceType(adjTable[face][2]) == NO_SURFACE_NO_OUTLINES ? NO_OUTLINES : 0;
		result.w = part->getSurfaceType(adjTable[face][3]) == NO_SURFACE_NO_OUTLINES ? NO_OUTLINES : 0;

		return result;
	}

	static Vector4 computeOutlineOffsetWedge(NormalId face, PartInstance* part)
	{
		if(part->getSurfaceType(face) == NO_SURFACE_NO_OUTLINES)
			return Vector4(NO_OUTLINES, NO_OUTLINES, NO_OUTLINES, NO_OUTLINES);


		static const NormalId adjTable [6][4] = {
			{ NORM_Z, NORM_Z_NEG, NORM_Z_NEG, NORM_Y_NEG }, // NormX
			{ NORM_Y, NORM_Y, NORM_Y, NORM_Y },				// NormY
			{ NORM_X, NORM_X_NEG, NORM_Z_NEG, NORM_Y_NEG }, // NormZ
			{ NORM_Z, NORM_Z_NEG, NORM_Z_NEG, NORM_Y_NEG }, // NormX_Neg
			{ NORM_X_NEG, NORM_X, NORM_Z_NEG, NORM_Z },		// NormY_Neg
			{ NORM_X_NEG, NORM_X, NORM_Z, NORM_Y_NEG },		// NormZ_Neg
		};

		Vector4 result;

		result.x = part->getSurfaceType(adjTable[face][0]) == NO_SURFACE_NO_OUTLINES ? NO_OUTLINES : 0;
		result.y = part->getSurfaceType(adjTable[face][1]) == NO_SURFACE_NO_OUTLINES ? NO_OUTLINES : 0;
		result.z = part->getSurfaceType(adjTable[face][2]) == NO_SURFACE_NO_OUTLINES ? NO_OUTLINES : 0;
		result.w = part->getSurfaceType(adjTable[face][3]) == NO_SURFACE_NO_OUTLINES ? NO_OUTLINES : 0;

		return result;
	}

	static Vector4 computeOutlineOffsetCornerWedge(NormalId face, PartInstance* part)
	{
		if(part->getSurfaceType(face) == NO_SURFACE_NO_OUTLINES)
			return Vector4(NO_OUTLINES, NO_OUTLINES, NO_OUTLINES, NO_OUTLINES);

		static const NormalId adjTable [6][4] = {
			{ NORM_Z_NEG, NORM_Z, NORM_Z_NEG, NORM_Y_NEG }, // NormX
			{ NORM_Y, NORM_Y, NORM_Y, NORM_Y },				// NormY
			{ NORM_X, NORM_X_NEG, NORM_X_NEG, NORM_Y_NEG },	// NormZ
			{ NORM_Z_NEG, NORM_Z, NORM_Z_NEG, NORM_Y_NEG }, // NormX_Neg
			{ NORM_X_NEG, NORM_X, NORM_Z_NEG, NORM_Z },		// NormY_Neg
			{ NORM_X_NEG, NORM_X, NORM_X_NEG, NORM_Y_NEG }, // NormZ_Neg
		};

		Vector4 result;

		result.x = part->getSurfaceType(adjTable[face][0]) == NO_SURFACE_NO_OUTLINES ? NO_OUTLINES : 0;
		result.y = part->getSurfaceType(adjTable[face][1]) == NO_SURFACE_NO_OUTLINES ? NO_OUTLINES : 0;
		result.z = part->getSurfaceType(adjTable[face][2]) == NO_SURFACE_NO_OUTLINES ? NO_OUTLINES : 0;
		result.w = part->getSurfaceType(adjTable[face][3]) == NO_SURFACE_NO_OUTLINES ? NO_OUTLINES : 0;

		return result;
	}
	
	void GeometryGenerator::addFileMesh(FileMeshData* data, DataModelMesh* specialShape, PartInstance* part, Decal* decal, const HumanoidIdentifier* humanoidIdentifier, const Options& options)
	{
		size_t vertexCount = data->vnts.size();
		size_t faceCount = data->faces.size();

		size_t vertexOffset = mVertexCount;
		size_t indexOffset = mIndexCount;
		
		mVertexCount += vertexCount;
		mIndexCount += faceCount * 3;
		
		if (!mVertices) return;

		Color4uint8 color = getColor(part, decal, specialShape, options, 0, false);
		Color4uint8 extra = getExtra(part, options);
		Vector3 scale = getMeshScale(part, specialShape, humanoidIdentifier);

		const CoordinateFrame& cframe = options.cframe;
		Vertex* vertices = mVertices;
        Vector3 offset = specialShape ? specialShape->getOffset() : Vector3::zero();

		SpecialShape* shape = Instance::fastDynamicCast<SpecialShape>(specialShape);
		bool isHead = shape && shape->getMeshType() == SpecialShape::HEAD_MESH;

		if (isHead)
		{
			// Head mesh scale is not uniform - XZ scale scales the caps (beveled portion), and Y scale just extrudes the cylinder portion
			// Since head mesh is centered around origin, with 0.625 as Y extents (maxY ~= 0.625, minY ~= -0.625), we scale vertices by scale.x
			// along Y axis and extrude by headCylinderExtents, that is set to keep the cumulative extents at 0.625 * scale.y
			float headCylinderExtents = 0.625 * (scale.y - scale.x);
			
			for (size_t i = 0; i < vertexCount; ++i)
			{
				Vertex& v = vertices[vertexOffset + i];
				const FileMeshVertexNormalTexture3d& sv = data->vnts[i];
				
				float headY = (sv.vy > 0) ? (sv.vy * scale.x + headCylinderExtents) : (sv.vy * scale.x - headCylinderExtents);
					
                Vector3 pos;
                if (FFlag::FixMeshOffset)
				    pos = cframe.pointToWorldSpace(Vector3(sv.vx * scale.x, headY, sv.vz * scale.z) + offset);
                else
                    pos = cframe.pointToWorldSpace(Vector3(sv.vx * scale.x, headY, sv.vz * scale.z)) + offset;
				
				fillVertex(v, pos, cframe.vectorToWorldSpace(Vector3(sv.nx, sv.ny, sv.nz)), 
					transformUV(Vector2(sv.tu, sv.tv), options), Vector2(0.0f,0.0f),
					color, extra, Vector3());
			}
		}
		else
		{
			for (size_t i = 0; i < vertexCount; ++i)
			{
				Vertex& v = vertices[vertexOffset + i];
				const FileMeshVertexNormalTexture3d& sv = data->vnts[i];
				
                Vector3 pos;
                if (FFlag::FixMeshOffset)
                    pos = cframe.pointToWorldSpace(Vector3(sv.vx * scale.x, sv.vy * scale.y, sv.vz * scale.z) + offset);
                else
                    pos = cframe.pointToWorldSpace(Vector3(sv.vx * scale.x, sv.vy * scale.y, sv.vz * scale.z)) + offset;
				
				fillVertex(v, pos, cframe.vectorToWorldSpace(Vector3(sv.nx, sv.ny, sv.nz)), 
					transformUV(Vector2(sv.tu, sv.tv), options), Vector2(0.0f,0.0f), 
					color, extra, Vector3());
			}
		}
		
		if (vertexCount > 0)
		{
			AABox aabb = cframe.AABBtoWorldSpace(AABox(data->aabb.low() * scale + offset, data->aabb.high() * scale + offset));
					
			extendBounds(mBboxMin, mBboxMax, aabb.low());
			extendBounds(mBboxMin, mBboxMax, aabb.high());
		}
		
		unsigned short* indices = mIndices;
		
		for (size_t i = 0; i < faceCount; ++i)
		{
			indices[indexOffset + i * 3 + 0] = vertexOffset + data->faces[i].a;
			indices[indexOffset + i * 3 + 1] = vertexOffset + data->faces[i].b;
			indices[indexOffset + i * 3 + 2] = vertexOffset + data->faces[i].c;
		}
	}

	bool isPartSmooth(PartInstance* p)
	{
		for(unsigned normal = 0; normal < NORM_UNDEFINED; normal++)
			if(p->getSurfaceType((NormalId)normal) != NO_SURFACE)
				return false;

		return true;
	}

	Vector3 computeCylinderPosition(const Vector3& localUnitPos, const Vector3& size, float radius, Vector2& normal2d)
	{
		normal2d = Vector2(localUnitPos.y, localUnitPos.z);
		normal2d = normal2d.isZero() ? normal2d : normal2d.direction();
		Vector3 localPos = Vector3(localUnitPos.x*size.x/2.0f,normal2d.x*radius,normal2d.y*radius);
		return localPos;
	}


	template<bool vertical> void GeometryGenerator::addCylinder(const Vector3& size, const Vector3& center, PartInstance* part, Decal* decal, const Options& options, unsigned int randomSeed, bool ignoreMaterialsStuds)
	{
		size_t vertexOffset = mVertexCount;
		size_t indexOffset = mIndexCount;
		bool isSmooth = isPartSmooth(part) || ignoreMaterialsStuds;

		Vector3 genSize = vertical ? size.yxz() : size;

		float primaryDimension = std::min(genSize.y, genSize.z);

		float primaryDimensionStuds = std::min(primaryDimension, MAX_TESSELATION*(TESSELATION_PIECE-1));
		
		unsigned minimumTesselationFactor = 6;

		float quadNumberF = isSmooth ? 1 : ceil(primaryDimensionStuds / (TESSELATION_PIECE-1));

		unsigned quadNumberI = (unsigned)quadNumberF;
		
		if (quadNumberI == 0) return;
		
		unsigned tesselationPerQuad = (minimumTesselationFactor + quadNumberI - 1) / quadNumberI;
		float invTesselationFactor = 1.0f / (quadNumberF*tesselationPerQuad);
		unsigned tesselationFactor = quadNumberI*tesselationPerQuad;

		unsigned totalVerts = 4*quadNumberI*2*(tesselationPerQuad+1) + 2*(1 + (tesselationFactor+1)*4);
		unsigned quads = 4*tesselationFactor;
		unsigned tris = 4*tesselationFactor*2;

		if(decal) {
			if(verticalRotate<vertical>(decal->getFace()) % 3 == 0) {
				// Cap decal
				totalVerts = (1 + (tesselationFactor+1)*4);
				quads = 0;
				tris = 4*tesselationFactor;
			}
			else
			{
				// Side decal
				totalVerts = quadNumberI*2*(tesselationPerQuad+1);
				quads = tesselationFactor;
				tris = 0;
			}
		}

		mVertexCount += totalVerts;
		mIndexCount += quads*6 + tris*3;

		if (!mVertices) return;

		Color4uint8 color = getColor(part, decal, NULL, options, randomSeed, true);
		Color4uint8 extra = getExtra(part, options);

		float radius = primaryDimension/2;

		Vector3 axisX = Vector3(1.0f, 0.0f, 0.0f);
		Vector3 axisY = Vector3(0.0f, 1.0f, 0.0f);
		Vector3 axisZ = Vector3(0.0f, 0.0f, 1.0f);

		Vector3 worldAxisX = Vector3(options.cframe.rotation[0][0], options.cframe.rotation[1][0], options.cframe.rotation[2][0]);
		Vector3 worldAxisY = Vector3(options.cframe.rotation[0][1], options.cframe.rotation[1][1], options.cframe.rotation[2][1]);
		Vector3 worldAxisZ = Vector3(options.cframe.rotation[0][2], options.cframe.rotation[1][2], options.cframe.rotation[2][2]);

		Vector3 cornerO = -axisX - axisY - axisZ;
		Vector3 cornerX = axisX * 2;
		Vector3 cornerY = axisY * 2;
		Vector3 cornerZ = axisZ * 2;

		Vector2 surfaceOffset = decal ? Vector2() : getSurfaceOffset(genSize, randomSeed, part->getRenderMaterial());
        float surfaceTiling = decal ? 1 : getSurfaceTiling(part->getRenderMaterial());

		Vector3 corners[8] =
		{
			cornerO,
			cornerO + cornerZ,
			cornerO + cornerY,
			cornerO + cornerY + cornerZ,
			cornerO + cornerX,
			cornerO + cornerX + cornerZ,
			cornerO + cornerX + cornerY,
			cornerO + cornerX + cornerY + cornerZ,
		};

		Vector3 facesDirU[] =
		{
			-axisY,
			-axisZ,
			-axisY,
			-axisY,
			axisZ,
			-axisY
		};

		Vector3 facesDirV[] =
		{
			axisZ,
			axisX,
			-axisX,
			-axisZ,
			axisX,
			axisX
		};

		Vector3 tangents[6];
		
		if (vertical)
		{
			tangents[0] = worldAxisX;
			tangents[1] = worldAxisY;
			tangents[2] = -worldAxisY;
			tangents[3] = -worldAxisX;
			tangents[4] = worldAxisY;
			tangents[5] = worldAxisY;
		}
		else
		{
			tangents[0] = -worldAxisZ;
			tangents[1] = -worldAxisX;
			tangents[2] = worldAxisX;
			tangents[3] = worldAxisZ;
			tangents[4] = -worldAxisX;
			tangents[5] = -worldAxisX;
		}

		unsigned vertexCounter = 0;
		unsigned faceCounter = 0;
		Vertex* vertices = mVertices;
		unsigned short* indices = mIndices;

		struct CAPDESC
		{
			Vector3 start[4];
			Vector3 direction[4];
			Vector3 fanPoint;
			Vector2 normal2dToUV;
		};


		CAPDESC capsDesc[] = 
		{
			{
				{ corners[6], corners[4], corners[5], corners[7] },
				{ -axisY, axisZ, axisY, -axisZ },
				Vector3(1,0,0),
				Vector2(-1,-1),
			},
			{
				{ corners[3], corners[1], corners[0], corners[2] },
				{ -axisY, -axisZ, axisY, axisZ },
				Vector3(-1,0,0),
				Vector2(1,-1),
			}
		};

		static const FACEDESC facesDesc[] =
		{
			{ 6,7,4,5, false, true, }, // Face 0
			{ 3,7,2,6, true, false, }, // Face 1
			{ 7,3,5,1, false, false, }, // Face 2
			{ 3,2,1,0, false, true, }, // Face 3
			{ 0,4,1,5, true, false, }, // Face 4
			{ 2,6,0,4, false, false, }, // Face 5
		};

		float capRightOutlineOffset = (part->getConstPartPrimitive()->getSurfaceType(NORM_X) == NO_SURFACE_NO_OUTLINES) ? NO_OUTLINES : 0;
		float capLeftOutlineOffset = (part->getConstPartPrimitive()->getSurfaceType(NORM_X_NEG) == NO_SURFACE_NO_OUTLINES) ? NO_OUTLINES : 0;


		for (int face = 0; face < 6; ++face)
		{
			if(decal && verticalRotate<vertical>(decal->getFace()) != face)
				continue;

			// Caps
			if(face % 3 == 0)
			{
				const CAPDESC& capDesc = capsDesc[face == 0 ? 0 : 1];
				Vector3 fanPoint = capDesc.fanPoint;

				unsigned fanPointIndex = vertexOffset+vertexCounter;

				Vector2 mainMultiplier = decal ? getDecalUVVertical<vertical>(decal, genSize, ignoreMaterialsStuds) : Vector2(primaryDimension,primaryDimension);
				Vector2 normal2d;
				Vector3 localPos = computeCylinderPosition(fanPoint, genSize, radius, normal2d);
				Vector3 outPos = options.cframe.pointToWorldSpace(verticalRotate<vertical>(localPos) + center);
				Vector3 normal = options.cframe.vectorToWorldSpace(verticalRotate<vertical>(fanPoint));

				Vector2 mainUV = 0.5f * mainMultiplier;

				float outlineOffset = face == 0 ? capRightOutlineOffset : capLeftOutlineOffset;
				fillVertex(vertices[vertexOffset + vertexCounter], outPos, normal, (mainUV + surfaceOffset) * surfaceTiling, Vector2::zero(), 
					color, extra,
					tangents[face], Vector4(NO_OUTLINES,NO_OUTLINES, NO_OUTLINES, radius + outlineOffset));
				vertexCounter++;

				unsigned prevVertexCounter = vertexCounter;
				for(unsigned adjFace = 0; adjFace < 4; adjFace++)
				{
					Vector3 unitPos = capDesc.start[adjFace];
					Vector3 unitDirV = capDesc.direction[adjFace]*2*invTesselationFactor;
					for(unsigned tess = 0; tess < tesselationFactor+1; tess++) 
					{
						Vector2 normal2d;
						Vector3 localPos = computeCylinderPosition(unitPos, genSize, radius, normal2d);
						Vector3 outPos = options.cframe.pointToWorldSpace(verticalRotate<vertical>(localPos) + center);

						Vector2 mainUV = (verticalRotateDecal<vertical>(normal2d.yx())*capDesc.normal2dToUV +Vector2(1,1))*0.5f * mainMultiplier;

						Vector3 normal = options.cframe.vectorToWorldSpace(verticalRotate<vertical>(fanPoint));
						fillVertex(vertices[vertexOffset + vertexCounter], outPos, normal, (mainUV + surfaceOffset) * surfaceTiling, Vector2::zero(), 
							color, extra,
							tangents[face], Vector4(NO_OUTLINES,NO_OUTLINES, NO_OUTLINES, outlineOffset));

						vertexCounter++;
						unitPos += unitDirV;
					}
				}
				unsigned tessIndex = vertexOffset+prevVertexCounter;
				for(unsigned adjFace = 0; adjFace < 4; adjFace++)
				{
					for(unsigned tess = 0; tess < tesselationFactor; tess++)
					{
						indices[indexOffset + faceCounter*3] = fanPointIndex;
						indices[indexOffset + faceCounter*3+1] = tessIndex+tess+1;
						indices[indexOffset + faceCounter*3+2] = tessIndex+tess;
						faceCounter++;
					}
					tessIndex += tesselationFactor+1;
				}
			}
			else 
			{
				const FACEDESC& desc = facesDesc[face];

				Vector3 start0 = corners[desc.start0]; //, start1 = corners[desc.start1];

				Vector3 unitDirU = facesDirU[face] * invTesselationFactor * 2;
				Vector3 unitDirV = facesDirV[face] * 2;

				Vector3 unitPos = start0;

				UVAtlasInfo studsAtlasInfo = getStudsAtlasInfo((ignoreMaterialsStuds || (options.flags & Flag_IgnoreStuds)) ? NO_SURFACE : part->getConstPartPrimitive()->getSurfaceType((NormalId)face));

				float studOffsetU = studsAtlasInfo.tileX ? genSize.x / 2.0f : 0.0f;
				float studOffsetV = studsAtlasInfo.scaleY * primaryDimensionStuds * invTesselationFactor / TESSELATION_PIECE;

				Vector2 mainOffsetOrig = decal ? getDecalUVVertical<vertical>(decal, genSize, ignoreMaterialsStuds) : Vector2(genSize.x,primaryDimension);
				Vector2 mainCoords = Vector2(mainOffsetOrig.x,0.0f) + surfaceOffset;
				Vector2 mainOffset = mainOffsetOrig*Vector2(1,invTesselationFactor);

				for (unsigned quad = 0; quad < quadNumberI; quad++) {
					unsigned prevVertexCounter = vertexCounter;
					Vector2 studStart = Vector2(studOffsetU, studsAtlasInfo.offsetY + 
						frac(quad*primaryDimensionStuds / quadNumberF) / TESSELATION_PIECE * studsAtlasInfo.scaleY);

					// Generate vertices
					for(unsigned iU = 0; iU < tesselationPerQuad+1; iU++) 
					{
						Vector3 unitPosV = unitPos;
						Vector2 studsV = studStart;
						Vector2 mainCoordsV = mainCoords;

						for(unsigned iV = 0; iV < 2; iV++) 
						{
							Vector2 normal2d;
							Vector3 localPos = computeCylinderPosition(unitPosV, genSize, radius, normal2d);
							Vector3 outPos = options.cframe.pointToWorldSpace(verticalRotate<vertical>(localPos) + center);

							Vector3 worldNormal = options.cframe.vectorToWorldSpace(verticalRotate<vertical>(Vector3(0, normal2d.x, normal2d.y)));

							Vector2 mainUV = mainCoordsV;
							if(decal && vertical) {
								mainUV = Vector2(mainUV.y, mainOffsetOrig.y - mainUV.x);
							}
							Vector4 edgeDistances = Vector4(NO_OUTLINES,NO_OUTLINES, 
								capLeftOutlineOffset + (iV == 0 ? 0 : genSize.x), capRightOutlineOffset + (iV == 0 ? genSize.x : 0));

							fillVertex(vertices[vertexOffset + vertexCounter], outPos, worldNormal, mainUV * surfaceTiling, studsV, 
								color, extra,
								tangents[face], edgeDistances);

							vertexCounter++;
							unitPosV += unitDirV;
							studsV.x -= studOffsetU;
							mainCoordsV.x -= mainOffset.x;
						}
						if(iU < tesselationPerQuad) {
							unitPos += unitDirU;
							mainCoords.y += mainOffset.y;
						}

						studStart.y += studOffsetV;
					}

					// Generate faces
					for(unsigned iU = 0; iU < tesselationPerQuad; iU++) 
					{
						fillQuadIndices(&indices[indexOffset + faceCounter*3], vertexOffset + prevVertexCounter, 
							iU*2, 
							iU*2+1,
							(iU+1)*2,
							(iU+1)*2+1);
						faceCounter+=2;
					}
				}
			}
		}

		RBXASSERT(faceCounter == 2*quads+tris);
		RBXASSERT(vertexCounter == totalVerts);

		Vector3 boundsCenter = options.cframe.pointToWorldSpace(center);
		extendBoundsBlock(mBboxMin, mBboxMax, boundsCenter, size, worldAxisX, worldAxisY, worldAxisZ);
	}

	void GeometryGenerator::addSphere(const Vector3& size, const Vector3& center, PartInstance* part, Decal* decal, const Options& options, unsigned int randomSeed, bool ignoreMaterialsStuds) 
	{
		size_t vertexOffset = mVertexCount;
		size_t indexOffset = mIndexCount;

		bool isSmooth = isPartSmooth(part) || ignoreMaterialsStuds;

		float primaryDimension = size.min();
		float primaryDimensionStuds = std::min(primaryDimension, MAX_TESSELATION*(TESSELATION_PIECE-1));

		unsigned minimumTesselationFactor = 6;
		float quadNumberF = isSmooth ? 1 : ceil(primaryDimensionStuds / (TESSELATION_PIECE-1));

		unsigned quadNumberI = (unsigned)quadNumberF;

		if (quadNumberI == 0) return;
		
		unsigned tesselationPerQuad = (minimumTesselationFactor + quadNumberI - 1) / quadNumberI;
		float invTesselationFactor = 1.0f / (quadNumberF*tesselationPerQuad);
		unsigned tesselationFactor = quadNumberI*tesselationPerQuad;

		unsigned totalVerts = quadNumberI*(tesselationFactor+1)*(tesselationPerQuad+1);
		unsigned quads = tesselationFactor*tesselationFactor;

		if(!decal)
		{
			totalVerts *= 6;
			quads *= 6;
		}

		mVertexCount += totalVerts;
		mIndexCount += quads*6;

		if (!mVertices) return;

		Color4uint8 color = getColor(part, decal, NULL, options, randomSeed, true);
		Color4uint8 extra = getExtra(part, options);

		float radius = primaryDimension/2;

		static const Vector3 axisX = Vector3(1.0f, 0.0f, 0.0f);
		static const Vector3 axisY = Vector3(0.0f, 1.0f, 0.0f);
		static const Vector3 axisZ = Vector3(0.0f, 0.0f, 1.0f);

		static const Vector3 cornerO = -axisX - axisY - axisZ;
		static const Vector3 cornerX = axisX * 2;
		static const Vector3 cornerY = axisY * 2;
		static const Vector3 cornerZ = axisZ * 2;

		static const Vector3 corners[8] =
		{
			cornerO,
			cornerO + cornerZ,
			cornerO + cornerY,
			cornerO + cornerY + cornerZ,
			cornerO + cornerX,
			cornerO + cornerX + cornerZ,
			cornerO + cornerX + cornerY,
			cornerO + cornerX + cornerY + cornerZ,
		};

		static const Vector3 facesDirU[] =
		{
			-axisY,
			-axisZ,
			-axisY,
			-axisY,
			axisZ,
			-axisY
		};

		static const Vector3 facesDirV[] =
		{
			axisZ,
			axisX,
			-axisX,
			-axisZ,
			axisX,
			axisX
		};

		Vector3 worldAxisX = Vector3(options.cframe.rotation[0][0], options.cframe.rotation[1][0], options.cframe.rotation[2][0]);
		Vector3 worldAxisY = Vector3(options.cframe.rotation[0][1], options.cframe.rotation[1][1], options.cframe.rotation[2][1]);
		Vector3 worldAxisZ = Vector3(options.cframe.rotation[0][2], options.cframe.rotation[1][2], options.cframe.rotation[2][2]);

		Vector2 surfaceOffset = decal ? Vector2() : getSurfaceOffset(size, randomSeed, part->getRenderMaterial());
        float surfaceTiling = decal ? 1 : getSurfaceTiling(part->getRenderMaterial());

		Vector3 tangents[] =
		{
			worldAxisZ,
			worldAxisX,
			-worldAxisX,
			-worldAxisZ,
			worldAxisX,
			worldAxisX
		};

		static const FACEDESC facesDesc[] =
		{
			{ 6,7,4,5, false, true, }, // Face 0
			{ 3,7,2,6, true, false, }, // Face 1
			{ 7,3,5,1, false, false, }, // Face 2
			{ 3,2,1,0, false, true, }, // Face 3
			{ 0,4,1,5, true, false, }, // Face 4
			{ 2,6,0,4, false, false, }, // Face 5
		};

		unsigned faceCounter = 0;
		Vertex* vertices = mVertices;
		unsigned short* indices = mIndices;
		Vector3 scale = size * (radius / primaryDimension);

		unsigned vertexCounter = 0;
		for (int face = 0; face < 6; ++face)
		{
			if(decal && decal->getFace() != face)
				continue;

			const FACEDESC& desc = facesDesc[face];
			Vector3 start0 = corners[desc.start0]; //, start1 = corners[desc.start1];

			Vector3 worldDirU = facesDirU[face] * invTesselationFactor * 2;
			Vector3 worldDirV = facesDirV[face] * invTesselationFactor * 2;

			Vector3 worldPos = start0;

			UVAtlasInfo studsAtlasInfo = getStudsAtlasInfo((ignoreMaterialsStuds || (options.flags & Flag_IgnoreStuds)) ? NO_SURFACE : part->getConstPartPrimitive()->getSurfaceType((NormalId)face));

			float studOffsetU = studsAtlasInfo.tileX ? primaryDimensionStuds * invTesselationFactor / 2.0f : 0.0f;
			float studOffsetV = studsAtlasInfo.scaleY * primaryDimensionStuds * invTesselationFactor / TESSELATION_PIECE;

			Vector2 mainOffset = decal ? getDecalUV(decal, size, ignoreMaterialsStuds) : abs(Vector2(dot(facesDirV[face], size),dot(facesDirU[face], size)));
			Vector2 mainCoords = surfaceOffset + Vector2(mainOffset.x, 0);
			mainOffset *= invTesselationFactor;
			
			for (unsigned quad = 0; quad < quadNumberI; quad++) {
				unsigned prevVertexCounter = vertexCounter;
				Vector2 studStart = Vector2(studOffsetU * tesselationPerQuad, studsAtlasInfo.offsetY + 
					frac(quad*primaryDimensionStuds / quadNumberF) / TESSELATION_PIECE * studsAtlasInfo.scaleY);

				// Generate vertices
				for(unsigned iU = 0; iU < tesselationPerQuad+1; iU++) 
				{
					Vector3 worlsPosV = worldPos;
					Vector2 studsV = studStart;
					Vector2 mainCoordsV = mainCoords;

					for(unsigned iV = 0; iV < tesselationFactor+1; iV++) 
					{
						Vector3 normal = worlsPosV.direction();
						Vector3 localPos = scale*normal;

						Vector3 worldOffset = options.cframe.vectorToWorldSpace(localPos);
						Vector3 worldNormal = worldOffset.direction();

						Vector3 tangent = (worldNormal*dot(tangents[face],worldNormal) - tangents[face]).direction();
						
						fillVertex(vertices[vertexOffset + vertexCounter], options.cframe.translation + worldOffset + center, worldNormal, 
							mainCoordsV * surfaceTiling, studsV, 
							color, extra, tangent);
						vertexCounter++;
						worlsPosV += worldDirV;
						studsV.x -= studOffsetU;
						mainCoordsV.x -= mainOffset.x;
					}
					if(iU < tesselationPerQuad) {
						worldPos += worldDirU;
						mainCoords.y += mainOffset.y;
					}

					studStart.y += studOffsetV;
				}

				// Generate faces
				for(unsigned iU = 0; iU < tesselationPerQuad; iU++) 
				{
					for(unsigned iV = 0; iV < tesselationFactor; iV++) 
					{
						fillQuadIndices(&indices[indexOffset + faceCounter*6], vertexOffset + prevVertexCounter, 
							iU*(tesselationFactor+1)+iV, 
							iU*(tesselationFactor+1)+iV+1,
							(iU+1)*(tesselationFactor+1)+iV,
							(iU+1)*(tesselationFactor+1)+iV+1);
						faceCounter++;
					}
				}
			}
		}

		RBXASSERT(faceCounter == quads);
		RBXASSERT(vertexCounter == totalVerts);

		extendBoundsBlock(mBboxMin, mBboxMax, options.cframe.translation + center, size, worldAxisX, worldAxisY, worldAxisZ);
	}

	template<bool corner> void GeometryGenerator::addWedge(const Vector3& size, const Vector3& center, PartInstance* part, Decal* decal, const Options& options, unsigned int randomSeed, bool ignoreMaterialsStuds)
	{
		size_t vertexOffset = mVertexCount;
		size_t indexOffset = mIndexCount;
	
		bool facesIgnoreTiling[NORM_UNDEFINED] = { 0 };
		Vector3 localExtentsSize = size;

		float wedgeFaceLength[6];

		if (!corner) {
			float diagLenZY = sqrt(size.y*size.y+size.z*size.z);
			wedgeFaceLength[0] = size.y;
			wedgeFaceLength[1] = size.z;
			wedgeFaceLength[2] = size.y;
			wedgeFaceLength[3] = size.y;
			wedgeFaceLength[4] = size.z;
			wedgeFaceLength[5] = diagLenZY;
		}
		else 
		{
			float diagLenXY = sqrt(size.y*size.y+size.x*size.x);
			float diagLenZY = sqrt(size.y*size.y+size.z*size.z);
			wedgeFaceLength[0] = size.y;
			wedgeFaceLength[1] = size.z;
			wedgeFaceLength[2] = diagLenZY;
			wedgeFaceLength[3] = diagLenXY;
			wedgeFaceLength[4] = size.z;
			wedgeFaceLength[5] = size.y;
		}

		bool noBlanks = true;

		unsigned quads = 1;
		if(!decal) 
		{
			quads = 0;
			for (int face = 0; face < NORM_UNDEFINED; face++)
			{
				SurfaceType surface = part->getConstPartPrimitive()->getSurfaceType((NormalId)face);
				if(surface == NO_SURFACE_NO_OUTLINES)
					noBlanks = false;

				if(face == NORM_Y)
					continue;

				facesIgnoreTiling[face] = IsNoSurface(surface) || ignoreMaterialsStuds;
				if(facesIgnoreTiling[face])
					quads += 1;
				else
				{
					quads += fastCeilPositiveApprox(wedgeFaceLength[face] / TESSELATION_PIECE);
				}
			}
		}
		
		if (quads > 32768 / 4)
		{
			// We do not allow a part to generate more than 32768 vertices
			// This is both to ensure that we never exceed 16-bit index limit even if we had some vertices before, and to avoid potential issues with negative size components
			RBXASSERT(false);
			return;
		}
		
		mVertexCount += quads*4;
		mIndexCount += quads*6;
		
		if (!mVertices) return;

		Color4uint8 color = getColor(part, decal, NULL, options, randomSeed, true);
		Color4uint8 extra = getExtra(part, options);
		
		const CoordinateFrame& cframe = options.cframe;
		
		Vector3 axisX = Vector3(cframe.rotation[0][0], cframe.rotation[1][0], cframe.rotation[2][0]);
		Vector3 axisY = Vector3(cframe.rotation[0][1], cframe.rotation[1][1], cframe.rotation[2][1]);
		Vector3 axisZ = Vector3(cframe.rotation[0][2], cframe.rotation[1][2], cframe.rotation[2][2]);
		
		Vector3 extent = size * 0.5f;
		
		Vector3 cornerO = cframe.pointToWorldSpace(Vector3(center - extent));
		Vector3 cornerX = axisX * size.x;
		Vector3 cornerY = axisY * size.y;
		Vector3 cornerZ = axisZ * size.z;
		
		Vector3 corners[8] =
		{
			cornerO,
			cornerO + cornerZ,
			cornerO + cornerY,
			cornerO + cornerY + cornerZ,
			cornerO + cornerX,
			cornerO + cornerX + cornerZ,
			cornerO + cornerX + cornerY,
			cornerO + cornerX + cornerY + cornerZ,
		};

		float zyProportion = size.z/size.y;
		float xyProportion = size.x/size.y;
		Vector3 adjustedAxisZ = axisZ*zyProportion;
		Vector3 adjustedAxisX = axisX*xyProportion;

		Vector2 surfaceOffset = getSurfaceOffset(size, randomSeed, part->getRenderMaterial());
        float surfaceTiling = decal ? 1 : getSurfaceTiling(part->getRenderMaterial());
			
		Vector3 normalOffset =
			(options.flags & Flag_RandomizeBlockAppearance)
				? kNormalOffsetTable[(randomSeed >> 3) & 31]
				: Vector3(0, 0, 0);
		
		Vector3 normals[6];
		if(!corner) 
		{
			normals[0] = normalize(normalOffset + axisX);
			normals[1] = normalize(normalOffset + axisY);
			normals[2] = normalize(normalOffset + axisZ);
			normals[3] = normalize(normalOffset - axisX);
			normals[4] = normalize(normalOffset - axisY);
			normals[5] = normalize(normalOffset - axisZ + axisY*zyProportion);
		} 
		else
		{
			Vector3 diag = -size.x*axisX - size.y*axisY + size.z*axisZ;
			Vector3 zside = -size.y*axisY + size.z*axisZ;
			Vector3 xside = -size.x*axisX - size.y*axisY;
			normals[0] = normalize(normalOffset + axisX);
			normals[1] = normalize(normalOffset + axisY);
			normals[2] = normalize(normalOffset + diag.cross(zside));
			normals[3] = normalize(normalOffset + xside.cross(diag));
			normals[4] = normalize(normalOffset - axisY);
			normals[5] = normalize(normalOffset - axisZ);
		}
		
		Vector3 tangents[6] =
		{
			-axisZ, -axisX,  axisX,
			 axisZ, -axisX, -axisX,
		};
		
		Vertex* vertices = mVertices;
		unsigned short* indices = mIndices;

		if (decal)
		{
			// Note: this table is almost identical to the facesDesc below, with the exception of bottom face.
			// This is needed to preserve the decal orientation compared to legacy code.
			static const unsigned int wedgeFaces[6][4] =
			{
				{7, 7, 4, 5}, // Face 0 +X
				{3, 7, 0, 4}, // Face 1 +Y
				{7, 3, 5, 1}, // Face 2 +Z
				{3, 3, 1, 0}, // Face 3 -X
				{5, 1, 4, 0}, // Face 4 -Y
				{3, 7, 0, 4}, // Face 5 -Z
			};

            static const unsigned int cornerWedgeFaces[6][4] =
            {
                { 6,6,4,5}, // Face 0 +X
                { 0,1,6,5}, // Face 1 +Y
                { 6,6,5,1}, // Face 2 +Z
                { 6,6,1,0}, // Face 3 -X
                { 5,1,4,0}, // Face 4 -Y
                { 6,6,0,4}, // Face 5 -Z
            };

            const unsigned int (*faces)[4] = corner ? cornerWedgeFaces : wedgeFaces;
			
			int face = decal->getFace();
			
			// If the block ignores studs (block meshes), then the Texture UV generation should not take size into account as well.
			Vector2 uv = getDecalUV(decal, size, /* ignoreSurfaceType= */ ignoreMaterialsStuds);

            Vector2 outUvs[4];
            if (corner && face == NORM_Y)
            {
                // when projecting from top to corner wedge, make decal to "stand" on base part
                outUvs[3] = Vector2(uv.x, uv.y);
                outUvs[2] = Vector2(uv.x, 0);
                outUvs[1] = Vector2(0, uv.y);
                outUvs[0] = Vector2(0, 0);
            }
            else
            {
                outUvs[0] = Vector2(uv.x, 0);
                outUvs[1] = Vector2(0, 0);
                outUvs[2] = Vector2(uv.x, uv.y);
                outUvs[3] = Vector2(0, uv.y);
            }

            if(!corner)
            {
                if(face == NORM_X_NEG) // Fix for wedges
                    outUvs[1] = outUvs[0];
            }
            else
            {
                if(face == NORM_X || face == NORM_Z) // Fix for triangular part of corner wedge part
                    outUvs[1] = outUvs[0];
            }
            
			fillVertex(vertices[vertexOffset + 0], corners[faces[face][0]], normals[face], outUvs[0], Vector2(), color, extra, Vector3());
			fillVertex(vertices[vertexOffset + 1], corners[faces[face][1]], normals[face], outUvs[1], Vector2(), color, extra, Vector3());
			fillVertex(vertices[vertexOffset + 2], corners[faces[face][2]], normals[face], outUvs[2], Vector2(), color, extra, Vector3());
			fillVertex(vertices[vertexOffset + 3], corners[faces[face][3]], normals[face], outUvs[3], Vector2(), color, extra, Vector3());
			
			fillQuadIndices(&indices[indexOffset], vertexOffset, 0, 1, 2, 3);
										
			for (int i = 0; i < 4; ++i)
				extendBounds(mBboxMin, mBboxMax, corners[faces[face][i]]);
		}
		else
		{
			struct WEDGESTEPDESC
			{
				Vector3 faceDir0, faceDir1;
				float studOffset0, studOffset1;

				inline WEDGESTEPDESC(const Vector3& faceDir0, const Vector3& faceDir1, float studOffset0, float studOffset1)
				    : faceDir0(faceDir0), faceDir1(faceDir1), studOffset0(studOffset0), studOffset1(studOffset1)
				{}

				inline WEDGESTEPDESC()
				{}
			};

			WEDGESTEPDESC wedgeStepDesc[6];

			if(!corner)
			{
				Vector3 wedgeDirection = (-axisY*size.y - axisZ*size.z).direction();;

				wedgeStepDesc[0] = WEDGESTEPDESC(-axisY-adjustedAxisZ,	-axisY, zyProportion, 0.0f);
				wedgeStepDesc[1] = WEDGESTEPDESC(Vector3(0,0,0), Vector3(0,0,0), 0.0f, 0.0f);
				wedgeStepDesc[2] = WEDGESTEPDESC(-axisY, -axisY, 0.0f, 0.0f);
				wedgeStepDesc[3] = WEDGESTEPDESC(-axisY,-axisY-adjustedAxisZ, 0.0f,  zyProportion);
				wedgeStepDesc[4] = WEDGESTEPDESC(axisZ, axisZ, 0.0f, 0.0f);
				wedgeStepDesc[5] = WEDGESTEPDESC(wedgeDirection, wedgeDirection, 0.0f, 0.0f);
			}
			else
			{
				Vector3 sideZDirection = (-size.y*axisY + size.z*axisZ).direction();
				Vector3 sideXDirection = (-size.x*axisX - size.y*axisY).direction();
				float xAspect = size.x/wedgeFaceLength[2];
				float zAspect = size.z/wedgeFaceLength[3];

				wedgeStepDesc[0] = WEDGESTEPDESC(-axisY, -axisY+adjustedAxisZ, 0.0f, zyProportion);
				wedgeStepDesc[1] = WEDGESTEPDESC(Vector3(0,0,0), Vector3(0,0,0), 0.0f, 0.0f);
				wedgeStepDesc[2] = WEDGESTEPDESC(sideZDirection, sideZDirection-axisX*xAspect, 0.0f, xAspect);
				wedgeStepDesc[3] = WEDGESTEPDESC(sideXDirection+axisZ*zAspect, sideXDirection, zAspect,  0.0f);
				wedgeStepDesc[4] = WEDGESTEPDESC(axisZ, axisZ, 0.0f, 0.0f);
				wedgeStepDesc[5] = WEDGESTEPDESC(-axisY -adjustedAxisX, -axisY, xyProportion, 0.0f);
			}

			static const FACEDESC wedgeFacesDesc[] =
			{
				{ 7,7,4,5, false, true, }, // Face 0
				{ 3,7,3,7, true, false, }, // Face 1
				{ 7,3,5,1, false, false, }, // Face 2
				{ 3,3,1,0, false, true, }, // Face 3
				{ 0,4,1,5, true, false, }, // Face 4
				{ 3,7,0,4, false, false, }, // Face 5
			};

			static const FACEDESC cornerWedgeFacesDesc[] =
			{
				{ 6,6,4,5, false, true, }, // Face 0
				{ 6,6,6,6, true, false, }, // Face 1
				{ 6,6,5,1, false, false, }, // Face 2
				{ 6,6,1,0, false, true, }, // Face 3
				{ 0,4,1,5, true, false, }, // Face 4
				{ 6,6,0,4, false, false, }, // Face 5
			};

			const FACEDESC* facesDesc = corner ? cornerWedgeFacesDesc : wedgeFacesDesc;


			unsigned faceCounter = 0;

			Vector2 uvs[4];

			for (int face = 0; face < ARRAYSIZE(wedgeFacesDesc); ++face)
			{
				if(face == NORM_Y) // Wedge only
					continue;

				const FACEDESC& desc = facesDesc[face];
				const WEDGESTEPDESC& wdesc = wedgeStepDesc[face];

				Vector3 start0 = corners[desc.start0], start1 = corners[desc.start1];

				int quadNumber = fastCeilPositiveApprox(wedgeFaceLength[face] / TESSELATION_PIECE);
				float quadLength = wedgeFaceLength[face];

				if(facesIgnoreTiling[face])
					quadNumber = 1;

				Vector3 offset0 = Vector3::zero(),offset1 = Vector3::zero();

				float uTiling = (desc.tangentZ ? localExtentsSize.z : localExtentsSize.x);

				uvs[0] = surfaceOffset;
				uvs[0].x += uTiling;
				uvs[1] = surfaceOffset;
				uvs[2] = surfaceOffset;
				uvs[2].x += uTiling;
				uvs[2].y += TESSELATION_PIECE;
				uvs[3] = surfaceOffset;
				uvs[3].y += TESSELATION_PIECE;
				
				UVAtlasInfo studsAtlasInfo = getStudsAtlasInfo((ignoreMaterialsStuds || (options.flags & Flag_IgnoreStuds)) ? NO_SURFACE : part->getConstPartPrimitive()->getSurfaceType((NormalId)face));
				
				float uTilingStuds = studsAtlasInfo.tileX ? uTiling * 0.5f : 0;
                float uStudsIncrement = studsAtlasInfo.tileX ? TESSELATION_PIECE * 0.5f : 0;

				Vector2 uvStuds[4] =
				{
					Vector2(uTilingStuds, studsAtlasInfo.offsetY),
					Vector2(0, studsAtlasInfo.offsetY),
					Vector2(uTilingStuds, studsAtlasInfo.offsetY + studsAtlasInfo.scaleY),
					Vector2(0, studsAtlasInfo.offsetY + studsAtlasInfo.scaleY),
				};

				Vector4 startEdgeOffset = Vector4::zero();
				if(!noBlanks)
				{
					if(corner)
						startEdgeOffset =computeOutlineOffsetCornerWedge((NormalId)face, part);
					else
						startEdgeOffset =computeOutlineOffsetWedge((NormalId)face, part);
				}

				Vector4 edge0 = Vector4(0,uTiling,0,quadLength) + startEdgeOffset;
				Vector4 edge1 = Vector4(uTiling,0,0,quadLength) + startEdgeOffset;

				if(wdesc.studOffset1 != 0) {
					uvStuds[1].x = uTilingStuds;
					uvStuds[3].x = uTilingStuds;
					uvs[1].x = uvs[0].x;
					uvs[3].x = uvs[2].x;
					edge1.x = edge0.x;
					edge0.y = edge1.y;
				}
				if(wdesc.studOffset0 != 0) {
					uvStuds[0].x = 0;
					uvStuds[2].x = 0;
					uvs[0].x = uvs[1].x;
					uvs[2].x = uvs[3].x;
					edge1.x = edge0.x;
					edge0.y = edge1.y;
				}

				Vector3 worldDir0 = wdesc.faceDir0 * TESSELATION_PIECE;
				Vector3 worldDir1 = wdesc.faceDir1 * TESSELATION_PIECE;

				for (int quad = 0; quad < quadNumber - 1; quad++)
				{
					Vector3 nextOffset0 = offset0 + worldDir0;
					Vector3 nextOffset1 = offset1 + worldDir1;

					Vector4 nextEdge0 = edge0 + Vector4(0,0,TESSELATION_PIECE, -TESSELATION_PIECE);
					Vector4 nextEdge1 = edge1 + Vector4(0,0,TESSELATION_PIECE, -TESSELATION_PIECE);

					if(wdesc.studOffset1 != 0) {
						uvStuds[3].x -= wdesc.studOffset1*uStudsIncrement;
						uvs[3].x -= wdesc.studOffset1*TESSELATION_PIECE;
						nextEdge0.y += wdesc.studOffset1*TESSELATION_PIECE;
						nextEdge1.x += wdesc.studOffset1*TESSELATION_PIECE;
					}

					if(wdesc.studOffset0 != 0) {
						uvStuds[2].x += wdesc.studOffset0*uStudsIncrement;
						uvs[2].x += wdesc.studOffset0*TESSELATION_PIECE;

						nextEdge0.x += wdesc.studOffset0*TESSELATION_PIECE;
						nextEdge1.y += wdesc.studOffset0*TESSELATION_PIECE;
					}
					fillVertex(vertices[vertexOffset + faceCounter * 4 + 0], start0 + offset0, normals[face], uvs[0] * surfaceTiling, uvStuds[0], color, extra, tangents[face], edge0);
					fillVertex(vertices[vertexOffset + faceCounter * 4 + 1], start1 + offset1, normals[face], uvs[1] * surfaceTiling, uvStuds[1], color, extra, tangents[face], edge1);
					fillVertex(vertices[vertexOffset + faceCounter * 4 + 2], start0 + nextOffset0, normals[face], uvs[2] * surfaceTiling, uvStuds[2], color, extra, tangents[face], nextEdge0);
					fillVertex(vertices[vertexOffset + faceCounter * 4 + 3], start1 + nextOffset1, normals[face], uvs[3] * surfaceTiling, uvStuds[3], color, extra, tangents[face], nextEdge1);
				
					uvs[0].y = uvs[2].y;
					uvs[1].y = uvs[3].y;
					uvs[2].y += TESSELATION_PIECE;
					uvs[3].y += TESSELATION_PIECE;

					if(wdesc.studOffset1 != 0) {
						uvStuds[1].x = uvStuds[3].x;
						uvs[1].x = uvs[3].x;
					}
				
					if(wdesc.studOffset0 != 0) {
						uvStuds[0].x = uvStuds[2].x;
						uvs[0].x = uvs[2].x;
					}

					offset0 = nextOffset0;
					offset1 = nextOffset1;
					edge0 = nextEdge0;
					edge1 = nextEdge1;
					
					fillQuadIndices(&indices[indexOffset + faceCounter * 6], vertexOffset + faceCounter * 4, 0, 1, 2, 3);
					faceCounter++;
				}
				
				if (quadNumber > 0)
				{
					// Last quad
					float texRemainderFraction = wedgeFaceLength[face] - (quadNumber - 1)*TESSELATION_PIECE;
					float texMultiplier = saturate(texRemainderFraction / TESSELATION_PIECE);

					Vector4 nextEdge0 = edge0 + Vector4(0,0,texRemainderFraction, -texRemainderFraction);
					Vector4 nextEdge1 = edge1 + Vector4(0,0,texRemainderFraction, -texRemainderFraction);

					if(wdesc.studOffset1 != 0) {
						uvStuds[3].x -= wdesc.studOffset1*texMultiplier*uStudsIncrement;
						uvs[3].x -= wdesc.studOffset1*texRemainderFraction;

						nextEdge0.y += wdesc.studOffset1*texRemainderFraction;
						nextEdge1.x += wdesc.studOffset1*texRemainderFraction;

					}

					if(wdesc.studOffset0 != 0) {
						uvStuds[2].x += wdesc.studOffset0*texMultiplier*uStudsIncrement;
						uvs[2].x += wdesc.studOffset0*texRemainderFraction;

						nextEdge0.x += wdesc.studOffset0*texRemainderFraction;
						nextEdge1.y += wdesc.studOffset0*texRemainderFraction;
					}

					Vector2 uv2 = Vector2(uvs[2].x, uvs[0].y + texRemainderFraction);
					Vector2 uv3 = Vector2(uvs[3].x, uvs[1].y + texRemainderFraction);

					Vector2 uvStuds2 = Vector2(uvStuds[2].x, studsAtlasInfo.offsetY + texMultiplier * studsAtlasInfo.scaleY);
					Vector2 uvStuds3 = Vector2(uvStuds[3].x, studsAtlasInfo.offsetY + texMultiplier * studsAtlasInfo.scaleY);

					fillVertex(vertices[vertexOffset + faceCounter * 4 + 0], start0 + offset0, normals[face], uvs[0] * surfaceTiling, uvStuds[0], color, extra, tangents[face], edge0);
					fillVertex(vertices[vertexOffset + faceCounter * 4 + 1], start1 + offset1, normals[face], uvs[1] * surfaceTiling, uvStuds[1], color, extra, tangents[face], edge1);
					fillVertex(vertices[vertexOffset + faceCounter * 4 + 2], corners[desc.end0], normals[face], uv2 * surfaceTiling, uvStuds2, color, extra, tangents[face], nextEdge0);
					fillVertex(vertices[vertexOffset + faceCounter * 4 + 3], corners[desc.end1], normals[face], uv3 * surfaceTiling, uvStuds3, color, extra, tangents[face], nextEdge1);
					
					fillQuadIndices(&indices[indexOffset + faceCounter * 6], vertexOffset + faceCounter * 4, 0, 1, 2, 3);
					faceCounter++;
				}
			}
			
			RBXASSERT(faceCounter == quads);
			
			Vector3 boundsCenter = cframe.pointToWorldSpace(center);
			extendBoundsBlock(mBboxMin, mBboxMax, boundsCenter, size, axisX, axisY, axisZ);
		}
	}

	void GeometryGenerator::addBlock(const Vector3& size, const Vector3& center, PartInstance* part, Decal* decal, const Options& options, unsigned int randomSeed, bool ignoreMaterialsStuds)
	{
		size_t vertexOffset = mVertexCount;
		size_t indexOffset = mIndexCount;
	
		bool facesIgnoreTiling[NORM_UNDEFINED] = { 0 };
		Vector3 localExtentsSize = size;
		Vector3int16 tesselatedExtents = Vector3CeilPositiveApprox(localExtentsSize / TESSELATION_PIECE);

		static const FACEDESC facesDesc[] =
		{
			{ 6,7,4,5, false, true, }, // Face 0
			{ 3,7,2,6, true, false, }, // Face 1
			{ 7,3,5,1, false, false, }, // Face 2
			{ 3,2,1,0, false, true, }, // Face 3
			{ 0,4,1,5, true, false, }, // Face 4
			{ 2,6,0,4, false, false, }, // Face 5
		};

		unsigned quads = 1;
		bool noBlanks = true;

		if(!decal) 
		{
			quads = 0;

			for (int face = 0; face < NORM_UNDEFINED; face++)
			{
				SurfaceType surface = part->getConstPartPrimitive()->getSurfaceType((NormalId)face);

				facesIgnoreTiling[face] = IsNoSurface(surface) || ignoreMaterialsStuds;
				if(facesIgnoreTiling[face])
					quads += 1;
				else
				{
					const FACEDESC& desc = facesDesc[face];
					quads += desc.dirZ ? tesselatedExtents.z : tesselatedExtents.y;
				}

				if(surface == NO_SURFACE_NO_OUTLINES)
					noBlanks = false;
			}
		}

		if (quads > 32768 / 4)
		{
			// We do not allow a part to generate more than 32768 vertices
			// This is both to ensure that we never exceed 16-bit index limit even if we had some vertices before, and to avoid potential issues with negative size components
			RBXASSERT(false);
			return;
		}
		
		mVertexCount += quads*4;
		mIndexCount += quads*6;
		
		if (!mVertices) return;

		Color4uint8 color = getColor(part, decal, NULL, options, randomSeed, true);
		Color4uint8 extra = getExtra(part, options);
		
		const CoordinateFrame& cframe = options.cframe;
		
		Vector3 axisX = Vector3(cframe.rotation[0][0], cframe.rotation[1][0], cframe.rotation[2][0]);
		Vector3 axisY = Vector3(cframe.rotation[0][1], cframe.rotation[1][1], cframe.rotation[2][1]);
		Vector3 axisZ = Vector3(cframe.rotation[0][2], cframe.rotation[1][2], cframe.rotation[2][2]);
		
		Vector3 extent = size * 0.5f;
		
		Vector3 cornerO = cframe.pointToWorldSpace(Vector3(center - extent));
		Vector3 cornerX = axisX * size.x;
		Vector3 cornerY = axisY * size.y;
		Vector3 cornerZ = axisZ * size.z;
		
		Vector3 corners[8] =
		{
			cornerO,
			cornerO + cornerZ,
			cornerO + cornerY,
			cornerO + cornerY + cornerZ,
			cornerO + cornerX,
			cornerO + cornerX + cornerZ,
			cornerO + cornerX + cornerY,
			cornerO + cornerX + cornerY + cornerZ,
		};

		Vector2 surfaceOffset = getSurfaceOffset(size, randomSeed, part->getRenderMaterial());
        float surfaceTiling = decal ? 1 : getSurfaceTiling(part->getRenderMaterial());
			
		Vector3 normalOffset =
			(options.flags & Flag_RandomizeBlockAppearance)
				? kNormalOffsetTable[(randomSeed >> 3) & 31]
				: Vector3(0, 0, 0);
		
		Vector3 normals[6] =
		{
			 normalize(normalOffset + axisX), normalize(normalOffset + axisY), normalize(normalOffset + axisZ),
			 normalize(normalOffset - axisX), normalize(normalOffset - axisY), normalize(normalOffset - axisZ),
		};
		
		Vector3 tangents[6] =
		{
			-axisZ, -axisX,  axisX,
			 axisZ, -axisX, -axisX,
		};
		
		Vertex* vertices = mVertices;
		unsigned short* indices = mIndices;

		if (decal)
		{
			// Note: this table is almost identical to the facesDesc below, with the exception of bottom face.
			// This is needed to preserve the decal orientation compared to legacy code.
			static const unsigned int faces[6][4] =
			{
				{6, 7, 4, 5},
				{3, 7, 2, 6},
				{7, 3, 5, 1},
				{3, 2, 1, 0},
				{5, 1, 4, 0},
				{2, 6, 0, 4},
			};
			
			int face = decal->getFace();
			
			// If the block ignores studs (block meshes), then the Texture UV generation should not take size into account as well.
			Vector2 uv = getDecalUV(decal, size, /* ignoreSurfaceType= */ ignoreMaterialsStuds);
			
			fillVertex(vertices[vertexOffset + 0], corners[faces[face][0]], normals[face], Vector2(uv.x, 0), Vector2(), color, extra, Vector3());
			fillVertex(vertices[vertexOffset + 1], corners[faces[face][1]], normals[face], Vector2(0, 0), Vector2(), color, extra, Vector3());
			fillVertex(vertices[vertexOffset + 2], corners[faces[face][2]], normals[face], Vector2(uv.x, uv.y), Vector2(), color, extra, Vector3());
			fillVertex(vertices[vertexOffset + 3], corners[faces[face][3]], normals[face], Vector2(0, uv.y), Vector2(), color, extra, Vector3());
			
			fillQuadIndices(&indices[indexOffset], vertexOffset, 0, 1, 2, 3);
										
			for (int i = 0; i < 4; ++i)
				extendBounds(mBboxMin, mBboxMax, corners[faces[face][i]]);
		}
		else
		{
			Vector3 facesDir[] =
			{
				-axisY,
				-axisZ,
				-axisY,
				-axisY,
				axisZ,
				-axisY
			};

			unsigned faceCounter = 0;
	 
			Vector2 uvs[4];

			for (int face = 0; face < ARRAYSIZE(facesDesc); ++face)
			{
				const FACEDESC& desc = facesDesc[face];
				
				Vector3 start0 = corners[desc.start0], start1 = corners[desc.start1];
				int quadNumber = desc.dirZ ? tesselatedExtents.z : tesselatedExtents.y;

				float quadLength = desc.dirZ ? size.z : size.y;

				if(facesIgnoreTiling[face])
					quadNumber = 1;

				Vector3 offset = Vector3::zero();
	
				float uTiling = (desc.tangentZ ? localExtentsSize.z : localExtentsSize.x);

				uvs[0] = surfaceOffset;
				uvs[0].x += uTiling;
				uvs[1] = surfaceOffset;
				uvs[2] = surfaceOffset;
				uvs[2].x += uTiling;
				uvs[2].y += TESSELATION_PIECE;
				uvs[3] = surfaceOffset;
				uvs[3].y += TESSELATION_PIECE;
				
				UVAtlasInfo studsAtlasInfo = getStudsAtlasInfo((ignoreMaterialsStuds || (options.flags & Flag_IgnoreStuds)) ? NO_SURFACE : part->getConstPartPrimitive()->getSurfaceType((NormalId)face));
				
				float uTilingStuds = studsAtlasInfo.tileX ? uTiling * 0.5f : 0;

				Vector2 uvStuds[4] =
				{
					Vector2(uTilingStuds, studsAtlasInfo.offsetY),
					Vector2(0, studsAtlasInfo.offsetY),
					Vector2(uTilingStuds, studsAtlasInfo.offsetY + studsAtlasInfo.scaleY),
					Vector2(0, studsAtlasInfo.offsetY + studsAtlasInfo.scaleY),
				};

				Vector4 startEdgeOffset = noBlanks ? Vector4() : computeOutlineOffsetPart((NormalId)face, part);

				Vector4 edgeOffset = startEdgeOffset;

				Vector4 edge0 = Vector4(0,uTiling,0,quadLength);
				Vector4 edge1 = Vector4(uTiling,0,0,quadLength);

				Vector3 worldDir = facesDir[face] * TESSELATION_PIECE;

				for (int quad = 0; quad < quadNumber - 1; quad++)
				{
					Vector3 nextOffset = offset + worldDir;

					Vector4 nextEdgeOffset = edgeOffset + Vector4(0,0,TESSELATION_PIECE, -TESSELATION_PIECE);

					fillVertex(vertices[vertexOffset + faceCounter * 4 + 0], start0 + offset, normals[face], uvs[0] * surfaceTiling, uvStuds[0], color, extra, tangents[face], edge0 + edgeOffset);
					fillVertex(vertices[vertexOffset + faceCounter * 4 + 1], start1 + offset, normals[face], uvs[1] * surfaceTiling, uvStuds[1], color, extra, tangents[face], edge1 + edgeOffset);
					fillVertex(vertices[vertexOffset + faceCounter * 4 + 2], start0 + nextOffset, normals[face], uvs[2] * surfaceTiling, uvStuds[2], color, extra, tangents[face], edge0 + nextEdgeOffset);
					fillVertex(vertices[vertexOffset + faceCounter * 4 + 3], start1 + nextOffset, normals[face], uvs[3] * surfaceTiling, uvStuds[3], color, extra, tangents[face], edge1 + nextEdgeOffset);
				
					uvs[0].y = uvs[2].y;
					uvs[1].y = uvs[3].y;
					uvs[2].y += TESSELATION_PIECE;
					uvs[3].y += TESSELATION_PIECE;

					offset = nextOffset;
					edgeOffset = nextEdgeOffset;
					
					fillQuadIndices(&indices[indexOffset + faceCounter * 6], vertexOffset + faceCounter * 4, 0, 1, 2, 3);
					faceCounter++;
				}
				
				if (quadNumber > 0)
				{
					// Last quad
					float texRemainderFraction = (desc.dirZ ? localExtentsSize.z : localExtentsSize.y) - (quadNumber - 1)*TESSELATION_PIECE;
					float texMultiplier = saturate(texRemainderFraction / TESSELATION_PIECE);

					Vector2 uv2 = Vector2(uvs[2].x, uvs[0].y + texRemainderFraction);
					Vector2 uv3 = Vector2(uvs[3].x, uvs[1].y + texRemainderFraction);
					
					Vector2 uvStuds2 = Vector2(uvStuds[2].x, studsAtlasInfo.offsetY + texMultiplier * studsAtlasInfo.scaleY);
					Vector2 uvStuds3 = Vector2(uvStuds[3].x, studsAtlasInfo.offsetY + texMultiplier * studsAtlasInfo.scaleY);

					Vector4 finalEdgeOffset = startEdgeOffset + Vector4(0,0, quadLength, -quadLength);

					fillVertex(vertices[vertexOffset + faceCounter * 4 + 0], start0 + offset, normals[face], uvs[0] * surfaceTiling, uvStuds[0], color, extra, tangents[face], edge0 + edgeOffset);
					fillVertex(vertices[vertexOffset + faceCounter * 4 + 1], start1 + offset, normals[face], uvs[1] * surfaceTiling, uvStuds[1], color, extra, tangents[face], edge1 + edgeOffset);
					fillVertex(vertices[vertexOffset + faceCounter * 4 + 2], corners[desc.end0], normals[face], uv2 * surfaceTiling, uvStuds2, color, extra, tangents[face], edge0 + finalEdgeOffset);
					fillVertex(vertices[vertexOffset + faceCounter * 4 + 3], corners[desc.end1], normals[face], uv3 * surfaceTiling, uvStuds3, color, extra, tangents[face], edge1 + finalEdgeOffset);
					
					fillQuadIndices(&indices[indexOffset + faceCounter * 6], vertexOffset + faceCounter * 4, 0, 1, 2, 3);
					faceCounter++;
				}
			}
			
			RBXASSERT(faceCounter == quads);
			
			Vector3 boundsCenter = cframe.pointToWorldSpace(center);
			extendBoundsBlock(mBboxMin, mBboxMax, boundsCenter, size, axisX, axisY, axisZ);
		}
	}

	void GeometryGenerator::addTorso(const Vector3& size, const Vector3& center, PartInstance* part, Decal* decal, const Options& options, unsigned int randomSeed)
	{
		int quads = decal ? 1 : 6;
		
		size_t vertexOffset = mVertexCount;
		size_t indexOffset = mIndexCount;

		mVertexCount += quads*4;
		mIndexCount += quads*6;
		
		if (!mVertices) return;

		Color4uint8 color = getColor(part, decal, NULL, options, randomSeed, false);
		Color4uint8 extra = getExtra(part, options);
		
		const CoordinateFrame& cframe = options.cframe;
		
		Vector3 extent = size * 0.5f;

		float shoulderInset = std::min(extent.x, size.z * 0.3f);

		Vector3 corners[8] =
		{
			Vector3(-extent.x, -extent.y, -extent.z) + center,
			Vector3(-extent.x, -extent.y, +extent.z) + center,
			Vector3(-extent.x + shoulderInset, +extent.y, -extent.z) + center,
			Vector3(-extent.x + shoulderInset, +extent.y, +extent.z) + center,
			Vector3(+extent.x, -extent.y, -extent.z) + center,
			Vector3(+extent.x, -extent.y, +extent.z) + center,
			Vector3(+extent.x - shoulderInset, +extent.y, -extent.z) + center,
			Vector3(+extent.x - shoulderInset, +extent.y, +extent.z) + center,
		};

		Vector2 surfaceOffset = getSurfaceOffset(size, randomSeed, part->getRenderMaterial());
        float surfaceTiling = decal ? 1 : getSurfaceTiling(part->getRenderMaterial());
			
		Vector3 normals[6] =
		{
			normalize(Vector3( size.y, shoulderInset, 0)), Vector3(0,  1, 0), Vector3(0, 0,  1),
			normalize(Vector3(-size.y, shoulderInset, 0)), Vector3(0, -1, 0), Vector3(0, 0, -1),
		};

		Vector3 tangents[6] =
		{
			-Vector3(0, 0, 1), -Vector3(1, 0, 0),  Vector3(1, 0, 0),
			 Vector3(0, 0, 1), -Vector3(1, 0, 0), -Vector3(1, 0, 0),
		};
		
		Vertex* vertices = mVertices;
		unsigned short* indices = mIndices;

		static const FACEDESC facesDesc[] =
		{
			{ 6,7,4,5, false, true, }, // Face 0
			{ 3,7,2,6, true, false, }, // Face 1
			{ 7,3,5,1, false, false, }, // Face 2
			{ 3,2,1,0, false, true, }, // Face 3
			{ 0,4,1,5, true, false, }, // Face 4
			{ 2,6,0,4, false, false, }, // Face 5
		};


		if (decal)
		{
			// Note: this table is almost identical to the facesDesc below, with the exception of bottom face.
			// This is needed to preserve the decal orientation compared to legacy code.
			static const unsigned int faces[6][4] =
			{
				{6, 7, 4, 5},
				{3, 7, 2, 6},
				{7, 3, 5, 1},
				{3, 2, 1, 0},
				{5, 1, 4, 0},
				{2, 6, 0, 4},
			};
			
			int face = decal->getFace();
			
			// If the block ignores studs (block meshes), then the Texture UV generation should not take size into account as well.
			Vector2 uv = getDecalUV(decal, size, /* ignoreSurfaceType= */ false);

			Vector3 normal = cframe.vectorToWorldSpace(normals[face]);
			
			fillVertex(vertices[vertexOffset + 0], cframe.pointToWorldSpace(corners[faces[face][0]]), normal, Vector2(uv.x, 0), Vector2(), color, extra, Vector3());
			fillVertex(vertices[vertexOffset + 1], cframe.pointToWorldSpace(corners[faces[face][1]]), normal, Vector2(0, 0), Vector2(), color, extra, Vector3());
			fillVertex(vertices[vertexOffset + 2], cframe.pointToWorldSpace(corners[faces[face][2]]), normal, Vector2(uv.x, uv.y), Vector2(), color, extra, Vector3());
			fillVertex(vertices[vertexOffset + 3], cframe.pointToWorldSpace(corners[faces[face][3]]), normal, Vector2(0, uv.y), Vector2(), color, extra, Vector3());
			
			fillQuadIndices(&indices[indexOffset], vertexOffset, 0, 1, 2, 3);
										
			for (int i = 0; i < 4; ++i)
				extendBounds(mBboxMin, mBboxMax, cframe.pointToWorldSpace(corners[faces[face][i]]));
		}
		else
		{
			unsigned faceCounter = 0;
	 
			for (int face = 0; face < ARRAYSIZE(facesDesc); ++face)
			{
				const FACEDESC& desc = facesDesc[face];
				
				Vector3 start0 = corners[desc.start0], start1 = corners[desc.start1];
				Vector3 end0 = corners[desc.end0], end1 = corners[desc.end1];

				float uTiling = (desc.tangentZ ? size.z : size.x);
				float vTiling = (desc.dirZ ? size.z : size.y);

				Vector3 normal = cframe.vectorToWorldSpace(normals[face]);
				Vector3 tangent = cframe.vectorToWorldSpace(tangents[face]);
		
				fillVertex(vertices[vertexOffset + faceCounter * 4 + 0], cframe.pointToWorldSpace(start0), normal, (surfaceOffset + Vector2(uTiling, 0)) * surfaceTiling, Vector2(), color, extra, tangent);
				fillVertex(vertices[vertexOffset + faceCounter * 4 + 1], cframe.pointToWorldSpace(start1), normal, surfaceOffset * surfaceTiling, Vector2(), color, extra, tangent);
				fillVertex(vertices[vertexOffset + faceCounter * 4 + 2], cframe.pointToWorldSpace(end0), normal, (surfaceOffset + Vector2(uTiling, vTiling)) * surfaceTiling, Vector2(), color, extra, tangent);
				fillVertex(vertices[vertexOffset + faceCounter * 4 + 3], cframe.pointToWorldSpace(end1), normal, (surfaceOffset + Vector2(0, vTiling)) * surfaceTiling, Vector2(), color, extra, tangent);
					
				fillQuadIndices(&indices[indexOffset + faceCounter * 6], vertexOffset + faceCounter * 4, 0, 1, 2, 3);

				faceCounter++;
			}

			for (int i = 0; i < 8; ++i)
				extendBounds(mBboxMin, mBboxMax, cframe.pointToWorldSpace(corners[i]));
			
			RBXASSERT(faceCounter == quads);
		}
	}
	
	struct TrussQuadBuilder
	{
		GeometryGenerator::Vertex* vertices;
		unsigned short* indices;
		unsigned int vertexOffset;
		unsigned int indexOffset;
		
		Vector3* bboxMin;
		Vector3* bboxMax;
		
		Color4uint8 color;
		Color4uint8 extra;
		
		CoordinateFrame cframe;
		Vector2 surfaceOffset;
        float surfaceTiling;
		
		TrussQuadBuilder(GeometryGenerator::Vertex* vertices, unsigned short* indices, unsigned int vertexOffset, unsigned int indexOffset, Vector3* bboxMin, Vector3* bboxMax, const Vector3& size, PartInstance* part, const GeometryGenerator::Options& options, unsigned int randomSeed)
		{
			this->vertices = vertices;
			this->indices = indices;
			this->vertexOffset = vertexOffset;
			this->indexOffset = indexOffset;
			this->bboxMin = bboxMin;
			this->bboxMax = bboxMax;
			
			color = getColor(part, NULL, NULL, options, randomSeed, false);
			extra = getExtra(part, options);
			
			cframe = options.cframe;
			
			surfaceOffset = getSurfaceOffset(size, randomSeed, part->getRenderMaterial());
            surfaceTiling = getSurfaceTiling(part->getRenderMaterial());
		}
		
		void emit(const Vector3& pos, const Vector3& udir, float usize, const Vector3& vdir, float vsize)
		{
			if (vertices)
			{
				Vector3 corner0 = pos + udir * std::max(0.f, usize) + vdir * std::max(0.f, vsize);
				Vector3 corner1 = pos + udir * std::min(0.f, usize) + vdir * std::max(0.f, vsize);
				Vector3 corner2 = pos + udir * std::max(0.f, usize) + vdir * std::min(0.f, vsize);
				Vector3 corner3 = pos + udir * std::min(0.f, usize) + vdir * std::min(0.f, vsize);
				
				Vector3 localNormal = cross(udir, vdir);
				Vector3 normal = cframe.vectorToWorldSpace(localNormal);
				Vector3 tangent = cframe.vectorToWorldSpace(udir);
				
				fillVertex(vertices[vertexOffset + 0], cframe.pointToWorldSpace(corner0), normal, (surfaceOffset + Vector2(dot(corner0, udir), dot(corner0, vdir))) * surfaceTiling, Vector2(), color, extra, tangent);
				fillVertex(vertices[vertexOffset + 1], cframe.pointToWorldSpace(corner1), normal, (surfaceOffset + Vector2(dot(corner1, udir), dot(corner1, vdir))) * surfaceTiling, Vector2(), color, extra, tangent);
				fillVertex(vertices[vertexOffset + 2], cframe.pointToWorldSpace(corner2), normal, (surfaceOffset + Vector2(dot(corner2, udir), dot(corner2, vdir))) * surfaceTiling, Vector2(), color, extra, tangent);
				fillVertex(vertices[vertexOffset + 3], cframe.pointToWorldSpace(corner3), normal, (surfaceOffset + Vector2(dot(corner3, udir), dot(corner3, vdir))) * surfaceTiling, Vector2(), color, extra, tangent);
				
				fillQuadIndices(&indices[indexOffset], vertexOffset, 0, 1, 2, 3);
				
				extendBounds(*bboxMin, *bboxMax, cframe.pointToWorldSpace(corner0));
				extendBounds(*bboxMin, *bboxMax, cframe.pointToWorldSpace(corner1));
				extendBounds(*bboxMin, *bboxMax, cframe.pointToWorldSpace(corner2));
				extendBounds(*bboxMin, *bboxMax, cframe.pointToWorldSpace(corner3));
			}
			
			vertexOffset += 4;
			indexOffset += 6;
		}
	};
	
	void GeometryGenerator::addTruss(int style, const Vector3& size, PartInstance* part, Decal* decal, const Options& options, unsigned int randomSeed)
	{
		if (decal) return;
		
		switch (size.primaryAxis())
		{
		case Vector3::X_AXIS:
			return addTrussY(Vector3(-size.x / 2, 0, 0), Vector3(0, 0, -1), Vector3(1, 0, 0), Vector3(0, -1, 0), static_cast<int>(size.x / 2 + 0.005f), style, size, part, options, randomSeed);
			
		case Vector3::Y_AXIS:
			return addTrussY(Vector3(0, -size.y / 2, 0), Vector3(1, 0, 0), Vector3(0, 1, 0), Vector3(0, 0, 1), static_cast<int>(size.y / 2 + 0.005f), style, size, part, options, randomSeed);

		case Vector3::Z_AXIS:
			return addTrussY(Vector3(0, 0, -size.z / 2), Vector3(1, 0, 0), Vector3(0, 0, 1), Vector3(0, -1, 0), static_cast<int>(size.z / 2 + 0.005f), style, size, part, options, randomSeed);
        default:
            break;
		}
	}

	void GeometryGenerator::addTrussY(const Vector3& origin, const Vector3& axisX, const Vector3& axisY, const Vector3& axisZ, int segments, int style, const Vector3& size, PartInstance* part, const Options& options, unsigned int randomSeed)
	{
		if (segments == 0) return;
		
		int styleAlt = (style == ExtrudedPartInstance::BRIDGE_STYLE_CROSS_BEAM) ? ExtrudedPartInstance::NO_CROSS_BEAM : style;
		
		addTrussSideX(origin - axisX - axisZ, axisX, axisY, axisZ, false, segments, styleAlt, size, part, options, randomSeed);
		addTrussSideX(origin - axisX + axisZ, -axisZ, axisY, axisX, true, segments, style, size, part, options, randomSeed);
		addTrussSideX(origin + axisX + axisZ, -axisX, axisY, -axisZ, false, segments, styleAlt, size, part, options, randomSeed);
		addTrussSideX(origin + axisX - axisZ, axisZ, axisY, -axisX, false, segments, style, size, part, options, randomSeed);
	}
	
	void GeometryGenerator::addTrussSideX(const Vector3& origin, const Vector3& axisX, const Vector3& axisY, const Vector3& axisZ, bool reverseBridge, int segments, int style, const Vector3& size, PartInstance* part, const Options& options, unsigned int randomSeed)
	{
		TrussQuadBuilder builder(mVertices, mIndices, mVertexCount, mIndexCount, &mBboxMin, &mBboxMax, size, part, options, randomSeed);

		// Build vertical frame
		builder.emit(origin, -axisX, -0.25f, axisY, segments * 2);
		builder.emit(origin, axisZ, 0.25f, axisY, segments * 2);
		builder.emit(origin + axisX * 0.25f, -axisZ, -0.25f, axisY, segments * 2);
		builder.emit(origin + axisZ * 0.25f, axisX, 0.25f, axisY, segments * 2);
		
		// Build horizontal frames
		for (int i = 0; i <= segments; ++i)
		{
			Vector3 frameOrigin = origin + axisY * (i == 0 ? 0 : 2 * i - 0.25f);
			float frameHeight = (i == 0 || i == segments) ? 0.25f : 0.5f;
			
			float bottomStart = (i == 0) ? 0.f : 0.25f;
			float topStart = (i == segments) ? 0.f : 0.25f;
			
			builder.emit(frameOrigin + axisX * 0.25f, -axisX, -1.5f, axisY, frameHeight);
			builder.emit(frameOrigin + axisX * 0.25f + axisZ * 0.25f, axisX, 1.5f, axisY, frameHeight);
			builder.emit(frameOrigin + axisX * bottomStart, axisX, 1.75f - bottomStart, axisZ, 0.25f);
			builder.emit(frameOrigin + axisX * topStart + axisY * frameHeight, -axisX, -(1.75f - topStart), axisZ, 0.25f);
		}

		// Build supports
		if (style != ExtrudedPartInstance::NO_CROSS_BEAM && segments > 1)
		{
			for (int i = 0; i < segments; ++i)
			{
				bool reverse = style == ExtrudedPartInstance::FULL_ALTERNATING_CROSS_BEAM ? (i % 2 == 1) : (reverseBridge ? i >= segments / 2 : i < segments / 2);
				
				Vector3 frameAxisY = reverse ? -axisY : axisY;
			
				Vector3 frameOrigin = origin + axisY * 2 * i + (reverse ? axisY * 2 : Vector3());
				
				Vector3 supportAxisU = (frameAxisY - axisX) * 0.707106f * (reverse ? -1 : 1);
				Vector3 supportAxisV = (axisX + frameAxisY) * 0.707106f;
				
				float supportLength = 2.474873f; // sqrt(sqr(1.75) * 2)
				
				Vector3 supportOrigin = frameOrigin + (frameAxisY + axisX) * 0.125f + axisZ * 0.0625f - supportAxisU * 0.125f;
				
				builder.emit(supportOrigin, supportAxisU, 0.25f, supportAxisV, supportLength);
				builder.emit(supportOrigin + axisZ * 0.125f, -supportAxisU, -0.25f, supportAxisV, supportLength);
				builder.emit(supportOrigin, -axisZ, -0.125f, supportAxisV, supportLength);
				builder.emit(supportOrigin + axisZ * 0.125f + supportAxisU * 0.25f, axisZ, -0.125f, supportAxisV, supportLength);
			}
		}

		mVertexCount = builder.vertexOffset;
		mIndexCount = builder.indexOffset;
	}

	void GeometryGenerator::addPartImpl(PartInstance* part, Decal* decal, const Options& options, const Resources& resources, const HumanoidIdentifier* hi, bool ignoreMaterialsStuds)
	{
		DataModelMesh* specialShape = getSpecialShape(part);
		
		unsigned int randomSeed = boost::hash_value(part);
		
		// handle mesh parts
		if (resources.fileMeshData)
		{
			return addFileMesh(resources.fileMeshData.get(), specialShape, part, decal, hi, options);
		}
			
		// handle parts with special shape
		if (specialShape)
		{
			if (SpecialShape* shape = specialShape->fastDynamicCast<SpecialShape>())
			{
                Vector3 offset = shape->getOffset();
				switch (shape->getMeshType())
				{
				case SpecialShape::BRICK_MESH:
					return addBlock(part->getPartSizeXml() * abs(shape->getScale()), offset, part, decal, options, randomSeed, ignoreMaterialsStuds);
				case SpecialShape::SPHERE_MESH:
					return addSphere(part->getPartSizeXml() * abs(shape->getScale()), offset, part, decal, options, randomSeed, ignoreMaterialsStuds);
				case SpecialShape::CYLINDER_MESH:
					return addCylinder<false>(part->getPartSizeXml() * abs(shape->getScale()), offset, part, decal, options, randomSeed,  ignoreMaterialsStuds);
				case SpecialShape::TORSO_MESH:
					return addTorso(part->getPartSizeXml() * abs(shape->getScale()), offset, part, decal, options, randomSeed);
				case SpecialShape::WEDGE_MESH:
					return addWedge<false>(part->getPartSizeXml() * abs(shape->getScale()), offset, part, decal, options, randomSeed, ignoreMaterialsStuds);
				default:
					// mesh type not supported yet
					return;
				}
			}
			else if (BlockMesh* shape = specialShape->fastDynamicCast<BlockMesh>())
			{
				return addBlock(part->getPartSizeXml() * abs(shape->getScale()), shape->getOffset(), part, decal, options, randomSeed, /* ignoreMaterialsStuds= */ true);
			}
			else if (CylinderMesh* shape = specialShape->fastDynamicCast<CylinderMesh>())
			{
				return addCylinder<true>(part->getPartSizeXml() * abs(shape->getScale()), shape->getOffset(), part, decal, options, randomSeed, /* ignoreMaterialsStuds= */ true);
			}

			// Unknown shape type, or a file mesh with an empty meshData (which means that the mesh data could not be loaded)
			return;
		}
		
		// handle parts
		switch(part->getPartType())
		{
		case BLOCK_PART:
			return addBlock(part->getPartSizeXml(), Vector3::zero(), part, decal, options, randomSeed, ignoreMaterialsStuds);
		case BALL_PART: 
			return addSphere(part->getPartSizeXml(), Vector3::zero(), part, decal, options, randomSeed, ignoreMaterialsStuds);
		case CYLINDER_PART:
			return addCylinder<false>(part->getPartSizeXml(), Vector3::zero(), part, decal, options, randomSeed, ignoreMaterialsStuds);
		case TRUSS_PART:
			return addTruss(static_cast<ExtrudedPartInstance*>(part)->getVisualTrussStyle(), part->getPartSizeXml(), part, decal, options, randomSeed);
		case WEDGE_PART:
			return addWedge<false>(part->getPartSizeXml(), Vector3::zero(), part, decal, options, randomSeed, ignoreMaterialsStuds);
		case CORNERWEDGE_PART:
			return addWedge<true>(part->getPartSizeXml(), Vector3::zero(), part, decal, options, randomSeed, ignoreMaterialsStuds);
		case OPERATION_PART:
			return addOperation(part, decal, options, resources, randomSeed);
        default:
            // unknown geometry type, skip it
            return;
		}
	}

    template <class AssetProvider, class AssetType, class AssetIdType>
	static boost::shared_ptr<AssetType> fetchMesh(const AssetIdType& meshFile, Instance* instance, AsyncResult* asyncResult)
	{
		if (meshFile.isNull())
			return boost::shared_ptr<AssetType>();
			
		ContentProvider* contentProvider = ServiceProvider::find<ContentProvider>(instance);
		CacheableContentProvider* mcp = ServiceProvider::find<AssetProvider>(contentProvider);
		
		if (!mcp)
		{
			asyncResult->returnResult(AsyncHttpQueue::Failed);
            return boost::shared_ptr<AssetType>();
		}
		
		AsyncHttpQueue::RequestResult reqResult;
		boost::shared_ptr<AssetType> meshData = boost::static_pointer_cast<AssetType>(mcp->requestContent(meshFile, ContentProvider::PRIORITY_MESH, true, reqResult));
		
		switch (reqResult)
		{
		case AsyncHttpQueue::Succeeded:
			return meshData;
			
		case AsyncHttpQueue::Waiting:
			asyncResult->returnWaitingFor(meshFile);
            return boost::shared_ptr<AssetType>();
        default:
			asyncResult->returnResult(reqResult);
            return boost::shared_ptr<AssetType>();
		}
	}
	
	GeometryGenerator::Resources GeometryGenerator::fetchResources(PartInstance* part, const HumanoidIdentifier* hi, unsigned int flags, AsyncResult* asyncResult)
	{
		DataModelMesh* specialShape = getSpecialShape(part);
		
		// handle parts with file mesh
		if (FileMesh* shape = getFileMesh(specialShape))
		{
            boost::shared_ptr<FileMeshData> fileMeshData = fetchMesh<MeshContentProvider, FileMeshData, MeshId>(shape->getMeshId(), part, asyncResult);
            return Resources(fileMeshData ? fileMeshData : kDummyMeshData);
		}

		// handle body parts
		if (Humanoid* humanoid = getHumanoid(part))
		{
			RBXASSERT(hi && hi->humanoid == humanoid);
			
			MeshId meshId = getMeshIdForBodyPart(*hi, part, specialShape, flags);
			
			if (!meshId.isNull())
			{
                boost::shared_ptr<FileMeshData> fileMeshData = fetchMesh<MeshContentProvider, FileMeshData, MeshId>(meshId, part, asyncResult);
                return Resources(fileMeshData ? fileMeshData : kDummyMeshData);
			}
		}
		
		// handle head mesh
		if (part->getCookie() & PartCookie::HAS_HEADMESH)
		{
			// Return the default head; scaling is taken care of in getMeshScale
            boost::shared_ptr<FileMeshData> fileMeshData = fetchMesh<MeshContentProvider, FileMeshData, MeshId>(MeshId("rbxasset://fonts/head.mesh"), part, asyncResult);
            return Resources(fileMeshData ? fileMeshData : kDummyMeshData);
		}

        if (FFlag::StudioCSGAssets && !FFlag::CSGLoadBlocking)
        {
            if (PartOperation* operation = part->fastDynamicCast<PartOperation>())
            {
                if (operation->hasAsset())
                {
                    shared_ptr<PartOperationAsset> partOperationAsset = fetchMesh<SolidModelContentProvider, PartOperationAsset, ContentId>(operation->getAssetId(), part, asyncResult);

                    if (partOperationAsset)
                        return Resources(partOperationAsset->getRenderMesh());
                }
            }
        }

        // no mesh
		return Resources();
	}
	
	GeometryGenerator::GeometryGenerator()
		: mVertices(NULL)
		, mIndices(NULL)
		, mVertexCount(0)
		, mIndexCount(0)
		, mBboxMin(Vector3::maxFinite())
		, mBboxMax(Vector3::minFinite())
	{
	}

	GeometryGenerator::GeometryGenerator(Vertex* vertices, unsigned short* indices, unsigned int vertexOffset)
		: mVertices(vertices)
		, mIndices(indices)
		, mVertexCount(vertexOffset)
		, mIndexCount(0)
		, mBboxMin(Vector3::maxFinite())
		, mBboxMax(Vector3::minFinite())
	{
		RBXASSERT(vertices && indices);
	}

	void GeometryGenerator::reset()
	{
		mVertexCount = 0;
		mIndexCount = 0;
		
		resetBounds();
	}
	
	void GeometryGenerator::resetBounds()
	{
		mBboxMin = Vector3::maxFinite();
		mBboxMax = Vector3::minFinite();
	}
	
	void GeometryGenerator::resetBounds(const Vector3& min, const Vector3& max)
	{
		mBboxMin = min;
		mBboxMax = max;
	}
	
	void GeometryGenerator::addInstance(PartInstance* part, Decal* decal, const Options& options, const Resources& resources, const HumanoidIdentifier* humanoidIdentifier /*=NULL*/)
	{
		addPartImpl(part, decal, options, resources, humanoidIdentifier, /* ignoreMaterialsStuds= */ false);
		
		// check that our indices did not overflow (caller should ensure this before calling addDecal)
		RBXASSERT(!mVertices || mVertexCount < (1 << 16));
	}

    static bool shouldRandomizeColor(VisualEngine* visualEngine, const PartInstance& part, Decal* decal)
    {
        bool randomizeColor = false;
        if (Lighting* lighting = visualEngine->getLighting())
            randomizeColor = lighting->getOutlines();

        if (randomizeColor)
        {
            if (decal && decal->isA<DecalTexture>())
                randomizeColor = false;
            else
            {
                randomizeColor = false;
                for (unsigned i = 0; i < NORM_UNDEFINED; ++i)
                    if (part.getSurfaceType((NormalId)i) != NO_SURFACE_NO_OUTLINES)
                    {
                        randomizeColor = true;
                        break;
                    }
            }  
        }

        return randomizeColor;
    }
    
	GeometryGenerator::Options::Options(VisualEngine* visualEngine, const PartInstance& part, Decal* decal, const CoordinateFrame& localTransform, unsigned int materialFlags, const Vector4& uvOffsetScale, unsigned int extra)
	{
        unsigned int randomizeFlag = 0;

        if (FFlag::NoRandomColorsWithoutOutlines)
        {
            randomizeFlag = 
                shouldRandomizeColor(visualEngine, part, decal)
                ? GeometryGenerator::Flag_RandomizeBlockAppearance
                : 0;   
        }
        else
        {
            randomizeFlag = 
            decal && decal->isA<DecalTexture>()
            ? 0
            : GeometryGenerator::Flag_RandomizeBlockAppearance;
        }

		unsigned int colorFlag =
			(materialFlags & MaterialGenerator::Result_UsesCompositTexture)
			? GeometryGenerator::Flag_VertexColorMesh // composited parts w/meshes and accoutrements need vertex color, but part color is baked in
			: (materialFlags & MaterialGenerator::Result_UsesTexture)
			? GeometryGenerator::Flag_VertexColorMesh  // meshes w/texture need vertex color, part color does not apply
			: GeometryGenerator::Flag_VertexColorPart; // no texture means that vertex color does not have any effect

		unsigned int colorOrderFlag =
			visualEngine->getDevice()->getCaps().colorOrderBGR ? GeometryGenerator::Flag_DiffuseColorBGRA : 0;

		unsigned int studFlag =
			(visualEngine->getSettings()->getDrawConnectors() || part.getRenderMaterial() == RBX::PLASTIC_MATERIAL)
			? 0
			: GeometryGenerator::Flag_IgnoreStuds;

		unsigned int extraFlag =
			visualEngine->getRenderCaps()->getSkinningBoneCount() > 0 ? 0 : GeometryGenerator::Flag_ExtraIsZero;

		unsigned int flags = randomizeFlag | colorFlag | colorOrderFlag | studFlag | extraFlag;

		this->flags = flags;
		this->extra = extra;

		if (uvOffsetScale != G3D::Vector4(0.f, 0.f, 1.f, 1.f))
		{
			this->flags |= GeometryGenerator::Flag_TransformUV;
			this->uvframe = uvOffsetScale;
		}

		this->cframe = localTransform;
	}

    void GeometryGenerator::addCSGPrimitive(PartInstance* part)
	{	
        Graphics::GeometryGenerator::Options options;
		options.flags |= Graphics::GeometryGenerator::Flag_VertexColorPart;

        // Setting the gfx cookie artificially so that the special mesh will
        // be searched for.  We need to do this because this part may not
        // have been through the renderer yet to receive the cookie.
        unsigned int oldCookie = part->getCookie();
        part->setCookie(oldCookie | PartCookie::HAS_SPECIALSHAPE);

        // Trusses are not manifold so skip them for now.
        if (part->getPartType() == TRUSS_PART)
        {
            return;
        }

        addPartImpl(part, NULL, options, Resources(), /*humanoidIdentifier=*/ NULL, /* ignoreMaterialsStuds= */ true);

        part->setCookie(oldCookie);
	}

    void GeometryGenerator::addOperation(PartInstance* part, Decal* decal, const Options& options, const Resources& resources, unsigned int randomSeed)
	{
        bool renderCollision = PartOperation::renderCollisionData;

		PartOperation* operation = part->fastDynamicCast<PartOperation>();

		if (!operation)
			return;

        shared_ptr<CSGMesh> csgMesh = resources.csgMeshData;

        if (!csgMesh)
        {
            if (!FFlag::StudioCSGAssets || FFlag::CSGLoadBlocking)
            {
                if (!operation->getMesh())
                    return;

                if (operation->getMesh()->isBadMesh())
                    return;

                csgMesh = operation->getMesh();
            }
            else
            {
                if (operation->getRenderMesh() && !operation->getRenderMesh()->isBadMesh())
                {
                    csgMesh = operation->getRenderMesh();
                }
            }
        }
                
        if (!csgMesh)
            return;

		if (renderCollision)
		{
			addOperationDecompositionDebug(operation, part, decal, options, randomSeed);
			return;
		}

		size_t vertexOffset = mVertexCount;
		size_t indexOffset = mIndexCount;
		CSGMesh* modelData = csgMesh.get();

        const std::vector<CSGVertex>& meshVertices = modelData->getVertices();
        const std::vector<unsigned int>& meshIndices = modelData->getIndices();

        if (!FFlag::FixGlowingCSG || !decal)
        {
		mVertexCount += meshVertices.size();
		mIndexCount += meshIndices.size();
        }
        else
        {
            mVertexCount += csgMesh->getVertexRemap(decal->getFace()).size();
            mIndexCount += csgMesh->getIndexRemap(decal->getFace()).size();
        }

		if (!mVertices) return;

		Vertex* vertices = mVertices;
		unsigned short* indices = mIndices;

		Vector3 size = part->getPartSizeXml();
		RBX::Color4uint8 partColor = getColor(part, decal, NULL, options, randomSeed, true);
        const RBX::Color4uint8& extra = getExtra(part, options);

        bool isNegate = part->fastDynamicCast<NegateOperation>() != 0;
        bool usePartColor = operation->getUsePartColor();

        if (isNegate)
        {
            partColor = RBX::Color4uint8(255, 154, 161, 150);
		    if ((options.flags & GeometryGenerator::Flag_DiffuseColorBGRA) != 0)
                std::swap(partColor.r, partColor.b);
        }

        Vector3 scale = operation->getSizeDifference();
		const CoordinateFrame& cframe = options.cframe;

		Vector2 surfaceOffset = getSurfaceOffset(size, randomSeed, part->getRenderMaterial());
        float surfaceTiling = decal ? 1 : getSurfaceTiling(part->getRenderMaterial());
        Vector3 invSize = Vector3(1.0f/size.x, 1.0f/size.y, 1.0f/size.z);
        bool isScaled = operation->getInitialSize() != operation->getPartSizeXml();
		Vector2 uvStuds; // blank for now

        if (!FFlag::FixGlowingCSG)
        {
		for (size_t vi = 0; vi < meshVertices.size(); vi++)
		{
            const CSGVertex& vert = meshVertices[vi];
			Vector3 pos = vert.pos * scale;
			Vector3 normal = vert.normal;
            Vector3 tangent = vert.tangent;
            Color4uint8 vertColor = partColor;
           
            if (!isNegate && !usePartColor && !decal)
			{
				vertColor = meshVertices[vi].color;
				vertColor.a = partColor.a;
				if ((options.flags & GeometryGenerator::Flag_DiffuseColorBGRA) != 0)
					std::swap(vertColor.r, vertColor.b);
            }

            Vector2 uv = vert.uv;
            
            if (isScaled)
                uv = vert.generateUv(pos);

            if (decal)
            {
                if (vert.extra.r - 1 != decal->getFace())
                {
                    uv = Vector2();
                    vertColor.a = 0.0f;
                }
                else
                {
                    DecalTexture* texture = decal->fastDynamicCast<DecalTexture>();
                    if (texture)
                    {
                        uv /= texture->getStudsPerTile();
                    }
                    else
                    {
                        switch (decal->getFace())
                        {
                            case NORM_X:
                            case NORM_X_NEG:
                                uv = uv * Vector2(invSize.z, invSize.y) + Vector2(0.5f, 0.5f);
                                break;
                            case NORM_Y:
                            case NORM_Y_NEG:
                                uv = uv * Vector2(invSize.x, invSize.z) + Vector2(0.5f, 0.5f);
                                break;
                            case NORM_Z:
                            case NORM_Z_NEG:
                                uv = uv * Vector2(invSize.x, invSize.y) + Vector2(0.5f, 0.5f);
                                break;
                            default: break;
                        }
                    }
                }
            }
            else
            {
                uv = Vector2(uv.x * surfaceTiling + surfaceOffset.x,
                             uv.y * surfaceTiling + surfaceOffset.y);
            }

            pos = cframe.pointToWorldSpace(pos);
            normal = cframe.vectorToWorldSpace(normal);
			tangent = cframe.vectorToWorldSpace(tangent);

            fillVertex(vertices[vertexOffset + vi], pos, normal, uv, uvStuds, vertColor /*Color4uint8(rand() % 255, rand() % 255, rand() % 255, 255)*/, extra, tangent);
		}

		for (size_t ii = 0; ii < meshIndices.size(); ii++)
		{
            indices[indexOffset + ii] = vertexOffset + meshIndices[ii];
		}
        }
        else
        {
            unsigned numberOfVerts = decal ? csgMesh->getVertexRemap(decal->getFace()).size() : meshVertices.size();
            unsigned int vertId = 0;
            for (size_t vi = 0; vi < numberOfVerts; vi++)
            {
                const CSGVertex& vert = decal ? meshVertices[csgMesh->getVertexRemap(decal->getFace())[vi]] : meshVertices[vi];

                Vector3 pos = vert.pos * scale;
                Vector3 normal = vert.normal;
                Vector3 tangent = vert.tangent;
                Color4uint8 vertColor = partColor;

                if (!isNegate && !usePartColor && !decal)
                {
                    vertColor = meshVertices[vi].color;
                    vertColor.a = partColor.a;
                    if ((options.flags & GeometryGenerator::Flag_DiffuseColorBGRA) != 0)
                        std::swap(vertColor.r, vertColor.b);
                }

                Vector2 uv = vert.uv;

                if (isScaled)
                    uv = vert.generateUv(pos);

                if (decal)
                {
                    DecalTexture* texture = decal->fastDynamicCast<DecalTexture>();
                    if (texture)
                    {
                        uv /= texture->getStudsPerTile();
                    }
                    else
                    {
                        switch (decal->getFace())
                        {
                        case NORM_X:
                        case NORM_X_NEG:
                            uv = uv * Vector2(invSize.z, invSize.y) + Vector2(0.5f, 0.5f);
                            break;
                        case NORM_Y:
                        case NORM_Y_NEG:
                            uv = uv * Vector2(invSize.x, invSize.z) + Vector2(0.5f, 0.5f);
                            break;
                        case NORM_Z:
                        case NORM_Z_NEG:
                            uv = uv * Vector2(invSize.x, invSize.y) + Vector2(0.5f, 0.5f);
                            break;
                        default: break;
                        } 
                    }
                }
                else
                {
                    uv = Vector2(uv.x * surfaceTiling + surfaceOffset.x,
                                 uv.y * surfaceTiling + surfaceOffset.y);
                }

                pos = cframe.pointToWorldSpace(pos);
                normal = cframe.vectorToWorldSpace(normal);
                tangent = cframe.vectorToWorldSpace(tangent);

                fillVertex(vertices[vertexOffset + vertId], pos, normal, uv, uvStuds, vertColor , extra, tangent);
                vertId++;
            }

            if (!decal)
            {
                for (size_t ii = 0; ii < meshIndices.size(); ii++)
                    indices[indexOffset + ii] = vertexOffset + meshIndices[ii];
            }
            else
            {
                const std::vector<unsigned>& indexFaceRamep = csgMesh->getIndexRemap(decal->getFace());

                for (size_t ii = 0; ii < indexFaceRamep.size(); ii++)
                    indices[indexOffset + ii] = vertexOffset + indexFaceRamep[ii];
            }
        }

		Vector3 boundsCenter = cframe.pointToWorldSpace(RBX::Vector3::zero());
		Vector3 axisX = Vector3(cframe.rotation[0][0], cframe.rotation[1][0], cframe.rotation[2][0]);
		Vector3 axisY = Vector3(cframe.rotation[0][1], cframe.rotation[1][1], cframe.rotation[2][1]);
		Vector3 axisZ = Vector3(cframe.rotation[0][2], cframe.rotation[1][2], cframe.rotation[2][2]);
		extendBoundsBlock(mBboxMin, mBboxMax, boundsCenter, part->getPartSizeXml(), axisX, axisY, axisZ);
	}

	void GeometryGenerator::addOperationDecompositionDebug(PartOperation* operation, PartInstance* part, Decal* decal, const Options& options, unsigned int randomSeed)
	{
		if (FFlag::CSGPhysicsLevelOfDetailEnabled && operation->getCollisionFidelity() == CollisionFidelity_Box)
		{
			fillBlockShapeDebug(part, options);
			return;
		}

		fillDecompShapeDebug(operation, part, options);
	}

	void GeometryGenerator::fillBlockShapeDebug(PartInstance* part, const Options &options)
	{
		size_t vertexOffset = mVertexCount;
		size_t indexOffset = mIndexCount;

		LcmRand randGen;
		randGen.setSeed(static_cast<unsigned>(reinterpret_cast<long>(part)));

		unsigned int numfaces = 6;
		mVertexCount += numfaces*4;
		mIndexCount += numfaces*6;

		Color4uint8 color = RBX::Color4uint8(randGen.value() % 255, randGen.value() % 255, randGen.value() % 255, 100);
		Color4uint8 extra = getExtra(part, options);

		const Vector3 size = part->getPartSizeUi();
		const CoordinateFrame& cframe = options.cframe;
		const Vector3 center = Vector3::zero();

		if (!mVertices) return;

		Vertex* vertices = mVertices;
		unsigned short* indices = mIndices;

		Vector3 axisX = Vector3(cframe.rotation[0][0], cframe.rotation[1][0], cframe.rotation[2][0]);
		Vector3 axisY = Vector3(cframe.rotation[0][1], cframe.rotation[1][1], cframe.rotation[2][1]);
		Vector3 axisZ = Vector3(cframe.rotation[0][2], cframe.rotation[1][2], cframe.rotation[2][2]);

		Vector3 extent = size * 0.5f;

		Vector3 cornerO = cframe.pointToWorldSpace(Vector3(center - extent));
		Vector3 cornerX = axisX * size.x;
		Vector3 cornerY = axisY * size.y;
		Vector3 cornerZ = axisZ * size.z;

		Vector3 corners[8] =
		{
			cornerO,
			cornerO + cornerZ,
			cornerO + cornerY,
			cornerO + cornerY + cornerZ,
			cornerO + cornerX,
			cornerO + cornerX + cornerZ,
			cornerO + cornerX + cornerY,
			cornerO + cornerX + cornerY + cornerZ,
		};

		Vector3 normal;  // Set Normals to Z
		normal.x = 0;
		normal.y = 0;
		normal.z = 1;
		Vector3 tangent;
		tangent.x = 0;
		tangent.y = 0;
		tangent.z = 1;	// Set Normals to Z

		Vector2 uv;
		uv.x = 0;
		uv.y = 0;

		static const unsigned int faces[6][4] =
		{
			{6, 7, 4, 5},
			{3, 7, 2, 6},
			{7, 3, 5, 1},
			{3, 2, 1, 0},
			{5, 1, 4, 0},
			{2, 6, 0, 4},
		};

		for (unsigned int face = 0; face < numfaces; face++)
		{
			unsigned int internalOffset = face*4;
			fillVertex(vertices[internalOffset + vertexOffset + 0], corners[faces[face][0]], normal, uv,uv, color, extra, Vector3());
			fillVertex(vertices[internalOffset + vertexOffset + 1], corners[faces[face][1]], normal, uv,uv, color, extra, Vector3());
			fillVertex(vertices[internalOffset + vertexOffset + 2], corners[faces[face][2]], normal, uv,uv, color, extra, Vector3());
			fillVertex(vertices[internalOffset + vertexOffset + 3], corners[faces[face][3]], normal, uv,uv, color, extra, Vector3());

			unsigned int internalIndexOffset = face*6;
			fillQuadIndices(&indices[internalIndexOffset + indexOffset], internalOffset + vertexOffset, 0, 1, 2, 3);
		}
		Vector3 boundsCenter = cframe.pointToWorldSpace(RBX::Vector3::zero());
		extendBoundsBlock(mBboxMin, mBboxMax, boundsCenter, part->getPartSizeXml(), axisX, axisY, axisZ);
	}

	void GeometryGenerator::fillDecompShapeDebug(PartOperation* operation, PartInstance* part, const Options &options)
	{
		Vector3 scale;
		int currentVersion = 0;

		if (operation->getPrimitive(operation)->getGeometry()->getGeometryType() != RBX::Geometry::GEOMETRY_TRI_MESH)
			return;

		size_t vertexOffset = mVertexCount;
		size_t indexOffset = mIndexCount;
		int childIndexOffset = 0;
		int childVertexOffset = 0;

		if (operation->getNonKeyPhysicsData() == "")
			return;

		btVector3 btScale; 
		std::vector<CSGConvex> meshConvexes = TriangleMesh::getDecompConvexes(operation->getNonKeyPhysicsData(), currentVersion, btScale, false);

		if (currentVersion == 0)
			return;

		for (unsigned int i = 0; i < meshConvexes.size(); i++)
		{
			mVertexCount += meshConvexes[i].vertices.size();
			mIndexCount += meshConvexes[i].indices.size();
		}

		if (!mVertices) return;


		Vertex* vertices = mVertices;
		unsigned short* indices = mIndices;

		const RBX::Color4uint8& extra = getExtra(part, options);
		const CoordinateFrame& cframe = options.cframe;

		Vector3 axisX = Vector3(cframe.rotation[0][0], cframe.rotation[1][0], cframe.rotation[2][0]);
		Vector3 axisY = Vector3(cframe.rotation[0][1], cframe.rotation[1][1], cframe.rotation[2][1]);
		Vector3 axisZ = Vector3(cframe.rotation[0][2], cframe.rotation[1][2], cframe.rotation[2][2]);

		scale = operation->calculateSizeDifference(operation->getPartSizeXml());

		LcmRand randGen;
		randGen.setSeed(static_cast<unsigned>(reinterpret_cast<long>(operation)));

		for (unsigned int i = 0; i < meshConvexes.size(); i++)
		{
			const RBX::Color4uint8 color = RBX::Color4uint8(randGen.value() % 255, randGen.value() % 255, randGen.value() % 255, 100);

			for (unsigned int j = 0; j < meshConvexes[i].vertices.size(); j++)
			{
				Vector3 pos;
				btVector3 offsetVertex = meshConvexes[i].transform * meshConvexes[i].vertices[j];
				pos.x = offsetVertex.x();
				pos.y = offsetVertex.y();
				pos.z = offsetVertex.z();

				Vector3 normal;  // Set Normals to Z
				normal.x = 0;
				normal.y = 0;
				normal.z = 1;
				Vector3 tangent;
				tangent.x = 0;
				tangent.y = 0;
				tangent.z = 1;	// Set Normals to Z

				Vector2 uv;
				uv.x = 0;
				uv.y = 0;

				pos *= scale;


				pos = cframe.pointToWorldSpace(pos);
				normal = cframe.vectorToWorldSpace(normal);
				tangent = cframe.vectorToWorldSpace(tangent);
				fillVertex(vertices[vertexOffset + childVertexOffset + j], pos, normal, uv,uv, color, extra, tangent);
			}
			for (unsigned int j = 0; j < meshConvexes[i].indices.size(); j++)
			{
				indices[indexOffset + childIndexOffset + j] = vertexOffset + childVertexOffset + meshConvexes[i].indices[j];
			}
			childIndexOffset += meshConvexes[i].indices.size();
			childVertexOffset += meshConvexes[i].vertices.size();
		}
		Vector3 boundsCenter = cframe.pointToWorldSpace(RBX::Vector3::zero());
		extendBoundsBlock(mBboxMin, mBboxMax, boundsCenter, part->getPartSizeXml(), axisX, axisY, axisZ);
	}
}
}

