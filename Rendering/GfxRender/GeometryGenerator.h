#pragma once

#include "util/G3DCore.h"

#include <boost/shared_ptr.hpp>

#include "rbx/Debug.h"

namespace RBX
{
    class CSGMesh;
	class PartOperation;
	class PartInstance;
	class AsyncResult;
	struct FileMeshData;
	class DataModelMesh;
	class HumanoidIdentifier;
	class Decal;
	
	namespace Graphics
	{
		class VisualEngine;
	}
}
	
namespace RBX
{
namespace Graphics
{
	class GeometryGenerator
	{
	public:
		struct Vertex
		{
			Vector3 pos;
			Vector3 normal;
			Color4uint8 color;
			Color4uint8 extra; // red = bone index; green = specular intensity; blue = specular power, alpha = reflectance
			Vector2 uv;
			Vector2 uvStuds;
			
			Vector3 tangent;
			Vector4 edgeDistances;
		};
	
		enum GenerationFlags
		{
			// use vertex color from part
			Flag_VertexColorPart = 1 << 0,
			// use vertex color from mesh
			Flag_VertexColorMesh = 1 << 1,
			// use uv frame to transform UVs
			Flag_TransformUV = 1 << 2,
			// use BGRA ordering of diffuse color components
			Flag_DiffuseColorBGRA = 1 << 3,
			// randomize block normals/colors based on part pointer
			Flag_RandomizeBlockAppearance = 1 << 4,
			// ignore studs and always make part smooth (needed for backward compatibility with non-plastic materials ingame)
			Flag_IgnoreStuds = 1 << 5,
			// extra field will only contain zeroes
			Flag_ExtraIsZero = 1 << 6
		};
		
		enum ResourceFlags
		{
			// use body part substitution (substituted meshes only work with composited textures due to UV layout)
			Resource_SubstituteBodyParts = 1 << 0,
		};
		
		struct Options
		{
			Options()
				: flags(0)
				, extra(0)
			{
			}

			Options(VisualEngine* visualEngine, const PartInstance& part, Decal* decal, const CoordinateFrame& localTransform, unsigned int materialFlags, const Vector4& uvOffsetScale, unsigned int extra);

			unsigned int flags;
			CoordinateFrame cframe;
			Vector4 uvframe;
			unsigned char extra;
		};

		class Resources
        {
        public:
            Resources() {}
            Resources(boost::shared_ptr<FileMeshData> fileMesh) { fileMeshData = fileMesh; }
            Resources(boost::shared_ptr<CSGMesh> csgMesh) { csgMeshData = csgMesh; }

            boost::shared_ptr<FileMeshData> fileMeshData;
            boost::shared_ptr<CSGMesh> csgMeshData;
        };
		
		static Resources fetchResources(PartInstance* part, const HumanoidIdentifier* hi, unsigned int flags, AsyncResult* asyncResult);
		
		GeometryGenerator();
		GeometryGenerator(Vertex* vertices, unsigned short* indices, unsigned int vertexOffset = 0);
		
		void	reset();
		void	resetBounds();
		void	resetBounds(const Vector3& min, const Vector3& max);
		
		void	addInstance(PartInstance* part, Decal* decal, const Options& options, const Resources& resources, const HumanoidIdentifier* humanoidIdentifier = NULL);
        void    addCSGPrimitive(PartInstance* part);

		unsigned int	getVertexCount()	const { return mVertexCount; }
		unsigned int	getIndexCount()		const { return mIndexCount; }

		bool areBoundsValid() const
		{
			RBXASSERT(mVertices); // geometry is not generated if mVertices is NULL
			return mBboxMin.x <= mBboxMax.x;
		}
		
		const AABox getBounds() const
		{
			RBXASSERT(mVertices); // geometry is not generated if mVertices is NULL
			return AABox(mBboxMin, mBboxMax);
		}
		
	private:
		Vector3 mBboxMin, mBboxMax;

		Vertex* mVertices;
		unsigned short* mIndices;
		
		unsigned int mVertexCount;
		unsigned int mIndexCount;
		
		void addPartImpl(PartInstance* part, Decal* decal, const Options& options, const Resources& resources, const HumanoidIdentifier* hi, bool ignoreMaterialsStuds);
		
		void addBlock(const Vector3& size, const Vector3& center, PartInstance* part, Decal* decal, const Options& options, unsigned int randomSeed, bool ignoreMaterialsStuds);
		template<bool corner> void addWedge(const Vector3& size, const Vector3& center, PartInstance* part, Decal* decal, const Options& options, unsigned int randomSeed, bool ignoreMaterialsStuds);
		void addSphere(const Vector3& size, const Vector3& center, PartInstance* part, Decal* decal, const Options& options, unsigned int randomSeed, bool ignoreMaterialsStuds); 
		template<bool vertical> void addCylinder(const Vector3& size, const Vector3& center, PartInstance* part, Decal* decal, const Options& options, unsigned int randomSeed, bool ignoreMaterialsStuds); 
		void addTorso(const Vector3& size, const Vector3& center, PartInstance* part, Decal* decal, const Options& options, unsigned int randomSeed);
		void addTruss(int style, const Vector3& size, PartInstance* part, Decal* decal, const Options& options, unsigned int randomSeed);
		void addTrussY(const Vector3& origin, const Vector3& axisX, const Vector3& axisY, const Vector3& axisZ, int segments, int style, const Vector3& size, PartInstance* part, const Options& options, unsigned int randomSeed);
		void addTrussSideX(const Vector3& origin, const Vector3& axisX, const Vector3& axisY, const Vector3& axisZ, bool reverseBridge, int segments, int style, const Vector3& size, PartInstance* part, const Options& options, unsigned int randomSeed);
		void addFileMesh(FileMeshData* data, DataModelMesh* specialShape, PartInstance* part, Decal* decal, const HumanoidIdentifier* humanoidIdentifier, const Options& options);
		void addOperation(PartInstance* part, Decal* decal, const Options& options, const Resources& resources, unsigned int randomSeed);
		void addOperationDecompositionDebug(PartOperation* operation, PartInstance* part, Decal* decal, const Options& options, unsigned int randomSeed);

		void fillDecompShapeDebug(PartOperation* operation, PartInstance* part, const Options &options);
		void fillBlockShapeDebug(PartInstance* part, const Options &options);
	};
}
}