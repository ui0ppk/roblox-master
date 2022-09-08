#include "stdafx.h"

#include "Voxel/Voxelizer.h"
#include "rbx/DenseHash.h"
#include "v8world/Primitive.h"
#include "v8world/ContactManager.h"
#include "v8world/TriangleMesh.h"
#include "v8datamodel/MegaCluster.h"
#include "v8datamodel/DataModelMesh.h"
#include "v8datamodel/PartInstance.h"
#include "v8datamodel/BlockMesh.h"
#include "v8datamodel/CylinderMesh.h"
#include "v8datamodel/SpecialMesh.h"
#include "v8datamodel/PartCookie.h"
#include "v8datamodel/PartOperation.h"

#include "voxel2/Grid.h"

#include "rbx/Profiler.h"

#include "BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h"
#include "Extras/GIMPACTUtils/btGImpactConvexDecompositionShape.h"

#if defined(__APPLE__)
#include "TargetConditionals.h"
#endif

#if (defined(RBX_PLATFORM_IOS) && !TARGET_IPHONE_SIMULATOR) || defined(__ANDROID__)
#include <arm_neon.h>
#endif

LOGVARIABLE(Voxelizer, 0);

FASTINTVARIABLE(CSGVoxelizerFadeRadius, 300);

#if defined(_WIN32) || (defined(__APPLE__) && !defined(RBX_PLATFORM_IOS))
#define SIMD_SSE2
#endif

#if (defined(RBX_PLATFORM_IOS) && !TARGET_IPHONE_SIMULATOR) || defined(__ANDROID__)
#define SIMD_NEON
#endif

#if defined(SIMD_SSE2)
#define SIMDCALL(func, args) (useSIMD ? func##SIMD args : func args)
#define SIMDCALLSSE(func, args) (useSIMD ? func##SIMD args : func args)
#elif defined(SIMD_NEON)
#define SIMDCALL(func, args) (useSIMD ? func##SIMD args : func args)
#define SIMDCALLSSE(func, args) func args
#else
#define SIMDCALL(func, args) func args
#define SIMDCALLSSE(func, args) func args
#endif

// vtbl?_u8 are (erroneously) unavailable for iOS arm64 with Xcode < 6.3
#if defined(__APPLE__) && defined(__aarch64__) && defined(__apple_build_version__) && __apple_build_version__< 6020037
#define SIMD_NEON_VTBL_WORKAROUND
#endif

// branchless equivalent of std::max(0, v)
inline unsigned char max0(int v)
{
	return v & ~(v >> 31);
}

// branchless equivalent of std::min(255, v)
inline unsigned char min255(int v)
{
	return v | ((255 - v) >> 31);
}

inline bool isPartFixed(RBX::PartInstance* part)
{
	return part->getPartPrimitive()->getAnchoredProperty() || part->getPartPrimitive()->computeIsGrounded();
}


namespace RBX { namespace Voxel {

	Extents OccupancyChunk::getChunkExtents() const
	{
		Vector3 min = Vector3(index.x, index.y, index.z) * Vector3(kVoxelChunkSizeXZ, kVoxelChunkSizeY, kVoxelChunkSizeXZ) * 4.f;
		Vector3 max = Vector3(index.x + 1, index.y + 1, index.z + 1) * Vector3(kVoxelChunkSizeXZ, kVoxelChunkSizeY, kVoxelChunkSizeXZ) * 4.f;

		return Extents(min, max);
	}

	Voxelizer::Voxelizer(bool collisionTransparency /* = false */)
		: collisionTransparency(collisionTransparency)
	{
		nonFixedPartsEnabled = true;
		useSIMD = false;

#ifdef SIMD_SSE2
		useSIMD = true;
#endif

#ifdef SIMD_NEON
		useSIMD = true;
#endif
	}

	float Voxelizer::getEffectiveTransparency(PartInstance* part)
	{
		if(collisionTransparency)
			return part->getCanCollide() ? 0.0f : 1.0f;
		else
			return part->getTransparencyUi();
	}

	void Voxelizer::occupancyFillBlock(OccupancyChunk& chunk, const Extents& chunkExtents, const Vector3& extents, const CoordinateFrame& cframe, float transparency, float meshRadius)
	{
		float dotX = fabs(cframe.rotation[0][0]);
		float dotY = fabs(cframe.rotation[1][1]);
		float dotZ = fabs(cframe.rotation[2][2]);

		float dotMin = 1e-4f;
		float dotMax = 1 - dotMin;

		if ((dotX <= dotMin || dotX >= dotMax) && (dotY <= dotMin || dotY >= dotMax) && (dotZ <= dotMin || dotZ >= dotMax))
		{
			occupancyFillBlockDFAA(chunk, chunkExtents, extents, cframe, transparency);
		}
		else
		{
			SIMDCALLSSE(occupancyFillBlockDF, (chunk, chunkExtents, extents, cframe, transparency));
		}
	}

	static Vector3 extentsToWorldSpace(const CoordinateFrame& cframe, const Vector3& extents)
	{
		return Vector3(
			fabs(cframe.rotation[0][0]) * extents[0] + fabs(cframe.rotation[0][1]) * extents[1] + fabs(cframe.rotation[0][2]) * extents[2],
			fabs(cframe.rotation[1][0]) * extents[0] + fabs(cframe.rotation[1][1]) * extents[1] + fabs(cframe.rotation[1][2]) * extents[2],
			fabs(cframe.rotation[2][0]) * extents[0] + fabs(cframe.rotation[2][1]) * extents[1] + fabs(cframe.rotation[2][2]) * extents[2]);
	}

