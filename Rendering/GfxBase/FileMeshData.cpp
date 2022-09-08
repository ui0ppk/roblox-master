#include "GfxBase/FileMeshData.h"

#include "rbx/Debug.h"

#include "rbx/DenseHash.h"

namespace RBX
{
    struct MeshVertexHasher
    {
        bool operator()(const FileMeshVertexNormalTexture3d& l, const FileMeshVertexNormalTexture3d& r) const
        {
            return memcmp(&l, &r, sizeof(l)) == 0;
        }
        
        size_t operator()(const FileMeshVertexNormalTexture3d& v) const
        {
            size_t result = 0;
            boost::hash_combine(result, v.vx);
            boost::hash_combine(result, v.vy);
            boost::hash_combine(result, v.vz);
            return result;
        }
    };
    
    void optimizeMesh(FileMeshData& mesh)
    {
        std::vector<unsigned int> remap(mesh.vnts.size());
        
        FileMeshVertexNormalTexture3d dummy = {};
        dummy.vx = FLT_MAX;
        
        typedef DenseHashMap<FileMeshVertexNormalTexture3d, unsigned int, MeshVertexHasher, MeshVertexHasher> VertexMap;
        VertexMap vertexMap(dummy);
        
        for (size_t i = 0; i < mesh.vnts.size(); ++i)
        {
            unsigned int& vi = vertexMap[mesh.vnts[i]];
            
            if (vi == 0)
                vi = vertexMap.size();
            
            remap[i] = vi - 1;
        }
        
        std::vector<FileMeshVertexNormalTexture3d> newvnts(vertexMap.size());
        
        for (size_t i = 0; i < mesh.vnts.size(); ++i)
            newvnts[remap[i]] = mesh.vnts[i];
        
        mesh.vnts.swap(newvnts);
        
        for (size_t i = 0; i < mesh.faces.size(); ++i)
        {
            FileMeshFace& face = mesh.faces[i];
            
            face.a = remap[face.a];
            face.b = remap[face.b];
            face.c = remap[face.c];
        }
    }
    
    inline unsigned int atouFast(const char* value, const char** end)
    {
        const char* s = value;
        
        // skip whitespace
        while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')
            s++;
        
        // read integer part
        unsigned int result = 0;
        
        while (static_cast<unsigned int>(*s - '0') < 10)
        {
            result = result * 10 + (*s - '0');
            s++;
        }
        
        // done!
        *end = s;
        
        return result;
    }

