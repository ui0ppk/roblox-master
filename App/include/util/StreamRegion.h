#pragma once

#include "Util/G3DCore.h"
#include "Util/Region3int32.h"
#include "Util/Vector3int32.h"
#include "v8world/ContactManagerSpatialHash.h"
#include "Util/Extents.h"
#include "Voxel/Util.h"

namespace RBX {

	// Utilities for working with sub-sections of global coordinate space called
	// StreamRegions. The sub-sections are of a fixed, globally defined size and
	// position. The sub-sections are integer aligned and origin aligned (the
	// origin is at the meeting point of 8 regions).
	namespace StreamRegion {

		namespace Constants {
			// these are const int to allow proper constant folding, inlining, and
			// to allow other, derived static const int values to also have these
			// benefits.
            const int kMinNumPlayableRegion = 3*3*3;
		}
		namespace _PrivateConstants {
			const int kRegionSizeInVoxelsAsBitShift = 4;
			const int kRegionSizeInStudsAsBitShift =
				kRegionSizeInVoxelsAsBitShift + Voxel::kCELL_SIZE_AS_BIT_SHIFT;
			
			const Vector3int32 kRegionDimensionInVoxelsAsBitShifts(
				kRegionSizeInVoxelsAsBitShift, kRegionSizeInVoxelsAsBitShift, kRegionSizeInVoxelsAsBitShift);

			const Vector3int32 kRegionDimensionInStudsAsBitShifts(
				kRegionSizeInStudsAsBitShift, kRegionSizeInStudsAsBitShift, kRegionSizeInStudsAsBitShift);

            const int kRegionDimensionInVoxels = 1 << kRegionSizeInVoxelsAsBitShift;
            const int kMaxVoxelOffsetInsideRegion = kRegionDimensionInVoxels - 1;
		}

		// Identifier object for regions of space. Has a Vector3int32 representation
		// also. The Vector3int32 representation has constraints:
		// * If two Ids represent the same area of space, then the Vector3int32's are ==
		// * If one Vector3int32's values (x, y, and/or z) is > another, then the
		//   spacial coordinates on those axis(es) are also >.
		// * If two regions are adjacent, then the Vector3int32 values will be
		//   sequential on the axis(es) they are adjacent in.
		// * The Vector3int32 representation is consistent between runs, and between
		//   client/server.
		class Id {
			Vector3int32 internalValue;
		public:
			Id() : internalValue(Vector3int32(0,0,0)) {}
			explicit Id(const Vector3int32& internalValue) : internalValue(internalValue) {}
			Id(int x, int y, int z) : internalValue(Vector3int32(x,y,z)) {}

			const Vector3int32& value() const { return internalValue; }

			Id operator+(const Vector3int32& other) const { return Id(internalValue + other); }
			Id operator+(const Id &other) const { return Id(internalValue + other.internalValue); }
			bool operator==(const Id& other) const { return internalValue == other.internalValue; }
			bool operator!=(const Id& other) const { return internalValue != other.internalValue; }

			static int streamGridCellSizeInStuds()
			{
				return static_cast<int>(SpatialHashStatic::hashGridSize(CONTACTMANAGER_MAXLEVELS-1));
			}

            static int getRegionLongestAxisDistance(Id r1, Id r2)
            {
                int xDistance = abs(r1.value().x - r2.value().x);
                int yDistance = abs(r1.value().y - r2.value().y);
                int zDistance = abs(r1.value().z - r2.value().z);
                int result = xDistance;
                if (yDistance > result)
                {
                    result = yDistance;
                }
                if (zDistance > result)
                {
                    result = zDistance;
                }
                return result;
            }

			bool isRegionInTerrainBoundaries() const {
                Region3int16 extents = Voxel::getTerrainExtentsInCells();
                Vector3int16 min = extents.getMinPos() >> _PrivateConstants::kRegionSizeInVoxelsAsBitShift;
                Vector3int16 max = extents.getMaxPos() >> _PrivateConstants::kRegionSizeInVoxelsAsBitShift;

                const Vector3int32 &vv = value();
                return vv.x >= min.x && vv.y >= min.y && vv.z >= min.z && vv.x <= max.x && vv.y <= max.y && vv.z <= max.z;
			}

			struct boost_compatible_hash_value {
				size_t operator()(const Id& key) const {
					const Vector3int32 &v = key.internalValue;
					return (v.x * 11) + ((v.y * 7) << 10) + ((v.z * 3) << 20);
				}
			};
		};
        
        std::size_t hash_value(const Id& key);

		class IdExtents {
		public:
			// inclusive stream region extents
			Id low, high;

			bool operator==(const IdExtents& other) const {
				return (low == other.low) && (high == other.high);
			}

			// Determines if these extents intersect the regions in the
			// argument container. If optionalFoundId is not null, it will
			// be set to an example of an Id that is both inside these extents
			// and inside the container.
			template<class Container>
			bool intersectsContainer(const Container& container, Id* optionalFoundId = NULL) const {
				const Vector3int32& extentsMin = low.value();
				const Vector3int32& extentsMax = high.value();
				Vector3int32 counter;
				for (counter.y = extentsMin.y; counter.y <= extentsMax.y; counter.y++) {
				for (counter.z = extentsMin.z; counter.z <= extentsMax.z; counter.z++) {
				for (counter.x = extentsMin.x; counter.x <= extentsMax.x; counter.x++) {
					Id counterId(counter);
					if (container.find(counterId) != container.cend()) {
						if (optionalFoundId) {
							(*optionalFoundId) = counterId; 
						}
						return true;
					}
				}
				}
				}
				return false;
			}
		};