	void Voxelizer::occupancyFillBlockDF(OccupancyChunk& chunk, const Extents& chunkExtents, const Vector3& extents, const CoordinateFrame& cframe, float transparency)
	{
		Vector3 extentsWorldSize = extentsToWorldSpace(cframe, extents);
		AABox extentsWorld = AABox(cframe.translation - extentsWorldSize * 0.5f, cframe.translation + extentsWorldSize * 0.5f);

		Vector3int32 extentsMin = fastFloorInt32((extentsWorld.low() - chunkExtents.min()) / 4.f).max(Vector3int32(0, 0, 0));
		// A small negative offset is added to extentsMax before flooring to prevent updating extra layer of cells for perfectly aligned/sized parts
		Vector3int32 extentsMax = fastFloorInt32((extentsWorld.high() - chunkExtents.min()) / 4.f - Vector3(0.01f, 0.01f, 0.01f)).min(kVoxelChunkSize - Vector3int32(1, 1, 1));

		float thickness = 255.5f * (1 - G3D::clamp(transparency, 0.f, 1.f));

		// Size of the part in cell space
		// We make sure the size in world space is at least 1 to reduce light leaks from thin walls/roofs
		// Additionally, since we only care about the size if it's less than one cell, we clamp this to make the calculations below faster.
		Vector3 extentsAdjusted = extents.max(Vector3(1.0f,1.0f,1.0f));
		Vector3 sizeCellsClamped = (extentsAdjusted / 4.f).min(Vector3(1.0f, 1.0f, 1.0f));
		Vector3 sizeCellsHalfOffset = extentsAdjusted * (0.5f / 4.f) + Vector3(0.5f, 0.5f, 0.5f);

		Vector3 localCorner = cframe.pointToObjectSpace(chunkExtents.min() + Vector3(extentsMin.x + 0.5f, extentsMin.y + 0.5f, extentsMin.z + 0.5f) * 4.f) / 4.f;
		Vector3 localAxisX = Vector3(cframe.rotation[0][0], cframe.rotation[0][1], cframe.rotation[0][2]);
		Vector3 localAxisY = Vector3(cframe.rotation[1][0], cframe.rotation[1][1], cframe.rotation[1][2]);
		Vector3 localAxisZ = Vector3(cframe.rotation[2][0], cframe.rotation[2][1], cframe.rotation[2][2]);

		Vector3 localY = localCorner;

		for (int y = extentsMin.y; y <= extentsMax.y; ++y)
		{
			Vector3 localZ = localY;

			for (int z = extentsMin.z; z <= extentsMax.z; ++z)
			{
				Vector3 localX = localZ;

				for (int x = extentsMin.x; x <= extentsMax.x; ++x)
				{
					float distX = sizeCellsHalfOffset.x - fabsf(localX.x);
					float distY = sizeCellsHalfOffset.y - fabsf(localX.y);
					float distZ = sizeCellsHalfOffset.z - fabsf(localX.z);

					// if distance <-0.5, voxel is completely inside the part
					// if distance >+0.5, voxel is completely outside the part
					// a product of the clamped factors turns out to be equal to the intersection volume for axis-aligned cframes
					// note that we include 0.5 offset in the distXYZ calculation
					float factorX = std::max(0.f, std::min(distX, sizeCellsClamped.x));
					float factorY = std::max(0.f, std::min(distY, sizeCellsClamped.y));
					float factorZ = std::max(0.f, std::min(distZ, sizeCellsClamped.z));

					int factor = static_cast<int>(factorX * factorY * factorZ * thickness);

					chunk.occupancy[y][z][x] = min255(chunk.occupancy[y][z][x] + factor);

					localX += localAxisX;
				}

				localZ += localAxisZ;
			}

			localY += localAxisY;
		}
	}

	static inline float penetrationDepth(float left, float right, float position)
	{
		if (position <= left)
			return left - position;

		if (position >= right)
			return position - right;

		return -std::min(position - left, right - position);
	}

    void Voxelizer::occupancyFillMesh(OccupancyChunk& chunk, const Extents& chunkExtents, const Vector3& extents, const CoordinateFrame& cframe, float transparency, float meshRadius)
    {
        const Vector3 sizeCellsClamped = (extents / 4.f).min(Vector3(1.0f, 1.0f, 1.0f));
        const float sizeCellsClampedMax = std::max(sizeCellsClamped.x, std::max(sizeCellsClamped.y, sizeCellsClamped.z));

        const Vector3 extentsWorldSize = extentsToWorldSpace(cframe, extents);

        const float fadeRadius = static_cast<float>(FInt::CSGVoxelizerFadeRadius);
        const float preCutoff = 0.25f;
        const float fadePower = 1.7f;

        float fadeValue = (meshRadius / fadeRadius) - preCutoff;
        fadeValue = std::max(0.0f, fadeValue);
        float fadeModifier = 1.0f + fadePower * fadeValue * fadeValue;

        const float thickness = 255.5f * (1 - G3D::clamp(transparency, 0.f, 1.f));

        const Vector3 halfLocalExtents = extentsWorldSize * 0.5f;
        const Vector3 aboxCenter = cframe.translation;
        const Vector3 invHalfLocalExtentsSquared = Vector3(1.f / (halfLocalExtents.x*halfLocalExtents.x), 1.f / (halfLocalExtents.y*halfLocalExtents.y), 1.f / (halfLocalExtents.z*halfLocalExtents.z));

        const AABox extentsWorld = AABox(cframe.translation - extentsWorldSize * 0.5f, cframe.translation + extentsWorldSize * 0.5f);

        const Vector3int32 extentsMin = fastFloorInt32((extentsWorld.low() - chunkExtents.min()) / 4.f).max(Vector3int32(0, 0, 0));

        // A small negative offset is added to extentsMax before flooring to prevent updating extra layer of cells for perfectly aligned/sized parts
        const Vector3int32 extentsMax = fastFloorInt32((extentsWorld.high() - chunkExtents.min()) / 4.f - Vector3(0.01f, 0.01f, 0.01f)).min(kVoxelChunkSize - Vector3int32(1, 1, 1));

        const Vector3 worldCorner = chunkExtents.min() + Vector3(extentsMin.x + 0.5f, extentsMin.y + 0.5f, extentsMin.z + 0.5f) * 4.f;
        const Vector3 axisX = Vector3(4.0f,0,0);
        const Vector3 axisY = Vector3(0,4.0f,0);
        const Vector3 axisZ = Vector3(0,0,4.0f);

        Vector3 worldY = worldCorner;

        for (int y = extentsMin.y; y <= extentsMax.y; ++y)
        {
            Vector3 worldZ = worldY;

            for (int z = extentsMin.z; z <= extentsMax.z; ++z)
            {
                Vector3 worldX = worldZ;

                for (int x = extentsMin.x; x <= extentsMax.x; ++x)
                {
                    // Compute an ellipsoidal distance.
                    Vector3 distFromCenter = worldX - aboxCenter;
                    distFromCenter.x = fabs(distFromCenter.x);
                    distFromCenter.y = fabs(distFromCenter.y);
                    distFromCenter.z = fabs(distFromCenter.z);

                    float ellipseX = distFromCenter.x;
                    ellipseX = ellipseX * ellipseX;
                    ellipseX *= invHalfLocalExtentsSquared.x;

                    float ellipseY = distFromCenter.y;
                    ellipseY = ellipseY * ellipseY;
                    ellipseY *= invHalfLocalExtentsSquared.y;

                    float ellipseZ = distFromCenter.z;
                    ellipseZ = ellipseZ * ellipseZ;
                    ellipseZ *= invHalfLocalExtentsSquared.z;

                    float distance = sqrtf(ellipseX + ellipseY + ellipseZ);
                    distance *= fadeModifier;

                    // remap to occupancy range where 0 is empty and 1 is occupied.
                    distance = 1.0f - distance * 0.5f;

                    float factorF = std::max(0.f, std::min(distance, sizeCellsClampedMax));

                    int factor = static_cast<int>(factorF * factorF * factorF * thickness);

                    chunk.occupancy[y][z][x] = min255(chunk.occupancy[y][z][x] + factor);

                    worldX += axisX;
                }

                worldZ += axisZ;
            }

            worldY += axisY;
        }
    }

    void Voxelizer::addMeshToPartCache(std::vector<DataModelPartCache>& partCache, PartInstance* part, OccupancyChunk& chunk, const Extents& chunkExtents, const Vector3& extents, const CoordinateFrame& cframe, float transparency)
    {
        Primitive* primitive = PartInstance::getPrimitive(part);
        Geometry* geometry = primitive->getGeometry();
        
        if (geometry->getGeometryType() != Geometry::GEOMETRY_TRI_MESH)
            return;
        
        TriangleMesh* triangleMesh = rbx_static_cast<TriangleMesh*>(geometry);
        const BulletDecompWrapper* compound = triangleMesh->getCompound();
        
        if (!compound)
            return;
        
		// If there's more than one primitive, expand the bounds to make the box blurriness overlap.
		float expansionFactor = (compound->getExtentArray().size() > 1) ? 1.3f : 1.f;

		for (auto& e: compound->getExtentArray())
		{
			CoordinateFrame ccf(cframe.rotation, cframe.translation + cframe.rotation * e.center);
			
            partCache.push_back(DataModelPartCache(&Voxelizer::occupancyFillMesh, chunk, e.size * expansionFactor, ccf, transparency, extents.magnitude()));
		}
    }