    inline double atofFast(const char* value, const char** end)
    {
        static const double digits[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        static const double powers[] = { 1e0, 1e+1, 1e+2, 1e+3, 1e+4, 1e+5, 1e+6, 1e+7, 1e+8, 1e+9, 1e+10, 1e+11, 1e+12, 1e+13, 1e+14, 1e+15, 1e+16, 1e+17, 1e+18, 1e+19, 1e+20, 1e+21, 1e+22 };
        
        const char* s = value;
        
        // skip whitespace
        while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')
            s++;
        
        // read sign
        double sign = (*s == '-') ? -1 : 1;
        s += (*s == '-' || *s == '+');
        
        // read integer part
        double result = 0;
        int power = 0;
        
        while (static_cast<unsigned int>(*s - '0') < 10)
        {
            result = result * 10 + digits[*s - '0'];
            s++;
        }
        
        // read fractional part
        if (*s == '.')
        {
            s++;
            
            while (static_cast<unsigned int>(*s - '0') < 10)
            {
                result = result * 10 + digits[*s - '0'];
                s++;
                power--;
            }
        }
        
        // read exponent part
        if ((*s | ' ') == 'e')
        {
            s++;
            
            // read exponent sign
            int expsign = (*s == '-') ? -1 : 1;
            s += (*s == '-' || *s == '+');
            
            // read exponent
            int exppower = 0;
            
            while (static_cast<unsigned int>(*s - '0') < 10)
            {
                exppower = exppower * 10 + (*s - '0');
                s++;
            }
            
            // done!
            power += expsign * exppower;
        }
        
        // done!
        *end = s;
        
        if (static_cast<unsigned int>(-power) < sizeof(powers) / sizeof(powers[0]))
            return sign * result / powers[-power];
        else if (static_cast<unsigned int>(power) < sizeof(powers) / sizeof(powers[0]))
            return sign * result * powers[power];
        else
            return sign * result * powf(10.0, power);
    }
    
    inline const char* readToken(const char* data, char terminator)
    {
        while (*data == ' ' || *data == '\t' || *data == '\r' || *data == '\n')
            ++data;
        
        if (*data != terminator)
            throw RBX::runtime_error("Error reading mesh data: expected %c", terminator);
        
        return data + 1;
    }
    
    inline const char* readFloatToken(const char* data, char terminator, float* output)
    {
        const char* end;
        double value = atofFast(data, &end);
        
        if (*end != terminator)
            throw RBX::runtime_error("Error reading mesh data: expected %c", terminator);

        *output = value;
        
        return end + 1;
    }
    
    shared_ptr<FileMeshData> readMeshFromV1(const std::string& data, size_t offset_, float scaler)
    {
        shared_ptr<FileMeshData> mesh(new FileMeshData());
        
        const char* offset = data.c_str() + offset_;
        unsigned int num_faces = atouFast(offset, &offset);
        
        mesh->vnts.reserve(num_faces * 3);
        mesh->faces.reserve(num_faces);
        
        for (unsigned int i = 0; i < num_faces; i++)
        {
            for (int v = 0; v < 3; v++)
            {
                float vx, vy, vz, nx, ny, nz, tu, tv, tw;
                
                offset = readToken(offset, '[');
                offset = readFloatToken(offset, ',', &vx);
                offset = readFloatToken(offset, ',', &vy);
                offset = readFloatToken(offset, ']', &vz);
                offset = readToken(offset, '[');
                offset = readFloatToken(offset, ',', &nx);
                offset = readFloatToken(offset, ',', &ny);
                offset = readFloatToken(offset, ']', &nz);
                offset = readToken(offset, '[');
                offset = readFloatToken(offset, ',', &tu);
                offset = readFloatToken(offset, ',', &tv);
                offset = readFloatToken(offset, ']', &tw);
                
                G3D::Vector3 normal = G3D::Vector3(nx, ny, nz).unit();
                
                if (!normal.isFinite())
                    normal = G3D::Vector3::zero();
                
                FileMeshVertexNormalTexture3d vtx =
                {
                    vx * scaler, vy * scaler, vz * scaler,
                    normal.x, normal.y, normal.z,
                    tu, 1.f - tv, tw
                };
                
                mesh->vnts.push_back(vtx);
            }
            
            FileMeshFace face = {i * 3 + 0, i * 3 + 1, i * 3 + 2};
            
            mesh->faces.push_back(face);
        }
        
        optimizeMesh(*mesh);
        
        return mesh;
    }

    static void readData(const std::string& data, size_t& offset, void* buffer, size_t size)
    {
        if (offset + size > data.size())
            throw RBX::runtime_error("Error reading mesh data: offset is out of bounds while reading %d bytes", (int)size);

        memcpy(buffer, data.data() + offset, size);
        offset += size;
    }

    static shared_ptr<FileMeshData> readMeshFromV2(const std::string& data, size_t offset)
    {
        shared_ptr<FileMeshData> mesh(new FileMeshData());
        
        FileMeshHeader header;
        readData(data, offset, &header, sizeof(header));

        if (header.cbSize != sizeof(FileMeshHeader) || header.cbVerticesStride != sizeof(FileMeshVertexNormalTexture3d) || header.cbFaceStride != sizeof(FileMeshFace))
            throw std::runtime_error("Error reading mesh data: incompatible stride");

        if (header.num_vertices == 0 || header.num_faces == 0)
            throw std::runtime_error("Error reading mesh data: empty mesh");

        mesh->vnts.resize(header.num_vertices);
        readData(data, offset, &mesh->vnts[0], header.num_vertices * header.cbVerticesStride);
        
        mesh->faces.resize(header.num_faces);
        readData(data, offset, &mesh->faces[0], header.num_faces * header.cbFaceStride);

        if (offset != data.size())
            throw std::runtime_error("Error reading mesh data: unexpected data at end of file");

        // validate indices to avoid buffer overruns later
        for (auto& face: mesh->faces)
            if (face.a >= header.num_vertices || face.b >= header.num_vertices || face.c >= header.num_vertices)
                throw std::runtime_error("Error reading mesh data: index value out of range");
        
        return mesh;
    }

	FileMeshData* computeAABB(FileMeshData* mesh)
	{
		if (mesh->vnts.empty())
		{
			mesh->aabb = AABox(Vector3::zero());
		}
		else
		{
			AABox result = AABox(Vector3(mesh->vnts[0].vx, mesh->vnts[0].vy, mesh->vnts[0].vz));
			
			for (size_t i = 1; i < mesh->vnts.size(); ++i)
				result.merge(Vector3(mesh->vnts[i].vx, mesh->vnts[i].vy, mesh->vnts[i].vz));
				
			mesh->aabb = result;
		}
			
		return mesh;
	}
}

namespace RBX
{
    shared_ptr<FileMeshData> ReadFileMesh(const std::string& data)
    {
        std::string::size_type versionEnd = data.find('\n');
        if (versionEnd == std::string::npos)
            throw std::runtime_error("Error reading mesh data: unknown version");
  
        shared_ptr<FileMeshData> result;

        if (data.compare(0, 12, "version 1.00") == 0)
            result = readMeshFromV1(data, versionEnd + 1, 0.5f);
        else if (data.compare(0, 12, "version 1.01") == 0)
            result = readMeshFromV1(data, versionEnd + 1, 1.0f);
        else if (data.compare(0, 12, "version 2.00") == 0)
            result = readMeshFromV2(data, versionEnd + 1);
        else
            throw std::runtime_error("Error reading mesh data: unknown version");

        computeAABB(result.get());
        
        return result;
    }

	void WriteFileMesh(std::ostream& f, const FileMeshData& data)
	{
		f << "version 2.00" << std::endl;
		
		FileMeshHeader header;
		header.num_faces = data.faces.size();
		header.num_vertices = data.vnts.size();
		header.cbFaceStride = (unsigned char)sizeof(data.faces[0]);
		header.cbVerticesStride = (unsigned char)sizeof(data.vnts[0]);
		header.cbSize = sizeof(header);
		f.write(reinterpret_cast<char*>(&header), sizeof(header));

		f.write(reinterpret_cast<const char*>(&data.vnts[0]), sizeof(data.vnts[0]) * data.vnts.size());
		f.write(reinterpret_cast<const char*>(&data.faces[0]), sizeof(data.faces[0]) * data.faces.size());
	}
}