		inline const Vector3int32& gridCellDimension()
		{
			static Vector3int32 v = Vector3int32(StreamRegion::Id::streamGridCellSizeInStuds(), 
												 StreamRegion::Id::streamGridCellSizeInStuds(), 
												 StreamRegion::Id::streamGridCellSizeInStuds());
			return v;
		}

		inline const Vector3int32& gridCellHalfDimension()
		{
			static Vector3int32 v = Vector3int32(StreamRegion::Id::streamGridCellSizeInStuds()/2, 
												 StreamRegion::Id::streamGridCellSizeInStuds()/2, 
												 StreamRegion::Id::streamGridCellSizeInStuds()/2);
			return v;
		}

		inline Id regionContainingWorldPosition(const Vector3 &worldPos) {
			// This loses precision unnecessarily, it could divide by the
			// dimension size before taking floor. However this will only
			// impact coordinates more than 2^31 studs away from origin.
			Vector3int32 floorPos = Vector3int32::floor(worldPos);
			return StreamRegion::Id(floorPos >> _PrivateConstants::kRegionDimensionInStudsAsBitShifts);
		}

		inline Extents extentsFromRegionId(const Id &id) {
			// This loses precision unnecessarily, it could multiply by the
			// dimension size after it is converted to float. However this will
			// only impact coordinates more than 2^25 studs away from origin.
			Vector3int32 min = (id.value() << _PrivateConstants::kRegionDimensionInStudsAsBitShifts);
			Vector3int32 max = min + (Vector3int32::one() << _PrivateConstants::kRegionDimensionInStudsAsBitShifts);
			return ExtentsInt32(min, max).toExtents();
		}

		inline Id regionContainingVoxel(const Vector3int16& voxelCoordinate) {
            return Id(Vector3int32(voxelCoordinate) >>
                    _PrivateConstants::kRegionDimensionInVoxelsAsBitShifts);
		}

		inline Id regionContainingVoxel(const Vector3int32& voxelCoordinate) {
            return Id(voxelCoordinate >> _PrivateConstants::kRegionDimensionInVoxelsAsBitShifts);
		}

		inline Vector3int16 getMinVoxelCoordinateInsideRegion(const Id& id) {
            // min terrain boundary corresponds with voxel coordinate origin
            return (id.value() <<
                _PrivateConstants::kRegionDimensionInVoxelsAsBitShifts).toVector3int16();
		}

        inline Vector3int16 getMaxVoxelOffsetInsideRegion() {
            using namespace _PrivateConstants;
            return Vector3int16(
                kMaxVoxelOffsetInsideRegion,
                kMaxVoxelOffsetInsideRegion,
                kMaxVoxelOffsetInsideRegion);
        }

        inline Vector3int16 getMaxVoxelCoordinateInsideRegion(const Id& id) {
            // max terrain boundary corresponds with voxel coordinate origin
            return (getMinVoxelCoordinateInsideRegion(id) + getMaxVoxelOffsetInsideRegion());
        }

		inline unsigned int getTotalVoxelVolumeOfARegion() {
			return 1 << (3 * _PrivateConstants::kRegionSizeInVoxelsAsBitShift);
		}

		inline IdExtents regionExtentsFromContactManagerLevelAndExtents(
				int level, ExtentsInt32 contactManagerExtents) {
			static const int kSpatialHashMaxLevel = CONTACTMANAGER_MAXLEVELS - 1;

			// Stream region extents coincide with max spatial hash buckets
			ExtentsInt32 scaledExtents = SpatialHashStatic::scaleExtents(
					level, kSpatialHashMaxLevel, contactManagerExtents);
			IdExtents result;
			result.low = Id(scaledExtents.min());
			result.high = Id(scaledExtents.max());
			return result;
		}

		inline bool coarseMovementCausesStreamRegionChange(
				const ContactManagerSpatialHash::CoarseMovementCallback::UpdateInfo& info,
				IdExtents* oldExtents, IdExtents* newExtents) {
			
			typedef ContactManagerSpatialHash::CoarseMovementCallback::UpdateInfo UpdateInfo;

			RBXASSERT_SLOW(info.updateType == UpdateInfo::UPDATE_TYPE_Change ||
				info.updateType == UpdateInfo::UPDATE_TYPE_Insert);

			(*newExtents) = regionExtentsFromContactManagerLevelAndExtents(
					info.newLevel, info.newSpatialExtents);

			if (info.updateType == UpdateInfo::UPDATE_TYPE_Change) {
				(*oldExtents) = regionExtentsFromContactManagerLevelAndExtents(
						info.oldLevel, info.oldSpatialExtents);
				if ((*newExtents) == (*oldExtents)) {
					return false;
				}
			}
			return true;
		}
	} // StreamRegion

} // RBX