	void Voxelizer::occupancyFillBlockDFAA(OccupancyChunk& chunk, const Extents& chunkExtents, const Vector3& extents_, const CoordinateFrame& cframe, float transparency)
	{
		Vector3 extentsWorldSize = extentsToWorldSpace(cframe, extents_);
		AABox extentsWorld = AABox(cframe.translation - extentsWorldSize * 0.5f, cframe.translation + extentsWorldSize * 0.5f);

		Vector3 extents = extentsWorldSize;

		Vector3int32 extentsMin = fastFloorInt32((extentsWorld.low() - chunkExtents.min()) / 4.f).max(Vector3int32(0, 0, 0));
		// A small negative offset is added to extentsMax before flooring to prevent updating extra layer of cells for perfectly aligned/sized parts
		Vector3int32 extentsMax = fastFloorInt32((extentsWorld.high() - chunkExtents.min()) / 4.f - Vector3(0.01f, 0.01f, 0.01f)).min(kVoxelChunkSize - Vector3int32(1, 1, 1));

		float thickness = 255.5f * (1 - G3D::clamp(transparency, 0.f, 1.f));

		// Size of the part in cell space
		// We make sure the size in world space is at least 1 to reduce light leaks from thin walls/roofs
		// Additionally, since we only care about the size if it's less than one cell, we clamp this to make the calculations below faster.
		Vector3 extentsAdjusted = extents.max(Vector3(1.0f,1.0f,1.0f));
		Vector3 sizeCellsClamped = (extentsAdjusted / 4.f).min(Vector3(1.0f, 1.0f, 1.0f));
		Vector3 sizeCellsHalfOffset = extentsAdjusted * (0.5f / 4.f) + Vector3(0.5f, 0.5f, 0.5f);

		Vector3 localCorner = (chunkExtents.min() + Vector3(extentsMin.x + 0.5f, extentsMin.y + 0.5f, extentsMin.z + 0.5f) * 4.f - cframe.translation) / 4.f;

		float localY = localCorner.y;

		for (int y = extentsMin.y; y <= extentsMax.y; ++y)
		{
			float localZ = localCorner.z;

			float distY = sizeCellsHalfOffset.y - fabsf(localY);
			float factorY = std::max(0.f, std::min(distY, sizeCellsClamped.y));
			float factorYT = factorY * thickness;

			for (int z = extentsMin.z; z <= extentsMax.z; ++z)
			{
				float localX = localCorner.x;

				float distZ = sizeCellsHalfOffset.z - fabsf(localZ);
				float factorZ = std::max(0.f, std::min(distZ, sizeCellsClamped.z));

				float factorYZT = factorZ * factorYT;

				if (extentsMax.x - extentsMin.x < 3)
				{
					for (int x = extentsMin.x; x <= extentsMax.x; ++x)
					{
						float distX = sizeCellsHalfOffset.x - fabsf(localX);
						float factorX = std::max(0.f, std::min(distX, sizeCellsClamped.x));

						int factor = static_cast<int>(factorX * factorYZT);

						chunk.occupancy[y][z][x] = min255(chunk.occupancy[y][z][x] + factor);

						localX += 1;
					}
				}
				else
				{
					// Split the loop
					{
						int x = extentsMin.x;

						float distX = sizeCellsHalfOffset.x - fabsf(localX);
						float factorX = std::max(0.f, std::min(distX, sizeCellsClamped.x));

						int factor = static_cast<int>(factorX * factorYZT);

						chunk.occupancy[y][z][x] = min255(chunk.occupancy[y][z][x] + factor);

						localX += 1;
					}

					int factorMiddle = static_cast<int>(factorYZT);

					for (int x = extentsMin.x + 1; x < extentsMax.x; ++x)
					{
						chunk.occupancy[y][z][x] = min255(chunk.occupancy[y][z][x] + factorMiddle);

						localX += 1;
					}

					{
						int x = extentsMax.x;

						float distX = sizeCellsHalfOffset.x - fabsf(localX);
						float factorX = std::max(0.f, std::min(distX, sizeCellsClamped.x));

						int factor = static_cast<int>(factorX * factorYZT);

						chunk.occupancy[y][z][x] = min255(chunk.occupancy[y][z][x] + factor);

						localX += 1;
					}
				}

				localZ += 1;
			}

			localY += 1;
		}
	}

#ifdef SIMD_SSE2
	__m128i fastFloorInt4Clamp0(__m128 v)
	{
		// do integer max instead of fp max since _mm_cvtps_epi32 can return INT_MIN for fp specials
		__m128i r = _mm_cvttps_epi32(v);

		// r >> 31 is 0xffffffff if r is negative
		return _mm_andnot_si128(_mm_srai_epi32(r, 31), r);
	}

	void Voxelizer::occupancyFillBlockDFSIMD(OccupancyChunk& chunk, const Extents& chunkExtents, const Vector3& extents_, const CoordinateFrame& cframe, float transparency)
	{
#define VSPLAT(v, c) _mm_shuffle_ps(v, v, _MM_SHUFFLE(c,c,c,c))

		__m128 cframeTranslation = _mm_loadu_ps(&cframe.translation.x);
		__m128 cframeRotation0 = _mm_loadu_ps(cframe.rotation[0]);
		__m128 cframeRotation1 = _mm_loadu_ps(cframe.rotation[1]);
		__m128 cframeRotation2 = _mm_loadu_ps(cframe.rotation[2]);

		__m128 extents = _mm_loadu_ps(&extents_.x);
		__m128 chunkOffset = _mm_loadu_ps(&chunkExtents.min().x);

		__m128 zero = _mm_setzero_ps();
		__m128 one = _mm_set1_ps(1.f);
		__m128 half = _mm_set1_ps(0.5f);
		__m128 nosign = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));

		__m128 cframeRotationC0 = cframeRotation0;
		__m128 cframeRotationC1 = cframeRotation1;
		__m128 cframeRotationC2 = cframeRotation2;
		__m128 cframeRotationC3 = zero;

		_MM_TRANSPOSE4_PS(cframeRotationC0, cframeRotationC1, cframeRotationC2, cframeRotationC3);

		__m128 extentsHalf = _mm_mul_ps(extents, half);
		__m128 extentsHalfRotated =
			_mm_add_ps(
			_mm_mul_ps(_mm_and_ps(cframeRotationC0, nosign), VSPLAT(extentsHalf, 0)),
			_mm_add_ps(
			_mm_mul_ps(_mm_and_ps(cframeRotationC1, nosign), VSPLAT(extentsHalf, 1)),
			_mm_mul_ps(_mm_and_ps(cframeRotationC2, nosign), VSPLAT(extentsHalf, 2))));

