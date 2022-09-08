#pragma once

#include "RBX/Debug.h"
#include "Util/ClusterCellIterator.h"
#include "Util/FixedSizeCircularBuffer.h"
#include "Util/G3DCore.h"
#include "Util/SpatialRegion.h"
#include "Util/VarInt.h"
#include "Voxel/Cell.h"
#include "Voxel/Grid.h"

namespace RBX { namespace Voxel {

class SerializerConstants {
public:
	// these values are used for serializing cluster
	// they are visible for testing
	static const unsigned char kNewCellMarker;
	static const unsigned char kRepeatCellMarker;
	static const unsigned char kEndSequenceMarker;
	static const unsigned int kRecentlyEncodedReferenceBits;
};

class Serializer {
	typedef FixedSizeCircularBuffer<unsigned int, 8> RecentlyEncodedBuffer;

	template<class CellBuffer, class OutputStream>
	void encodeFromPosition(const Grid* voxelStore, Vector3int16& cellpos,
			const SpatialRegion::Id& lastChunkPos, const Grid::Region& region,
			RecentlyEncodedBuffer& lastSeenNewCells,
			CellBuffer& cellBuffer, OutputStream* outputStream) const {

		unsigned char cellValue = Cell::serializeAsUnsignedChar(region.voxelAt(cellpos));
		unsigned char materialValue = region.materialAt(cellpos);
		
		unsigned int content = (materialValue << 8) | cellValue;
		Vector3int16 unread;

		unsigned int findIndex;
		bool isOldContent = lastSeenNewCells.find(content, &findIndex);
	
		if (!isOldContent) {
			outputStream->WriteBits(&SerializerConstants::kNewCellMarker, 2);
			outputStream->WriteBits(&materialValue, 8);
			outputStream->WriteBits(&cellValue, 8);
			lastSeenNewCells.push(content);
            CellBuffer::nextCellInIterationOrder(cellpos, &cellpos);
		} else {
			// TODO: The cell reads in this section aren't safe! They will read
			// past the end of the cluster's data array.
			unsigned int copyCount = 1; // this cell is a copy
			Vector3int16 nextPos;
            CellBuffer::nextCellInIterationOrder(cellpos, &nextPos);

            SpatialRegion::Id nextChunk = SpatialRegion::regionContainingVoxel(nextPos);

			unsigned char nextCellValue = Cell::serializeAsUnsignedChar(region.voxelAt(nextPos));
			unsigned char nextMaterialValue = region.materialAt(nextPos);
			unsigned int nextContent = (nextMaterialValue << 8) | nextCellValue;
			
			while (nextChunk == lastChunkPos && cellBuffer.chk(nextPos) &&
				nextContent == content) {
				copyCount++;
				cellBuffer.pop(&unread);
				RBXASSERT(nextPos == unread);
                CellBuffer::nextCellInIterationOrder(nextPos, &nextPos);
				nextChunk = SpatialRegion::regionContainingVoxel(nextPos);
				nextCellValue = Cell::serializeAsUnsignedChar(region.voxelAt(nextPos));
				nextMaterialValue = region.materialAt(nextPos);
				nextContent = (nextMaterialValue << 8) | nextCellValue;
			}

			cellpos = nextPos;
			outputStream->WriteBits(&SerializerConstants::kRepeatCellMarker, 2);
			unsigned char charFindIndex = findIndex;
			outputStream->WriteBits(&charFindIndex, SerializerConstants::kRecentlyEncodedReferenceBits);
			VarInt<>::encode(*outputStream, copyCount);
		}
	}

public:

	template<class CellBuffer, class OutputStream>
	void encodeCells(const Grid* voxelStore, CellBuffer& cellBuffer,
			OutputStream* outputStream, int sizeLimitInBytes) const {

		const Vector3int16 kCellInChunkBits(
				SpatialRegion::getRegionDimensionInVoxelsAsBitShifts());

		RecentlyEncodedBuffer lastSeenNewCells;
		Grid::Region region;
		SpatialRegion::Id lastChunkPos(SHRT_MIN, SHRT_MIN, SHRT_MIN);
		while(cellBuffer.size() > 0 && (sizeLimitInBytes == -1 || ((int)outputStream->GetNumberOfBytesUsed()) < sizeLimitInBytes))
		{
			Vector3int16 cellpos;
			cellBuffer.pop(&cellpos);

			SpatialRegion::Id chunk = SpatialRegion::regionContainingVoxel(cellpos);
			Vector3int16 cellModChunk = SpatialRegion::voxelCoordinateRelativeToEnclosingRegion(cellpos);

			unsigned char chunkChanged = 0;
			if (chunk != lastChunkPos) {
				lastChunkPos = chunk;
				chunkChanged = 1;
				outputStream->WriteBits(&chunkChanged, 1);

				// write a "0" to indicate that we aren't finished
				chunkChanged = 0;
				outputStream->WriteBits(&chunkChanged, 1);

                boost::int16_t data = chunk.value().x;
                outputStream->WriteBits(reinterpret_cast<unsigned char*>(&data), 16);
                data = chunk.value().y;
                outputStream->WriteBits(reinterpret_cast<unsigned char*>(&data), 16);
                data = chunk.value().z;
                outputStream->WriteBits(reinterpret_cast<unsigned char*>(&data), 16);

				Region3int16 extents = SpatialRegion::inclusiveVoxelExtentsOfRegion(chunk);
				region = voxelStore->getRegion(extents.getMinPos(), extents.getMaxPos());
			} else {
				outputStream->WriteBits(&chunkChanged, 1);
			}

			unsigned char data = cellModChunk.x;
			outputStream->WriteBits(&data, kCellInChunkBits.x);
			data = cellModChunk.y;
			outputStream->WriteBits(&data, kCellInChunkBits.y);
			data = cellModChunk.z;
			outputStream->WriteBits(&data, kCellInChunkBits.z);

			Vector3int16& nextPos = cellpos;
			bool continuing = true;
			do {
				encodeFromPosition(voxelStore, nextPos, lastChunkPos, region, lastSeenNewCells,
					cellBuffer, outputStream);
				continuing = cellBuffer.chk(nextPos) &&
					SpatialRegion::regionContainingVoxel(nextPos) == lastChunkPos &&
					(sizeLimitInBytes == -1 || ((int)outputStream->GetNumberOfBytesUsed()) < sizeLimitInBytes);
				if (continuing) {
					Vector3int16 unused;
					cellBuffer.pop(&unused);
					RBXASSERT(unused == nextPos);
				}
			} while (continuing);

			unsigned char endSequenceMarker = SerializerConstants::kEndSequenceMarker;
			outputStream->WriteBits(&endSequenceMarker, 2);
		}

		// write finalizer
		unsigned char finalValue = 0xff;
		// write 1 bit for chunk changed, and one bit to indicate EOM
		outputStream->WriteBits(&finalValue, 2);
	}

	template<class CellBuffer, class InputStream, class CellUpdateFilter>
	void decodeCells(Grid* voxelStore, InputStream& inputStream,
			CellUpdateFilter& filter) {
		const Vector3int16 kCellInChunkBits(
			SpatialRegion::getRegionDimensionInVoxelsAsBitShifts());

		RecentlyEncodedBuffer lastSeenNewCells;
		SpatialRegion::Id chunkPos(SHRT_MIN, SHRT_MIN, SHRT_MIN);

		while(1)
		{
			unsigned char changedChunk;
			inputStream.ReadBits(&changedChunk, 1);
			if (changedChunk) {
				unsigned char eomTokenReceived = 0;
				inputStream.ReadBits(&eomTokenReceived, 1);
				if (eomTokenReceived) {
					break;
				}

                boost::int16_t x, y, z;

                inputStream.ReadBits(reinterpret_cast<unsigned char*>(&x), 16);
                inputStream.ReadBits(reinterpret_cast<unsigned char*>(&y), 16);
                inputStream.ReadBits(reinterpret_cast<unsigned char*>(&z), 16);

                chunkPos = SpatialRegion::Id(x, y, z);
			}
			Vector3int16 cellPos(0,0,0);

			unsigned char data;
			inputStream.ReadBits(&data, kCellInChunkBits.x);
			cellPos.x = data;
			inputStream.ReadBits(&data, kCellInChunkBits.y);
			cellPos.y = data;
			inputStream.ReadBits(&data, kCellInChunkBits.z);
			cellPos.z = data;
			cellPos = SpatialRegion::globalVoxelCoordinateFromRegionAndRelativeCoordinate(
				chunkPos, cellPos);
			unsigned char controlBits;
			do {
				inputStream.ReadBits(&controlBits, 2);
				if (controlBits == SerializerConstants::kNewCellMarker) {
					unsigned char material, cell;
					inputStream.ReadBits(&material, 8);
					inputStream.ReadBits(&cell, 8);

					unsigned int content = (material << 8) | cell;

					lastSeenNewCells.push(content);
				
					if (filter.canSet(cellPos)) {
						voxelStore->setCell(cellPos, Cell::deserializeFromUnsignedChar(cell), (CellMaterial)material);
					}
					// advance cellPos
                    CellBuffer::nextCellInIterationOrder(cellPos, &cellPos);
				} else if (controlBits == SerializerConstants::kRepeatCellMarker) {
					unsigned char backIndex;
					inputStream.ReadBits(&backIndex, SerializerConstants::kRecentlyEncodedReferenceBits);
					unsigned int count = 0;
					VarInt<>::decode(inputStream, &count);
					RBXASSERT(count > 0);

					unsigned int content = lastSeenNewCells[backIndex];
					unsigned char material = content >> 8;
					unsigned char cell = content & 0xFF;

					do {
						if (filter.canSet(cellPos)) {
							voxelStore->setCell(cellPos,
								Cell::deserializeFromUnsignedChar(cell),
								(CellMaterial)material);
						}
                        CellBuffer::nextCellInIterationOrder(cellPos, &cellPos);
						count--;
					} while (count);
					// at this point cellPos points to the next cell after the sequence
				}
			} while(controlBits != SerializerConstants::kEndSequenceMarker);
			// do not read cellPos after this line, the do {} while() loop ends with
			// cellPos at an invalid cell.
		}
	}
};

} } // namespace RBX
