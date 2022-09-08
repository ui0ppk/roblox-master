/************************************************************************/
/*
	This class provides functionality to export scene or its parts to file.

	Usage:
		The easiest way to use it is to call static function of ObjectExporter::exportToFile
		providing what format you want. It will take care of creation 

		ObjectExporter::exportToFile(filename, saveType(whole or selection) , fileFormat, dataModel, visualEngine);

	Extending:
		If you want to write your own exporter, you should be able to do it quite simply
		inheriting from ObjectExporter and writing just saveFile function. See class
		ObjectExporterObj as example
*/
/************************************************************************/

#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

#include "rbx/Declarations.h"

#include "GfxBase/ViewBase.h"
#include "GfxBase/RenderSettings.h"
#include "MegaCluster.h"

#include "GeometryGenerator.h"

namespace G3D
{
    class GImage;
};

namespace RBX
{

class MegaClusterInstance;

namespace Voxel2 { namespace Mesher { struct GraphicsVertex; } }

namespace Graphics
{

class VisualEngine;
class Texture;
class GeometryGenerator;
class Material;


class ObjectExporter
{
public:
	ObjectExporter(VisualEngine* visEngine, DataModel* dataModel): visualEngine(visEngine), dataModel(dataModel) { clear(); }
    virtual ~ObjectExporter() {}

			void clear();

    // todo: This would be cool as factory, but who cares now as we have just one type.
	static	bool exportToFile(const std::string& fileName, ExporterSaveType saveType, ExporterFormat fileFormat, DataModel* dataModel, VisualEngine* visualEngine);
			bool exportToFile(const std::string& fileName, ExporterSaveType saveType); 

    static  bool exportToJSON(ExporterSaveType saveType, ExporterFormat fileFormat, DataModel* dataModel, VisualEngine* visualEngine, bool encodeBase64,  std::string* strOut);
            bool exportToJSON(ExporterSaveType saveType, ExporterFormat fileFormat, bool encodeBase64, std::string* strOut);

protected:

    struct Model;
	struct ExporterMaterial 
	{
		std::string				    name;
		Color4uint8				    color;
		shared_ptr<Texture>		    texture;
		float					    specularIntensity;
		float					    specularPower;
        std::vector<const Model*>   models;
       
		ExporterMaterial()
		{
			color = Color4uint8();
			name = "";
		}

		ExporterMaterial(const std::string& nameIn, const Color4uint8& colorIn, float specularIntensityIn, float specularPowerIn, shared_ptr<Texture> textureIn):
			name(nameIn)
			,color(colorIn)
			,texture(textureIn)
			,specularPower(specularPowerIn)
			,specularIntensity(specularIntensityIn)
		{}

        size_t getHashId() const
        {
            size_t seed = 0;
            boost::hash_combine(seed, color.r);
            boost::hash_combine(seed, color.g);
            boost::hash_combine(seed, color.b);
            boost::hash_combine(seed, color.a);
            boost::hash_combine(seed, texture);
            boost::hash_combine(seed, specularIntensity);
            boost::hash_combine(seed, specularPower);
            return seed;
        }

        bool operator==(const ExporterMaterial &other) const
        { 
            return color == other.color
                && texture == other.texture
                && specularIntensity == other.specularIntensity
                && specularPower == other.specularPower;
        }
	};

    struct MaterialHasher
    {
        std::size_t operator()(const ExporterMaterial& mat) const
        {
            return mat.getHashId();
        }
    };

	struct Model
	{
		std::vector<GeometryGenerator::Vertex>	vertices;
		std::vector<unsigned short>				indices;
		std::string								name;
		const ExporterMaterial* 				material;

		Model(const std::string& nameIn, size_t vertCnt, size_t indexCnt, const ExporterMaterial* materialIn):
			 name(nameIn)
			,material(materialIn)
		{
			vertices.resize(vertCnt);
			indices.resize(indexCnt);
		}
	};

	typedef boost::unordered_map<Texture*, std::string> TexFiles;
	typedef boost::unordered_set<ExporterMaterial, MaterialHasher> MatarialSet;
	typedef boost::unordered_map<std::string, unsigned> NameCounts;
    typedef boost::unordered_set<Instance*> InstanceSet;
    typedef std::vector<MegaCluster::TerrainVertexFFP> TerrainVerts;

	TexFiles		textureMap;
	NameCounts		nameCount; 
	MatarialSet     materialMap;
    InstanceSet     instanceChecker;

	typedef std::vector<Model> Models;
	Models models;

	VisualEngine*	visualEngine;
	DataModel*		dataModel;

    // geometry processing
        bool prepareScene(ExporterSaveType saveType);
        void prepareMaterialMerge();
		void prepareModelsFromSelection();
		void prepareModelsTree(Instance* rootInstance);
		void prepareTerrain(MegaClusterInstance* megaCluster);
		void prepareSmoothTerrain(MegaClusterInstance* megaCluster);
		void processInstance(Instance* instance);
		void processPart(PartInstance* part, Decal* decal);
        void createTerrainModel(const TerrainVerts& vertArray, const std::string& name, const ExporterMaterial* material, const Vector3& toWorldSpace);
        void createSmoothTerrainModel(const std::vector<Voxel2::Mesher::GraphicsVertex>& vertices, const std::vector<unsigned int>& indices, const std::string& name, const ExporterMaterial* material);
        AABox computeAABB();

    // texture manipulation
        bool getImage(Texture* tex, G3D::GImage& imgOut);
		bool saveTexture(const std::string& filename, Texture* tex);
        bool getBase64Tex(Texture* tex, std::string& strOut);

    void strToBase64(const std::stringstream& strStreamInOut);
    std::string getUniqueModelName(const std::string& nameIn);
    const ExporterMaterial* getMutualMaterial(const ExporterMaterial& mat);

	virtual bool saveFiles(const std::string& fileName)=0;
    
    // not mandatory, JSON is more like rare case
    virtual bool getJSON(std::string* JSONdataOut, bool encodeBase64) {return false;}
};

class ObjectExporterObj: public ObjectExporter
{
public:

	ObjectExporterObj(VisualEngine* visEngine, DataModel* dataModel): ObjectExporter(visEngine, dataModel)
	{}

protected:
	virtual bool saveFiles(const std::string& fileName);
    virtual bool getJSON(std::string* JSONdataOut, bool encodeBase64);

    void exportObj(std::ostream& exportStream, bool encodeBase64 = false);
    void exportMtl(std::ostream& exportStream, bool encodeBase64 = false);

	void exportMaterial(ExporterMaterial material);
	void exportVector (std::ostream& exportFile, const char* keyWord, const Vector3& v);
	void exportVector (std::ostream& exportFile, const char* keyWord, const Vector2& v);
	void exportIndices(std::ostream& exportFile, const char* keyWord, const unsigned short* indices, unsigned startIndex, unsigned offset);
    void exportVectorJSON (std::ostream& exportFile, const char* keyWord, const Vector3& v);
};

}
}