		__m128 extentsWorldLow = _mm_sub_ps(cframeTranslation, extentsHalfRotated);
		__m128 extentsWorldHigh = _mm_add_ps(cframeTranslation, extentsHalfRotated);

		__m128 chunkExtentsLimit = _mm_setr_ps(kVoxelChunkSizeXZ-1, kVoxelChunkSizeY-1, kVoxelChunkSizeXZ-1, 0);

		__m128i extentsMin = fastFloorInt4Clamp0(_mm_mul_ps(_mm_sub_ps(extentsWorldLow, chunkOffset), _mm_set1_ps(0.25f)));
		// A small negative offset is added to extentsMax before flooring to prevent updating extra layer of cells for perfectly aligned/sized parts
		__m128i extentsMax = fastFloorInt4Clamp0(_mm_min_ps(_mm_sub_ps(_mm_mul_ps(_mm_sub_ps(extentsWorldHigh, chunkOffset), _mm_set1_ps(0.25f)), _mm_set1_ps(0.001f)), chunkExtentsLimit));

		__m128 thickness = _mm_set1_ps(255.5f * (1 - G3D::clamp(transparency, 0.f, 1.f)));

		// Size of the part in cell space
		// We make sure the size in world space is at least 1 to reduce light leaks from thin walls/roofs
		// Additionally, since we only care about the size if it's less than one cell, we clamp this to make the calculations below faster.
		__m128 extentsAdjusted = _mm_max_ps(extents, one);
		__m128 sizeCellsClamped = _mm_min_ps(_mm_mul_ps(extentsAdjusted, _mm_set1_ps(0.25f)), one);
		__m128 sizeCellsHalfOffset = _mm_add_ps(_mm_mul_ps(extentsAdjusted, _mm_set1_ps(0.5f / 4.f)), half);

		__m128 worldCorner = _mm_add_ps(chunkOffset, _mm_mul_ps(_mm_add_ps(_mm_cvtepi32_ps(extentsMin), half), _mm_set1_ps(4.f)));
		__m128 localCornerNoRot = _mm_sub_ps(worldCorner, cframeTranslation);
		__m128 localCornerRot =
			_mm_add_ps(
			_mm_mul_ps(cframeRotation0, VSPLAT(localCornerNoRot, 0)),
			_mm_add_ps(
			_mm_mul_ps(cframeRotation1, VSPLAT(localCornerNoRot, 1)),
			_mm_mul_ps(cframeRotation2, VSPLAT(localCornerNoRot, 2))));

		__m128 localCorner = _mm_mul_ps(localCornerRot, _mm_set1_ps(0.25f));
		__m128 localAxisX = cframeRotation0;
		__m128 localAxisY = cframeRotation1;
		__m128 localAxisZ = cframeRotation2;

		__m128 localY = localCorner;

		int extentsMinBuf[4], extentsMaxBuf[4];
		_mm_storeu_si128((__m128i*)extentsMinBuf, extentsMin);
		_mm_storeu_si128((__m128i*)extentsMaxBuf, extentsMax);

		for (int y = extentsMinBuf[1]; y <= extentsMaxBuf[1]; ++y)
		{
			__m128 localZ = localY;

			for (int z = extentsMinBuf[2]; z <= extentsMaxBuf[2]; ++z)
			{
				__m128 localX = localZ;

				for (int x = extentsMinBuf[0]; x <= extentsMaxBuf[0]; ++x)
				{
					__m128i occ = _mm_loadu_si128((__m128i*)&chunk.occupancy[y][z][x]);
					__m128 dist = _mm_sub_ps(sizeCellsHalfOffset, _mm_and_ps(localX, nosign));

					// if distance <-0.5, voxel is completely inside the part
					// if distance >+0.5, voxel is completely outside the part
					// a product of the clamped factors turns out to be equal to the intersection volume for axis-aligned cframes
					// note that we include 0.5 offset in the distXYZ calculation
					__m128 factors = _mm_max_ps(zero, _mm_min_ps(dist, sizeCellsClamped));

					__m128 factor =
						_mm_mul_ps(
						_mm_mul_ps(VSPLAT(factors, 0), VSPLAT(factors, 1)),
						_mm_mul_ps(VSPLAT(factors, 2), thickness));

					__m128i occres = _mm_adds_epu8(occ, _mm_cvttps_epi32(factor));

					chunk.occupancy[y][z][x] = _mm_cvtsi128_si32(occres);

					localX = _mm_add_ps(localX, localAxisX);
				}

				localZ = _mm_add_ps(localZ, localAxisZ);
			}

			localY = _mm_add_ps(localY, localAxisY);
		}

#undef VSPLAT
	}
