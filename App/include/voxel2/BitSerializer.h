#pragma once

#include "voxel2/Grid.h"

namespace RBX { namespace Voxel2 {

template <typename BitStream> class BitSerializer
{
public:
	void encodeIndex(const Vector3int32& index, BitStream& stream)
	{
        encodeChunkIndex(index - lastIndex, stream);

        lastIndex = index;
	}

	void encodeContent(const Box& box, BitStream& stream)
	{
        encodeChunkData(box, stream);
	}

    void decodeIndex(Vector3int32& index, BitStream& stream)
	{
        Vector3int32 diff;
        decodeChunkIndex(diff, stream);

        index = lastIndex + diff;
        lastIndex = index;
	}
	
	void decodeContent(Box& box, BitStream& stream)
	{
        decodeChunkData(box, stream);
	}

private:
    Vector3int32 lastIndex;
    std::vector<Cell> cells;

	void encodeChunkIndex(const Vector3int32& diff, BitStream& stream)
	{
        if (char(diff.x) == diff.x && char(diff.y) == diff.y && char(diff.z) == diff.z)
		{
            // Single-byte diffs: tag "1"
            stream << true;
            stream << char(diff.x);
            stream << char(diff.y);
            stream << char(diff.z);
		}
		else if (short(diff.x) == diff.x && short(diff.y) == diff.y && short(diff.z) == diff.z)
		{
            // Two-byte diffs: tag "01"
            stream << false;
            stream << true;
            stream << short(diff.x);
            stream << short(diff.y);
            stream << short(diff.z);
		}
        else
        {
            // Four-byte diffs: tag "00"
            stream << false;
            stream << false;
            stream << diff.x;
            stream << diff.y;
            stream << diff.z;
        }
	}

	void decodeChunkIndex(Vector3int32& diff, BitStream& stream)
	{
        bool size1;
        stream >> size1;

        if (size1)
		{
            // Single-byte diffs: tag "1"
            char x, y, z;
            stream >> x;
            stream >> y;
            stream >> z;

            diff = Vector3int32(x, y, z);
		}
		else
		{
            bool size2;
            stream >> size2;

            if (size2)
            {
                // Two-byte diffs: tag "01"
                short x, y, z;
                stream >> x;
                stream >> y;
                stream >> z;

                diff = Vector3int32(x, y, z);
            }
            else
            {
                // Four-byte diffs: tag "00"
                int x, y, z;
                stream >> x;
                stream >> y;
                stream >> z;

                diff = Vector3int32(x, y, z);
            }
		}
	}

	void encodeChunkData(const Box& box, BitStream& stream)
	{
		bool empty = box.isEmpty();

        stream << empty;

        if (empty)
            return;

		Vector3int32 size = box.getSize();

		cells.resize(size.x * size.y * size.z);

        unsigned int cellOffset = 0;

        for (int y = 0; y < size.y; ++y)
            for (int z = 0; z < size.z; ++z)
			{
                memcpy(&cells[cellOffset], box.readRow(0, y, z), size.x * sizeof(Cell));
                cellOffset += size.x;
			}

        int lastMaterial = 0;

        for (unsigned int offset = 0; offset < cells.size(); )
		{
            // identify run length
            Cell cell = cells[offset];
            unsigned int count = 0;

            do offset++, count++;
            while (offset < cells.size() && cells[offset] == cell && count < 512);

            // serialize run length
            // 00 = single cell
            // xx = x groups of 3-bit values (max is 3 groups of 3-bit values = 9 bit = 512)
            unsigned char groups = (count == 1) ? 0 : (count <= 8) ? 1 : (count <= 64) ? 2 : 3;
            unsigned int temp = count - 1;

            stream.WriteBits(&groups, 2);
            stream.WriteBits((const unsigned char*)&temp, groups * 3);

            // serialize material/occupancy combo
            // 0 = air
            // 10 = full (occupancy is assumed to be max)
            // 11 = custom (followed by 8 bits with occupancy data)

            // material (only if it's not air)
            // 0 = last
            // 1 = new (followed by 6 bits with material data)
			if (cell.getMaterial() == Cell::Material_Air)
                stream << false;
			else
			{
                // solid
                stream << true;

                // customOccupancy
				if (cell.getOccupancy() == Cell::Occupancy_Max)
                    stream << false;
				else
				{
                    stream << true;

					unsigned char occupancy = cell.getOccupancy();
					stream.WriteBits(&occupancy, Cell::Occupancy_Bits);
				}

                // newMaterial
				if (cell.getMaterial() == lastMaterial)
                    stream << false;
				else
				{
                    stream << true;

					unsigned char material = cell.getMaterial();
                    stream.WriteBits(&material, Cell::Material_Bits);
				}

                lastMaterial = cell.getMaterial();
			}
		}
	}

	void decodeChunkData(Box& box, BitStream& stream)
	{
		RBXASSERT(box.isEmpty());

        bool empty;
        stream >> empty;

        if (empty)
            return;

		Vector3int32 size = box.getSize();

        cells.resize(size.x * size.y * size.z);

        int lastMaterial = 0;

        for (unsigned int offset = 0; offset < cells.size(); )
		{
            // deserialize run length
            unsigned char groups = 0;
            stream.ReadBits(&groups, 2);

            unsigned int temp = 0;
            stream.ReadBits((unsigned char*)&temp, groups * 3);

            unsigned int count = temp + 1;

            if (offset + count > cells.size())
				throw RBX::runtime_error("Error while decoding data: chunk overflow at %u cells", offset + count);

            // deserialize material/occupancy
			unsigned char material = Cell::Material_Air;
			unsigned char occupancy = 0;

            bool solid;
            stream >> solid;

            if (solid)
			{
                bool customOccupancy;
                stream >> customOccupancy;

                if (!customOccupancy)
					occupancy = Cell::Occupancy_Max;
				else
					stream.ReadBits(&occupancy, Cell::Occupancy_Bits);

                bool newMaterial;
                stream >> newMaterial;

                if (!newMaterial)
					material = lastMaterial;
				else
                    stream.ReadBits(&material, Cell::Material_Bits);

                lastMaterial = material;
			}

            // fill cells
            Cell cell(material, occupancy);

            for (unsigned int i = 0; i < count; ++i)
                cells[offset + i] = cell;

            offset += count;
		}

        unsigned int cellOffset = 0;

        for (int y = 0; y < size.y; ++y)
            for (int z = 0; z < size.z; ++z)
			{
                memcpy(box.writeRow(0, y, z), &cells[cellOffset], size.x * sizeof(Cell));
                cellOffset += size.x;
			}
	}
};

} }