#endif

	struct SphereDistanceFunction
	{
		float radius;

		SphereDistanceFunction(const Vector3& extents)
		{
			radius = extents.x / 2;
		}

		Vector3 operator()(const Vector3& point) const
		{
			float r = sqrtf(point.x * point.x + point.y * point.y + point.z * point.z);
			float d = r - radius;

			return Vector3(d, d, d);
		}
	};

	void Voxelizer::occupancyFillSphere(OccupancyChunk& chunk, const Extents& chunkExtents, const Vector3& extents, const CoordinateFrame& cframe, float transparency, float meshRadius)
	{
		SphereDistanceFunction df(extents);

		occupancyFillDF(chunk, chunkExtents, extents, cframe, transparency, df);
	}

	struct EllipsoidDistanceFunction
	{
		Vector3 radius;
		Vector3 invRadiusSq;

		EllipsoidDistanceFunction(const Vector3& extents)
		{
			radius = extents / 2;
			invRadiusSq = Vector3(1 / (radius.x * radius.x), 1 / (radius.y * radius.y), 1 / (radius.z * radius.z));
		}

		Vector3 operator()(const Vector3& point) const
		{
			float r = sqrtf(point.x * point.x * invRadiusSq.x + point.y * point.y * invRadiusSq.y + point.z * point.z * invRadiusSq.z);

			return Vector3((r - 1) * radius.x, (r - 1) * radius.y, (r - 1) * radius.z);
		}
	};

	void Voxelizer::occupancyFillEllipsoid(OccupancyChunk& chunk, const Extents& chunkExtents, const Vector3& extents, const CoordinateFrame& cframe, float transparency, float meshRadius)
	{
		EllipsoidDistanceFunction df(extents);

		occupancyFillDF(chunk, chunkExtents, extents, cframe, transparency, df);
	}

	struct CylinderXDistanceFunction
	{
		float radius;
		float halfLength;

		CylinderXDistanceFunction(const Vector3& extents)
		{
			radius = std::min(extents.y, extents.z) / 2;
			halfLength = extents.x / 2;
		}

		Vector3 operator()(const Vector3& point) const
		{
			float r = sqrtf(point.y * point.y + point.z * point.z);
			float d = r - radius;

			return Vector3(fabsf(point.x) - halfLength, d, d);
		}
	};

	void Voxelizer::occupancyFillCylinderX(OccupancyChunk& chunk, const Extents& chunkExtents, const Vector3& extents, const CoordinateFrame& cframe, float transparency, float meshRadius)
	{
		CylinderXDistanceFunction df(extents);

		occupancyFillDF(chunk, chunkExtents, extents, cframe, transparency, df);
	}

	struct CylinderYDistanceFunction
	{
		float radius;
		float halfLength;

		CylinderYDistanceFunction(const Vector3& extents)
		{
			radius = std::min(extents.x, extents.z) / 2;
			halfLength = extents.y / 2;
		}

		Vector3 operator()(const Vector3& point) const
		{
			float r = sqrtf(point.x * point.x + point.z * point.z);
			float d = r - radius;

			return Vector3(d, fabsf(point.y) - halfLength, d);
		}
	};

	void Voxelizer::occupancyFillCylinderY(OccupancyChunk& chunk, const Extents& chunkExtents, const Vector3& extents, const CoordinateFrame& cframe, float transparency, float meshRadius)
	{
		CylinderYDistanceFunction df(extents);

		occupancyFillDF(chunk, chunkExtents, extents, cframe, transparency, df);
	}

	struct WedgeDistanceFunction
	{
		Vector3 halfExtents;
		float slopeYZ, slopeZY;

		WedgeDistanceFunction(const Vector3& extents)
		{
			halfExtents = extents * 0.5f;
			slopeYZ = extents.y / extents.z;
			slopeZY = extents.z / extents.y;
		}

		Vector3 operator()(const Vector3& point) const
		{
			float distX = fabsf(point.x) - halfExtents.x;
			float distY = penetrationDepth(-halfExtents.y, point.z * slopeYZ, point.y);
			float distZ = penetrationDepth(point.y * slopeZY, halfExtents.z, point.z);

			return Vector3(distX, distY, distZ);
		}
	};

	void Voxelizer::occupancyFillWedge(OccupancyChunk& chunk, const Extents& chunkExtents, const Vector3& extents, const CoordinateFrame& cframe, float transparency, float meshRadius)
	{
		WedgeDistanceFunction df(extents);

		occupancyFillDF(chunk, chunkExtents, extents, cframe, transparency, df);
	}

	struct CornerWedgeDistanceFunction
	{
		Vector3 halfExtents;
		float slopeXY, slopeZY;
		float slopeYZ, slopeYX;

		CornerWedgeDistanceFunction(const Vector3& extents)
		{
			halfExtents = extents * 0.5f;
			slopeXY = extents.x / extents.y;
			slopeZY = extents.z / extents.y;
			slopeYZ = extents.y / extents.z;
			slopeYX = extents.y / extents.x;
		}

		Vector3 operator()(const Vector3& point) const
		{
			float distX = penetrationDepth(point.y * slopeXY, halfExtents.x, point.x);
			float distY = penetrationDepth(-halfExtents.y, std::min(-point.z * slopeYZ, point.x * slopeYX), point.y);
			float distZ = penetrationDepth(-halfExtents.z, -point.y * slopeZY, point.z);

			return Vector3(distX, distY, distZ);
		}
	};

	void Voxelizer::occupancyFillCornerWedge(OccupancyChunk& chunk, const Extents& chunkExtents, const Vector3& extents, const CoordinateFrame& cframe, float transparency, float meshRadius)
	{
		CornerWedgeDistanceFunction df(extents);

		occupancyFillDF(chunk, chunkExtents, extents, cframe, transparency, df);
	}

	struct TorsoDistanceFunction
	{
		Vector3 halfExtents;
		float shoulderInset;

		TorsoDistanceFunction(const Vector3& extents)
		{
			halfExtents = extents * 0.5f;
			shoulderInset = extents.z * 0.3f;
		}

		Vector3 operator()(const Vector3& point) const
		{
			float halfExtentsXTop = std::max(halfExtents.x - shoulderInset, 0.f);
			float halfExtentsXCur = G3D::lerp(halfExtents.x, halfExtentsXTop, point.y / halfExtents.y * 0.5f + 0.5f);

			float distX = fabsf(point.x) - halfExtentsXCur;
			float distY = fabsf(point.y) - halfExtents.y;
			float distZ = fabsf(point.z) - halfExtents.z;

			return Vector3(distX, distY, distZ);
		}
	};

	void Voxelizer::occupancyFillTorso(OccupancyChunk& chunk, const Extents& chunkExtents, const Vector3& extents, const CoordinateFrame& cframe, float transparency, float meshRadius)
	{
		TorsoDistanceFunction df(extents);

		occupancyFillDF(chunk, chunkExtents, extents, cframe, transparency, df);
	}

	template <typename DistanceFunction> void Voxelizer::occupancyFillDF(OccupancyChunk& chunk, const Extents& chunkExtents, const Vector3& extents, const CoordinateFrame& cframe, float transparency, DistanceFunction& df)
	{
		Vector3 extentsWorldSize = extentsToWorldSpace(cframe, extents);
		AABox extentsWorld = AABox(cframe.translation - extentsWorldSize * 0.5f, cframe.translation + extentsWorldSize * 0.5f);

		Vector3int32 extentsMin = fastFloorInt32((extentsWorld.low() - chunkExtents.min()) / 4.f).max(Vector3int32(0, 0, 0));
		// A small negative offset is added to extentsMax before flooring to prevent updating extra layer of cells for perfectly aligned/sized parts
		Vector3int32 extentsMax = fastFloorInt32((extentsWorld.high() - chunkExtents.min()) / 4.f - Vector3(0.01f, 0.01f, 0.01f)).min(kVoxelChunkSize - Vector3int32(1, 1, 1));

		float thickness = 255.5f * (1 - G3D::clamp(transparency, 0.f, 1.f));

		Vector3 sizeCellsClamped = (extents / 4.f).min(Vector3(1.0f, 1.0f, 1.0f));

		Vector3 localCorner = cframe.pointToObjectSpace(chunkExtents.min() + Vector3(extentsMin.x + 0.5f, extentsMin.y + 0.5f, extentsMin.z + 0.5f) * 4.f);
		Vector3 localAxisX = Vector3(cframe.rotation[0][0], cframe.rotation[0][1], cframe.rotation[0][2]) * 4.f;
		Vector3 localAxisY = Vector3(cframe.rotation[1][0], cframe.rotation[1][1], cframe.rotation[1][2]) * 4.f;
		Vector3 localAxisZ = Vector3(cframe.rotation[2][0], cframe.rotation[2][1], cframe.rotation[2][2]) * 4.f;

		Vector3 localY = localCorner;

		for (int y = extentsMin.y; y <= extentsMax.y; ++y)
		{
			Vector3 localZ = localY;

			for (int z = extentsMin.z; z <= extentsMax.z; ++z)
			{
				Vector3 localX = localZ;

				for (int x = extentsMin.x; x <= extentsMax.x; ++x)
				{
					Vector3 dist = df(localX);

					// if distance <-2, voxel is completely inside the part
					// if distance >+2, voxel is completely outside the part
					// a product of the clamped factors turns out to be a reasonable approximation of the intersection volume...
					float factorX = std::max(0.f, std::min(0.5f - dist.x * 0.25f, sizeCellsClamped.x));
					float factorY = std::max(0.f, std::min(0.5f - dist.y * 0.25f, sizeCellsClamped.y));
					float factorZ = std::max(0.f, std::min(0.5f - dist.z * 0.25f, sizeCellsClamped.z));

					int factor = static_cast<int>(factorX * factorY * factorZ * thickness);

					chunk.occupancy[y][z][x] = min255(chunk.occupancy[y][z][x] + factor);

					localX += localAxisX;
				}

				localZ += localAxisZ;
			}

			localY += localAxisY;
		}
	}

    void Voxelizer::occupancyUpdateChunk(OccupancyChunk& chunk, MegaClusterInstance* terrain, ContactManager* contactManager)
    {
        std::vector<DataModelPartCache> partCache;
        occupancyUpdateChunkPrepare(chunk, terrain, contactManager, partCache);
        occupancyUpdateChunkPerform(partCache);
    }

	void Voxelizer::occupancyUpdateChunkPrepare(OccupancyChunk& chunk, MegaClusterInstance* terrain, ContactManager* contactManager, std::vector<DataModelPartCache>& partCache)
	{
//		TIMERFUNC();
		memset(chunk.occupancy, 0, sizeof(chunk.occupancy));

		Extents chunkExtents = chunk.getChunkExtents();

		FASTLOG3F(FLog::Voxelizer, "LightGrid: Asking spatial hash for parts, extents min: %fx%fx%f", chunkExtents.min().x, chunkExtents.min().y, chunkExtents.min().z);
		FASTLOG3F(FLog::Voxelizer, "LightGrid: Asking spatial hash for parts, extents max: %fx%fx%f", chunkExtents.max().x, chunkExtents.max().y, chunkExtents.max().z);

		DenseHashSet<Primitive*> result(NULL);

        {
            RBXPROFILER_SCOPE("Voxel", "getPrimitivesOverlapping");

    		contactManager->getPrimitivesOverlapping(chunkExtents, result);
        }

		FASTLOG1(FLog::Voxelizer, "LightGrid: Returned primitives: %u", result.size());

		if (terrain)
		{
            RBXPROFILER_SCOPE("Voxel", "occupancyFillTerrain");

			if (terrain->isSmooth())
			{
                SIMDCALL(occupancyFillTerrainSmooth, (chunk, *terrain->getSmoothGrid(), chunkExtents));
			}
			else
			{
                SIMDCALL(occupancyFillTerrainMega, (chunk, *terrain->getVoxelGrid(), chunk.index * kVoxelChunkSize, chunkExtents));
			}
		}

		for (DenseHashSet<Primitive*>::const_iterator it = result.begin(); it != result.end(); ++it)
		{
			PartInstance* part = PartInstance::fromPrimitive(*it);

			if (part->getCookie() & RBX::PartCookie::IS_HUMANOID_PART)
				continue;

			if (nonFixedPartsEnabled || isPartFixed(part))
			{
                float transparency = getEffectiveTransparency(part);

                if (transparency >= 1.f) continue;

				const CoordinateFrame& cframe = part->getCoordinateFrame();
                Vector3 size = part->getPartSizeXml();
				DataModelMesh* shape = getSpecialShape(part);

				if (shape)
				{
					if (shape->getScale() == G3D::Vector3(1,1,1) && (shape->getOffset() == G3D::Vector3(0,0,0)))
					{
						if (SpecialShape* specialShape = shape->fastDynamicCast<SpecialShape>())
						{
							SpecialShape::MeshType meshType = specialShape->getMeshType();

							if (meshType == SpecialShape::HEAD_MESH)
							{
                                partCache.push_back(DataModelPartCache(&Voxelizer::occupancyFillCylinderY, chunk, size, cframe, transparency));
							}
							else if (meshType == SpecialShape::SPHERE_MESH)
							{
                                partCache.push_back(DataModelPartCache(&Voxelizer::occupancyFillEllipsoid, chunk, size, cframe, transparency));
							}
							else if (meshType == SpecialShape::CYLINDER_MESH)
							{
                                partCache.push_back(DataModelPartCache(&Voxelizer::occupancyFillCylinderX, chunk, size, cframe, transparency));
							}
							else if (meshType == SpecialShape::BRICK_MESH)
							{
                                partCache.push_back(DataModelPartCache(&Voxelizer::occupancyFillBlock, chunk, size, cframe, transparency));
							}
							else if (meshType == SpecialShape::WEDGE_MESH)
							{
                                partCache.push_back(DataModelPartCache(&Voxelizer::occupancyFillWedge, chunk, size, cframe, transparency));
							}
							else if (meshType == SpecialShape::TORSO_MESH)
							{
                                partCache.push_back(DataModelPartCache(&Voxelizer::occupancyFillTorso, chunk, size, cframe, transparency));
							}
						}
						else if (BlockMesh* blockMesh = shape->fastDynamicCast<BlockMesh>())
						{
							if (blockMesh->getOffset() == G3D::Vector3(0.f, 0.f, 0.f))
							{
                                partCache.push_back(DataModelPartCache(&Voxelizer::occupancyFillBlock, chunk, size, cframe, transparency));
							}
						}
						else if (CylinderMesh* cylinderMesh = shape->fastDynamicCast<CylinderMesh>())
						{
							if (cylinderMesh->getOffset() == G3D::Vector3(0.f, 0.f, 0.f))
							{
                                partCache.push_back(DataModelPartCache(&Voxelizer::occupancyFillCylinderY, chunk, size, cframe, transparency));
							}
						}
					}
				}
				else
				{
					PartType partType = part->getPartType();

					if (partType == BLOCK_PART)
					{
                        partCache.push_back(DataModelPartCache( &Voxelizer::occupancyFillBlock, chunk, size, cframe, transparency));
					}
					else if (partType == BALL_PART)
					{
                        partCache.push_back(DataModelPartCache( &Voxelizer::occupancyFillSphere, chunk, size, cframe, transparency));
					}
					else if (partType == CYLINDER_PART)
					{
                        partCache.push_back(DataModelPartCache( &Voxelizer::occupancyFillCylinderX, chunk, size, cframe, transparency));
					}
					else if (partType == WEDGE_PART)
					{
                        partCache.push_back(DataModelPartCache( &Voxelizer::occupancyFillWedge, chunk, size, cframe, transparency));
					}
					else if (partType == CORNERWEDGE_PART)
					{
                        partCache.push_back(DataModelPartCache( &Voxelizer::occupancyFillCornerWedge, chunk, size, cframe, transparency));
					}
					else if (partType == TRUSS_PART)
					{
                        partCache.push_back(DataModelPartCache( &Voxelizer::occupancyFillBlock, chunk, size, cframe, 0.75f + 0.25f * transparency));
					}
					else if (partType == OPERATION_PART)
					{
						addMeshToPartCache(partCache, part, chunk, chunkExtents, part->getPartSizeXml(), cframe, transparency);
					}
				}
			}
		}
	}

    void Voxelizer::occupancyUpdateChunkPerform(const std::vector<DataModelPartCache>& partCache)
    {
        OccupancyChunk* lastChunk = 0;
        Extents chunkExtents;

        for (auto& part: partCache)
        {
            if (lastChunk != part.chunk)
                chunkExtents = part.chunk->getChunkExtents();

            (this->*part.fillFunc)(*part.chunk, chunkExtents, part.extents, part.cframe, part.transparency, part.meshRadius);
        }
    }

	inline bool isInsideChunkLocal(const Vector3int32& coord)
	{
		BOOST_STATIC_ASSERT((kVoxelChunkSizeXZ & (kVoxelChunkSizeXZ - 1)) == 0);
		BOOST_STATIC_ASSERT(kVoxelChunkSizeY * 2 == kVoxelChunkSizeXZ);

		// range-check all three coordinates with a single branch by checking if there are any set bits beyond size
		return ((((coord.x | coord.z) >> 1) | coord.y) & ~(kVoxelChunkSizeY - 1)) == 0;
	}

	void Voxelizer::occupancyFillTerrainMega(OccupancyChunk& chunk, Voxel::Grid& terrain, const Vector3int32& chunkOffset, const Extents& chunkExtents)
	{
		//TIMERFUNC();

		using namespace Voxel;

		// convert from studs to terrain voxel coordinates
		Vector3int16 lightChunkMinInTerrainCoords = worldToCell_floor(chunkExtents.min());
		Vector3int16 lightChunkMaxInTerrainCoords = worldToCell_floor(chunkExtents.max()) - Vector3int16::one();

		Grid::Region region = terrain.getRegion(lightChunkMinInTerrainCoords, lightChunkMaxInTerrainCoords);
		if (region.isGuaranteedAllEmpty()) { return; }

		static const unsigned char table[] = {255, 128, 42, 213, 128, 0, 0, 0};

		Vector3int32 chunkOffsetTerrain = chunkOffset;

		for (Grid::Region::xline_iterator it = region.xLineBegin(); it != region.xLineEnd(); ++it)
		{
			int y = it.getCurrentLocation().y - chunkOffsetTerrain.y;
			int z = it.getCurrentLocation().z - chunkOffsetTerrain.z;

			RBXASSERT(it.getLineSize() == kVoxelChunkSizeXZ);
			RBXASSERT(isInsideChunkLocal(Vector3int32(0, y, z)));

			const Cell* terrainVoxelRow = it.getLineCells();

			for (int x = 0; x < kVoxelChunkSizeXZ; ++x)
			{
				CellBlock block = terrainVoxelRow[x].solid.getBlock();

				unsigned char val = table[block];

				unsigned char& occ = chunk.occupancy[y][z][x];

				occ = min255(occ + val);
			}
		}
	}


#ifdef SIMD_SSE2
	void Voxelizer::occupancyFillTerrainMegaSIMD(OccupancyChunk& chunk, Voxel::Grid& terrain, const Vector3int32& chunkOffset, const Extents& chunkExtents)
	{
		//TIMERFUNC();

		using namespace Voxel;

		// convert from studs to terrain voxel coordinates
		Vector3int16 lightChunkMinInTerrainCoords = worldToCell_floor(chunkExtents.min());
		Vector3int16 lightChunkMaxInTerrainCoords = worldToCell_floor(chunkExtents.max()) - Vector3int16::one();

		Grid::Region region = terrain.getRegion(lightChunkMinInTerrainCoords, lightChunkMaxInTerrainCoords);
		if (region.isGuaranteedAllEmpty()) { return; }

		Vector3int32 chunkOffsetTerrain = chunkOffset;

		__m128i index0 = _mm_set1_epi8(CELL_BLOCK_Solid);
		__m128i table0 = _mm_set1_epi8((char)255);

		__m128i index1 = _mm_set1_epi8(CELL_BLOCK_VerticalWedge);
		__m128i table1 = _mm_set1_epi8((char)128);

		__m128i index2 = _mm_set1_epi8(CELL_BLOCK_CornerWedge);
		__m128i table2 = _mm_set1_epi8((char)42);

		__m128i index3 = _mm_set1_epi8(CELL_BLOCK_InverseCornerWedge);
		__m128i table3 = _mm_set1_epi8((char)213);

		__m128i index4 = _mm_set1_epi8(CELL_BLOCK_HorizontalWedge);
		__m128i table4 = _mm_set1_epi8((char)128);

		const int blockShift = 3;
		__m128i blockMask = _mm_set1_epi8(MAX_CELL_BLOCKS - 1);

		for (Grid::Region::xline_iterator it = region.xLineBegin(); it != region.xLineEnd(); ++it)
		{
			int y = it.getCurrentLocation().y - chunkOffsetTerrain.y;
			int z = it.getCurrentLocation().z - chunkOffsetTerrain.z;

			RBXASSERT(it.getLineSize() == kVoxelChunkSizeXZ);
			RBXASSERT(isInsideChunkLocal(Vector3int32(0, y, z)));

			const Cell* terrainVoxelRow = it.getLineCells();

			for (int x = 0; x < kVoxelChunkSizeXZ; x += 16)
			{
				__m128i voxel = _mm_loadu_si128((__m128i*)&terrainVoxelRow[x]);
				__m128i occ = _mm_loadu_si128((__m128i*)&chunk.occupancy[y][z][x]);

				__m128i block = _mm_and_si128(_mm_srli_epi16(voxel, blockShift), blockMask);

				__m128i block0 = _mm_and_si128(_mm_cmpeq_epi8(block, index0), table0);
				__m128i block1 = _mm_and_si128(_mm_cmpeq_epi8(block, index1), table1);
				__m128i block2 = _mm_and_si128(_mm_cmpeq_epi8(block, index2), table2);
				__m128i block3 = _mm_and_si128(_mm_cmpeq_epi8(block, index3), table3);
				__m128i block4 = _mm_and_si128(_mm_cmpeq_epi8(block, index4), table4);

				__m128i value = _mm_or_si128(_mm_or_si128(block0, block1), _mm_or_si128(_mm_or_si128(block2, block3), block4));

				__m128i result = _mm_adds_epu8(occ, value);

				_mm_storeu_si128((__m128i*)&chunk.occupancy[y][z][x], result);
			}
		}
	}
#endif

#ifdef SIMD_NEON
	void Voxelizer::occupancyFillTerrainMegaSIMD(OccupancyChunk& chunk, Voxel::Grid& terrain, const Vector3int32& chunkOffset, const Extents& chunkExtents)
	{
		//TIMERFUNC();

		using namespace Voxel;

		// convert from studs to terrain voxel coordinates
		Vector3int16 lightChunkMinInTerrainCoords = worldToCell_floor(chunkExtents.min());
		Vector3int16 lightChunkMaxInTerrainCoords = worldToCell_floor(chunkExtents.max()) - Vector3int16::one();

		Grid::Region region = terrain.getRegion(lightChunkMinInTerrainCoords, lightChunkMaxInTerrainCoords);
		if (region.isGuaranteedAllEmpty()) { return; }

		Vector3int32 chunkOffsetTerrain = chunkOffset;

    #ifdef SIMD_NEON_VTBL_WORKAROUND
        uint8x16_t table = {255, 128, 42, 213, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    #else
        uint8x8_t table =  {255, 128, 42, 213, 128, 0, 0, 0};
    #endif
        
		const int blockShift = 3;
		uint8x16_t blockMask = vdupq_n_u8(MAX_CELL_BLOCKS - 1);

		for (Grid::Region::xline_iterator it = region.xLineBegin(); it != region.xLineEnd(); ++it)
		{
			int y = it.getCurrentLocation().y - chunkOffsetTerrain.y;
			int z = it.getCurrentLocation().z - chunkOffsetTerrain.z;

			RBXASSERT(it.getLineSize() == kVoxelChunkSizeXZ);
			RBXASSERT(isInsideChunkLocal(Vector3int32(0, y, z)));

			const Cell* terrainVoxelRow = it.getLineCells();

			for (int x = 0; x < kVoxelChunkSizeXZ; x += 16)
			{
				uint8x16_t voxel = vld1q_u8(reinterpret_cast<const uint8_t*>(&terrainVoxelRow[x]));
				uint8x16_t occ = vld1q_u8(&chunk.occupancy[y][z][x]);

				uint8x16_t block = vandq_u8(vshrq_n_u8(voxel, blockShift), blockMask);

				uint8x8_t block0 = vget_low_u8(block);
				uint8x8_t block1 = vget_high_u8(block);
                
            #ifdef SIMD_NEON_VTBL_WORKAROUND
                uint8x8_t value0 = vtbl1q_u8(table, block0);
                uint8x8_t value1 = vtbl1q_u8(table, block1);
            #else
                uint8x8_t value0 = vtbl1_u8(table, block0);
                uint8x8_t value1 = vtbl1_u8(table, block1);
            #endif

                uint8x16_t value = vcombine_u8(value0, value1);

				uint8x16_t result = vqaddq_u8(occ, value);

				vst1q_u8(&chunk.occupancy[y][z][x], result);
			}
		}
	}
#endif

	void Voxelizer::occupancyFillTerrainSmooth(OccupancyChunk& chunk, Voxel2::Grid& terrain, const Extents& chunkExtents)
	{
		//TIMERFUNC();

		Voxel2::Region region = Voxel2::Region::fromExtents(chunkExtents.min(), chunkExtents.max());
		Voxel2::Box box = terrain.read(region);

		if (box.isEmpty())
            return;

        BOOST_STATIC_ASSERT(Voxel2::Cell::Occupancy_Bits == 8);

		for (int y = 0; y < kVoxelChunkSizeY; ++y)
            for (int z = 0; z < kVoxelChunkSizeXZ; ++z)
            {
				RBXASSERT(box.getSizeX() == kVoxelChunkSizeXZ);
                RBXASSERT(isInsideChunkLocal(Vector3int32(0, y, z)));

                const Voxel2::Cell* terrainVoxelRow = box.readRow(0, y, z);

                for (int x = 0; x < kVoxelChunkSizeXZ; ++x)
                {
					const Voxel2::Cell& cell = terrainVoxelRow[x];

					int mask = (Voxel2::Cell::Material_Water - cell.getMaterial()) >> 31;
					unsigned char val = cell.getOccupancy() & mask;

                    unsigned char& occ = chunk.occupancy[y][z][x];

                    occ = min255(occ + val);
                }
            }
	}

#ifdef SIMD_SSE2
	void Voxelizer::occupancyFillTerrainSmoothSIMD(OccupancyChunk& chunk, Voxel2::Grid& terrain, const Extents& chunkExtents)
	{
		//TIMERFUNC();

		Voxel2::Region region = Voxel2::Region::fromExtents(chunkExtents.min(), chunkExtents.max());
		Voxel2::Box box = terrain.read(region);

		if (box.isEmpty())
            return;

		BOOST_STATIC_ASSERT(sizeof(Voxel2::Cell) == 2);
        BOOST_STATIC_ASSERT(Voxel2::Cell::Occupancy_Bits == 8);

		__m128i lowByteMask = _mm_set1_epi16(0xff);
		__m128i waterMask = _mm_set1_epi8(Voxel2::Cell::Material_Water);

		for (int y = 0; y < kVoxelChunkSizeY; ++y)
            for (int z = 0; z < kVoxelChunkSizeXZ; ++z)
            {
				RBXASSERT(box.getSizeX() == kVoxelChunkSizeXZ);
                RBXASSERT(isInsideChunkLocal(Vector3int32(0, y, z)));

                const Voxel2::Cell* terrainVoxelRow = box.readRow(0, y, z);

                for (int x = 0; x < kVoxelChunkSizeXZ; x += 16)
                {
                    // voxel0/1 contain cells in 'material occupancy' pairs
                    __m128i voxel0 = _mm_loadu_si128((__m128i*)&terrainVoxelRow[x + 0]);
                    __m128i voxel1 = _mm_loadu_si128((__m128i*)&terrainVoxelRow[x + 8]);
                    __m128i occ = _mm_loadu_si128((__m128i*)&chunk.occupancy[y][z][x]);

                    // unpack material/occupancy vectors
					__m128i voxelm = _mm_packus_epi16(_mm_and_si128(voxel0, lowByteMask), _mm_and_si128(voxel1, lowByteMask)); 
					__m128i voxelo = _mm_packus_epi16(_mm_srli_epi16(voxel0, 8), _mm_srli_epi16(voxel1, 8)); 

                    // pick occupancy for voxels that have material > water; use 0 for water/air
					__m128i value = _mm_and_si128(voxelo, _mm_cmpgt_epi8(voxelm, waterMask));

                    // accumulate occupancy
                    __m128i result = _mm_adds_epu8(occ, value);

                    _mm_storeu_si128((__m128i*)&chunk.occupancy[y][z][x], result);
                }
            }
	}
#endif

#ifdef SIMD_NEON
	void Voxelizer::occupancyFillTerrainSmoothSIMD(OccupancyChunk& chunk, Voxel2::Grid& terrain, const Extents& chunkExtents)
	{
		//TIMERFUNC();

		Voxel2::Region region = Voxel2::Region::fromExtents(chunkExtents.min(), chunkExtents.max());
		Voxel2::Box box = terrain.read(region);

		if (box.isEmpty())
            return;

		BOOST_STATIC_ASSERT(sizeof(Voxel2::Cell) == 2);
        BOOST_STATIC_ASSERT(Voxel2::Cell::Occupancy_Bits == 8);

		uint8x16_t waterMask = vdupq_n_u8(Voxel2::Cell::Material_Water);

		for (int y = 0; y < kVoxelChunkSizeY; ++y)
            for (int z = 0; z < kVoxelChunkSizeXZ; ++z)
            {
				RBXASSERT(box.getSizeX() == kVoxelChunkSizeXZ);
                RBXASSERT(isInsideChunkLocal(Vector3int32(0, y, z)));

                const Voxel2::Cell* terrainVoxelRow = box.readRow(0, y, z);

                for (int x = 0; x < kVoxelChunkSizeXZ; x += 16)
                {
                    // voxel0/1 contain cells in 'material occupancy' pairs
                    uint8x16_t voxel0 = vld1q_u8(reinterpret_cast<const uint8_t*>(&terrainVoxelRow[x + 0]));
                    uint8x16_t voxel1 = vld1q_u8(reinterpret_cast<const uint8_t*>(&terrainVoxelRow[x + 8]));
                    uint8x16_t occ = vld1q_u8(&chunk.occupancy[y][z][x]);

                    // unpack material/occupancy vectors
					uint8x16x2_t voxelmo = vuzpq_u8(voxel0, voxel1);
					uint8x16_t voxelm = voxelmo.val[0];
					uint8x16_t voxelo = voxelmo.val[1];

                    // pick occupancy for voxels that have material > water; use 0 for water/air
					uint8x16_t value = vandq_u8(voxelo, vcgtq_u8(voxelm, waterMask));

                    // accumulate occupancy
                    uint8x16_t result = vqaddq_u8(occ, value);

                    vst1q_u8(&chunk.occupancy[y][z][x], result);
                }
            }
	}
#endif
	
} }
