#include "stdafx.h"
#include "ObjectExporter.h"

#include "GfxBase/AsyncResult.h"
#include "GfxBase/PartIdentifier.h"
#include "SceneUpdater.h"
#include "FastCluster.h"
#include "Material.h"
#include "MaterialGenerator.h"
#include "MegaCluster.h"
#include "TextureManager.h"
#include "Image.h"

#include "util/base64.hpp"
#include "g3d/GImage.h"
#include "g3d/BinaryOutput.h"
#include "humanoid/Humanoid.h"

#include "v8datamodel/MegaCluster.h"
#include "V8DataModel/Decal.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/PartCookie.h"

#include "voxel2/Grid.h"
#include "voxel2/Mesher.h"

#include <stack>

#include "VisualEngine.h"

namespace RBX
{
namespace Graphics
{

	/************************************************************************/
	/* Base File Exporter													*/
	/************************************************************************/

	void ObjectExporter::clear()
	{
        instanceChecker.clear();
		materialMap.clear();
		nameCount.clear();
		models.clear();
	}

	bool ObjectExporter::exportToFile(const std::string& fileName, ExporterSaveType saveType, ExporterFormat fileFormat, DataModel* dataModel, VisualEngine* visualEngine)
	{
		scoped_ptr<ObjectExporter> exporter;
		switch (fileFormat)
		{
		case ExporterFormat_Obj:
			exporter.reset(new ObjectExporterObj(visualEngine, dataModel));
			break;
		default:
			return false;
		}

		return exporter->exportToFile(fileName, saveType);
	}

    bool ObjectExporter::exportToFile(const std::string& fileName, ExporterSaveType saveType)
    {
        clear();
        prepareScene(saveType);
        return saveFiles(fileName);
    }

    bool ObjectExporter::exportToJSON(ExporterSaveType saveType, ExporterFormat fileFormat, DataModel* dataModel, VisualEngine* visualEngine, bool encodeBase64,  std::string* strOut)
    {
        scoped_ptr<ObjectExporter> exporter;
        switch (fileFormat)
        {
        case ExporterFormat_Obj:
            exporter.reset(new ObjectExporterObj(visualEngine, dataModel));
            break;
        default:
            return false;
        }

        return exporter->exportToJSON(saveType, fileFormat, encodeBase64, strOut);
    }

    bool ObjectExporter::exportToJSON(ExporterSaveType saveType, ExporterFormat fileFormat, bool encodeBase64, std::string* strOut)
    {
        clear();
        prepareScene(saveType);
        return getJSON(strOut, encodeBase64);
    }

    bool ObjectExporter::prepareScene(ExporterSaveType saveType)
    {
        Workspace* workspace = (dataModel->getWorkspace());

        switch (saveType)
        {
        case ExporterSaveType_Everything:
            prepareModelsTree(static_cast<Instance*>(workspace));
            break;
        case ExporterSaveType_Selection:
            prepareModelsFromSelection();
            break;
        default:
            return false;
        }

        if (visualEngine->getSettings()->getObjExportMergeByMaterial())
            prepareMaterialMerge();

        return true;
    }

    void ObjectExporter::createTerrainModel(const TerrainVerts& vertArray, const std::string& name, const ExporterMaterial* material, const Vector3& toWorldSpace)
    {
        size_t indexCnt = vertArray.size() / 4 * 6;
        size_t vertCnt = vertArray.size();

        Model modelNew(name, vertCnt, indexCnt, material);
        models.push_back(modelNew);
        Model& model = models.back();

        for (unsigned i = 0; i < vertArray.size(); ++i)
        {
            model.vertices[i].pos = vertArray[i].pos + toWorldSpace;
            model.vertices[i].normal = vertArray[i].normal;
            model.vertices[i].uv = vertArray[i].uv;
        }

        // create indices, they are simple quads
        unsigned indexOffset = 0;
        for (unsigned i = 0; i < indexCnt; i += 6)
        {
            model.indices[i + 0] = 0 + indexOffset;
            model.indices[i + 1] = 1 + indexOffset;
            model.indices[i + 2] = 3 + indexOffset;
            model.indices[i + 3] = 1 + indexOffset;
            model.indices[i + 4] = 2 + indexOffset;
            model.indices[i + 5] = 3 + indexOffset;

            indexOffset += 4;
        }			
    }

    void ObjectExporter::createSmoothTerrainModel(const std::vector<Voxel2::Mesher::GraphicsVertex>& vertices, const std::vector<unsigned int>& indices, const std::string& name, const ExporterMaterial* material)
    {
        Model modelNew(name, vertices.size(), indices.size(), material);
        models.push_back(modelNew);
        Model& model = models.back();

        for (unsigned i = 0; i < vertices.size(); ++i)
        {
            model.vertices[i].pos = Voxel::cellSpaceToWorldSpace(vertices[i].position);
            model.vertices[i].normal = (Vector3(vertices[i].normal.r, vertices[i].normal.g, vertices[i].normal.b) - Vector3(127.0)) / 127.0;
            model.vertices[i].uv = Vector2(0, 0);
        }

        model.indices.assign(indices.begin(), indices.end());
    }

	void ObjectExporter::prepareTerrain(MegaClusterInstance* megaCluster)
	{
		Voxel::Grid* grid = megaCluster->getVoxelGrid();

		std::vector<SpatialRegion::Id> activeIDs = grid->getNonEmptyChunks(); 

		// get the texture first
		TextureRef texRef = visualEngine->getTextureManager()->load(ContentId(MegaCluster::kTerrainTexClose + ".dds"), TextureManager::Fallback_White);
		boost::shared_ptr<Texture> tex = texRef.getTexture();
            
		ExporterMaterial solidMaterial("TerrainSolid", Color4uint8(255,255,255,255), 0, 0, tex);
		ExporterMaterial waterMaterial("TerrainWater", Color4uint8(0,0,255,255), 0, 0, shared_ptr<Texture>());
		
		bool needToOutputTerrainTex = false;
		std::vector<MegaCluster::TerrainVertexFFP> vertArray;
		for (std::vector<SpatialRegion::Id>::iterator it = activeIDs.begin(); it != activeIDs.end(); ++it)
		{
            std::string name;
            Vector3 toWorldSpace = SpatialRegion::smallestCornerOfRegionInGlobalCoordStuds(*it).toVector3();

			vertArray.clear();
            MegaCluster::generateAndReturnSolidGeometry(grid, *it, &vertArray);
            if (!vertArray.empty())
            {
                needToOutputTerrainTex = true;

                name = getUniqueModelName(megaCluster->getName());
                solidMaterial.name = name;
                createTerrainModel(vertArray, name, getMutualMaterial(solidMaterial), toWorldSpace);
            }

            vertArray.clear();
            MegaCluster::generateAndReturnWaterGeometry(grid, *it, &vertArray);
            if (!vertArray.empty())
            {
                name = getUniqueModelName(megaCluster->getName());
                waterMaterial.name = name;
                createTerrainModel(vertArray, name, getMutualMaterial(waterMaterial), toWorldSpace);
            }
		}
		
        if (needToOutputTerrainTex && tex.get())
        {
            TexFiles::iterator texIt = textureMap.find(tex.get());
            if (texIt == textureMap.end())
                textureMap[tex.get()] = megaCluster->getName();
        }
	}

	void ObjectExporter::prepareSmoothTerrain(MegaClusterInstance* megaCluster)
	{
        using namespace Voxel2::Mesher;

		ExporterMaterial solidMaterial("TerrainSolid", Color4uint8(77,80,36,255), 0, 0, shared_ptr<Texture>());
		ExporterMaterial waterMaterial("TerrainWater", Color4uint8(51,134,158,255), 0, 0, shared_ptr<Texture>());

        Voxel2::Grid* grid = megaCluster->getSmoothGrid();

        for (auto& gr: grid->getNonEmptyRegions())
        {
            Voxel2::Region region = gr.expand(2);
            Voxel2::Box box = grid->read(region);

            Options options = { megaCluster->getMaterialTable(), /* generateWater= */ true };
            BasicMesh geometry = generateGeometry(box, region.begin(), 0, options);
        	GraphicsMesh graphicsGeometry = generateGraphicsGeometry(geometry, options);

            if (!graphicsGeometry.solidIndices.empty())
            {
                std::string name = getUniqueModelName(megaCluster->getName());

                createSmoothTerrainModel(graphicsGeometry.vertices, graphicsGeometry.solidIndices, name, getMutualMaterial(solidMaterial));
            }

            if (!graphicsGeometry.waterIndices.empty())
            {
                std::string name = getUniqueModelName(megaCluster->getName());

                createSmoothTerrainModel(graphicsGeometry.vertices, graphicsGeometry.waterIndices, name, getMutualMaterial(waterMaterial));
            }
        }
	}

    const ObjectExporter::ExporterMaterial* ObjectExporter::getMutualMaterial(const ExporterMaterial& mat)
    {
        if (materialMap.find(mat) == materialMap.end())
            materialMap.insert(mat);

        return &(*materialMap.find(mat));
    }

    std::string ObjectExporter::getUniqueModelName(const std::string& nameIn)
	{
		// names has to be unique, so different parts will be different objects in file
        // fix the string name, so it contains just characters and numbers. It also converts empty spaces from "some thing" to "SomeThing"
        std::string name;
        bool upperCase = true;
        for (unsigned i = 0; i < nameIn.length(); ++i)
        {
            char ch = nameIn[i];
            if (ch == ' ')
            {
                upperCase = true;
                continue;
            }
            if (isalnum(ch))
            {
                name += upperCase ? toupper(ch) : tolower(ch);
                upperCase = false;
            }
        }
      
        ++nameCount[name];

        return name + format("%i", nameCount[name]);
	}

	void ObjectExporter::processPart(PartInstance* part, Decal* decal)
	{
        // early out for fully transparent objects
        if (part->getTransparencyUi() >= 1)
            return;

		GeometryGenerator count;
		Humanoid* humanoid = SceneUpdater::getHumanoid(part);
        HumanoidIdentifier humanoidIdentifier(humanoid);
        const HumanoidIdentifier* hi = humanoidIdentifier.humanoid ? &humanoidIdentifier : NULL;

		// Material flags
		bool ignoreDecals = false;
		unsigned int materialFlags = MaterialGenerator::createFlags(false, part, hi, ignoreDecals);
		
		if (ignoreDecals && decal)
			return;
		
		// Decal inherits the transparency property of the part - for transparent parts we have to sort their decals, but for opaque parts
		// decals can be just blended after the opaque geometry.
		// Since it's a decal, it will be alpha blended whether we ask it to be transparent or not.
		if (decal)
			materialFlags = materialFlags & (MaterialGenerator::Flag_Skinned | MaterialGenerator::Flag_Transparent);

		// fetch any resources the part might need
		unsigned int resourceFlags =
			((materialFlags & MaterialGenerator::Flag_UseCompositTexture) || (hi && hi->isPartHead(part)))
			? GeometryGenerator::Resource_SubstituteBodyParts
			: 0;

		AsyncResult asyncResult;
		GeometryGenerator::Resources resources = GeometryGenerator::fetchResources(part, hi, resourceFlags, &asyncResult);

		GeometryGenerator::Options options = GeometryGenerator::Options();
		count.addInstance(part, decal, options, resources);
		if (count.getVertexCount() > 0 && count.getIndexCount() >= 3)
		{
            std::string partName;
            if (hi)
            {
                if(hi->humanoid->getParent())
                    partName = getUniqueModelName(hi->humanoid->getParent()->getName());
                else
                    partName = getUniqueModelName(hi->humanoid->getName());
            }
            else
                partName = getUniqueModelName(part->getName());

			MaterialGenerator::Result materialResult = visualEngine->getMaterialGenerator()->createMaterial(part, decal, hi, materialFlags);
            if (!materialResult.material)
                return;

			options = GeometryGenerator::Options(visualEngine, *part, decal, part->getPart().coordinateFrame, materialResult.flags, materialResult.uvOffsetScale, 0);
            // make sure that block randomization is turned off
            options.flags &= ~GeometryGenerator::Flag_RandomizeBlockAppearance; 

			// MODEL
			models.push_back(Model(partName, count.getVertexCount(), count.getIndexCount(), NULL));
			Model& model = models.back();

			GeometryGenerator generator(&model.vertices[0], &model.indices[0], 0);
			generator.addInstance(part, decal, options, resources);

			// MATERIAL
			std::string matName = partName;
			matName.append("Mtl");

			shared_ptr<Texture> tex;
			const std::vector<Technique>& techniques = materialResult.material->getTechniques();
			if (!techniques.empty()) 
			{
				const TextureRef& texRef = techniques[0].getTexture(visualEngine->getMaterialGenerator()->getDiffuseMapStage());
				if (texRef.getTexture() && 
                    !visualEngine->getTextureManager()->isFallbackTexture(texRef.getTexture()) && 
                    texRef.getTexture()->getFormat() == Texture::Format_RGBA8)
                {
					tex = texRef.getTexture();
                }
            }

			Color4uint8 vertColor = model.vertices[0].color;
			if (visualEngine->getDevice()->getCaps().colorOrderBGR)
			{
                std::swap(vertColor.r, vertColor.b);
			}

            Vector2int16 plasticSpecular = MaterialGenerator::getSpecular(PLASTIC_MATERIAL);
			static const float div255 = 1.0f / 255.0f;
			bool mergeByMat = visualEngine->getSettings()->getObjExportMergeByMaterial();
            float specularIntensity = (mergeByMat ? plasticSpecular.x : model.vertices[0].extra.g) * div255;
			float specularPower = (mergeByMat ? plasticSpecular.y : model.vertices[0].extra.b);

            model.material = getMutualMaterial(ExporterMaterial(matName, vertColor, specularIntensity, specularPower, tex));

			// TEXTURES 
			// We want to hashmap textures so we dont save the same textures on disc
			if (tex.get())
			{
				TexFiles::iterator texIt = textureMap.find(tex.get());
				if (texIt == textureMap.end())
					textureMap[tex.get()] = partName;
			}

			// if decal we need to offset it from geometry bellow it
            if (decal)
			    for (unsigned i = 0; i < model.vertices.size(); ++i)
			    {
					static const float offset = 0.02f;
					model.vertices[i].pos += model.vertices[i].normal * offset;
			    }
		}
	}

	void ObjectExporter::processInstance(Instance* instance)
	{
		if (PartInstance* part = Instance::fastDynamicCast<PartInstance>(instance))
		{
			processPart(part, NULL);

			if ((part->getCookie() & PartCookie::HAS_DECALS) && part->getChildren())
			{
				const Instances& children = *part->getChildren();
				for (size_t i = 0; i < children.size(); ++i)
					if (Decal* decal = children[i]->fastDynamicCast<Decal>())
						processPart(part, decal);
			}
		}
        if (MegaClusterInstance* megaCluster = Instance::fastDynamicCast<MegaClusterInstance>(instance))
        {
			if (megaCluster->isSmooth())
				prepareSmoothTerrain(megaCluster);
			else
				prepareTerrain(megaCluster);
        }
	}

    void ObjectExporter::prepareMaterialMerge()
    {
        for (Models::iterator it = models.begin(); it != models.end(); ++it)
        {
            ExporterMaterial* mat = const_cast<ExporterMaterial*>(it->material);
            mat->models.push_back(&(*it));
        }
    }

	void ObjectExporter::prepareModelsFromSelection()
	{
		RBX::Selection* selection = RBX::ServiceProvider::create< RBX::Selection >(dataModel);
    
		for (Instances::const_iterator it = selection->begin(); it != selection->end(); ++it)
			prepareModelsTree(it->get());
	}

	void ObjectExporter::prepareModelsTree(Instance* rootInstance)
	{
		// walk the whole tree from root instance
		std::stack<Instance*> instances;
		instances.push(rootInstance);

		while(!instances.empty())
		{
			Instance* instance = instances.top();
			instances.pop();

            if (instanceChecker.find(instance) != instanceChecker.end())
                continue;
            instanceChecker.insert(instance);

			processInstance(instance);

			if (instance->getChildren())
			{
				const Instances& children = *instance->getChildren();
				for (size_t i = 0; i < children.size(); ++i)
				{
					instances.push(children[i].get());
				}
			}
		}
	}

    AABox ObjectExporter::computeAABB()
    {
        if (models.empty())
            return AABox::zero();

        AABox aabb = AABox((*models.begin()).vertices[0].pos);
        for (Models::iterator it = models.begin(); it != models.end(); ++it)
        {
            for (unsigned i = 0; i < it->vertices.size(); ++i)
                aabb.merge(it->vertices[i].pos);
        }

        return aabb;
    }

    bool ObjectExporter::getImage(Texture* tex, G3D::GImage& imgOut)
    {
        Texture::Format format = tex->getFormat();

        unsigned w = tex->getWidth();
        unsigned h = tex->getHeight();
        unsigned int length = Texture::getImageSize(format, w, h);

        imgOut = G3D::GImage(w, h, 4);

        switch (format)
        {
        case Texture::Format_BC1:
        case Texture::Format_BC2:
        case Texture::Format_BC3:
            {	
                boost::scoped_array<unsigned char> data(new unsigned char[length]);
                if (!tex->download(0, 0, data.get(), length))
                    return false;
                Image::decodeDXT(imgOut.byte(), data.get(), w, h, 1, format);
                break;
            }
        case Texture::Format_RGBA8:
            {
                if (!tex->download(0, 0, imgOut.byte(), length))
                    return false;

                // in this case we have to swap to RGB
                if (visualEngine->getDevice()->getCaps().colorOrderBGR)
                {
                    unsigned char* data = imgOut.byte();
                    for (unsigned int i = 0; i < length / 4; ++i)
                    {
                        unsigned idx = i*4;
                        std::swap(data[idx], data[idx+2]);
                    }
                }

                break;
            }
        default:
            return false;
        }

        return true;
    }

    bool ObjectExporter::getBase64Tex(Texture* tex, std::string& strOut)
    {
        G3D::GImage img;
        if (!getImage(tex, img))
            return false;

        // TODO: isnt this a bit slow for something that should provide picture of user?
        G3D::BinaryOutput binaryOutput;
        binaryOutput.setEndian(G3D::G3D_LITTLE_ENDIAN);

        img.encode(G3D::GImage::PNG, binaryOutput);
        base64<char>::encode((const char*)binaryOutput.getCArray(), binaryOutput.length(), strOut, base64<>::noline());

        return true;
    }

	bool ObjectExporter::saveTexture(const std::string& filename, Texture* tex)
	{
        G3D::GImage img;
        if (!getImage(tex, img))
            return false;

        G3D::BinaryOutput binaryOutput;
        binaryOutput.setEndian(G3D::G3D_LITTLE_ENDIAN);

        img.encode(G3D::GImage::PNG, binaryOutput);

        std::ofstream out(filename.c_str(), std::ios::out | std::ios::binary);
		if (!out)
			return false;

        out.write(reinterpret_cast<const char*>(binaryOutput.getCArray()), binaryOutput.length());

		return true;
	}

	/************************************************************************/
	/* OBJ Exporter                                                         */
	/************************************************************************/

    void ObjectExporterObj::exportObj(std::ostream& exportStream, bool encodeBase64 /* = false */ )
    {
        unsigned vertexOffset = 1; // obj vertices starts at index 

        std::stringstream strStream;

        if (visualEngine->getSettings()->getObjExportMergeByMaterial())
        {
            for (MatarialSet::iterator it = materialMap.begin(); it != materialMap.end(); ++it)
            {
                const ExporterMaterial& mat = *it;

                strStream << "g " << it->name + "_Obj" << "\n\n";

                // Verts
                for (size_t i = 0; i < mat.models.size(); ++i)
                {
                    const Model* model = mat.models[i];                    
                    for (unsigned i = 0; i < model->vertices.size(); ++i)
                        exportVector(strStream, "v", model->vertices[i].pos);
                    strStream << "\n";
                }
                // Normals
                for (size_t i = 0; i < mat.models.size(); ++i)
                {
                    const Model* model = mat.models[i];
                    for (unsigned i = 0; i < model->vertices.size(); ++i)
                        exportVector(strStream, "vn", model->vertices[i].normal);
                    strStream << "\n";
                }
                // UVs
                for (size_t i = 0; i < mat.models.size(); ++i)
                {
                    const Model* model = mat.models[i];
                    for (unsigned i = 0; i < model->vertices.size(); ++i)
                        exportVector(strStream, "vt", Vector2(model->vertices[i].uv.x, 1.0f - model->vertices[i].uv.y));
                    strStream << "\n";
                }
        
                // Material (just reference)
                strStream << "usemtl " << it->name << "\n\n" ; 

                // Faces
                for (size_t i = 0; i < mat.models.size(); ++i)
                {
                    const Model* model = mat.models[i];
                    for (unsigned i = 0; i < model->indices.size(); i += 3)
                        exportIndices(strStream, "f", &model->indices[0], i, vertexOffset);
                    strStream << "\n";

                    vertexOffset += model->vertices.size();
                }
            }
        }
        else
        {
            for (Models::iterator it = models.begin(); it != models.end(); ++it)
            {
                strStream << "g " << it->name << "\n\n";

                // Verts
                for (unsigned i = 0; i < it->vertices.size(); ++i)
                    exportVector(strStream, "v", it->vertices[i].pos);
                strStream << "\n";

                // Normals
                for (unsigned i = 0; i < it->vertices.size(); ++i)
                    exportVector(strStream, "vn", it->vertices[i].normal);
                strStream << "\n";

                // UVs
                for (unsigned i = 0; i < it->vertices.size(); ++i)
                    exportVector(strStream, "vt", Vector2(it->vertices[i].uv.x, 1.0f - it->vertices[i].uv.y));
                strStream << "\n";

                // Material (just reference)
                strStream << "usemtl " << it->material->name << "\n\n" ; 

                // Faces
                for (unsigned i = 0; i < it->indices.size(); i += 3)
                    exportIndices(strStream, "f", &it->indices[0], i, vertexOffset);
                strStream << "\n";

                vertexOffset += it->vertices.size();
            }
        }

        if (encodeBase64)
        {
            std::string tmpstr;
            base64<char>::encode(strStream.str().c_str(), strStream.str().length(), tmpstr, base64<>::noline());
            exportStream << tmpstr;
        }
        else
            exportStream << strStream.rdbuf();
    }

    void ObjectExporterObj::exportMtl(std::ostream& exportStream, bool encodeBase64 /* = false */)
    {
        std::stringstream strStream;

        for (MatarialSet::iterator it = materialMap.begin(); it != materialMap.end(); ++it)
        {
            const ExporterMaterial& mat = *it;

            Color3 color(mat.color.rgb());
            strStream << "newmtl " << mat.name << "\n";
            strStream << "Material Color\n";
            strStream << "Ka " << color.r << " " << color.g << " " << color.b << "\n";
            strStream << "Kd " << color.r << " " << color.g << " " << color.b << "\n";
            strStream << "Ks " << color.r * mat.specularIntensity << " " <<  color.g * mat.specularIntensity << " " << color.b * mat.specularIntensity << " \n\n";

            strStream << "d " << mat.color.a / 255.0f << "\n\n";
            strStream << "Ns " << mat.specularPower * 255.0f << "\n\n";

            // does it use texture? and if does, which texture
            if (mat.texture.get())
            {
                TexFiles::iterator texIt = textureMap.find(mat.texture.get());
                if (texIt != textureMap.end())
                {
                    std::string texName = textureMap[mat.texture.get()];
                    texName.append("Tex.png");
                    strStream << "map_Ka " << texName << "\n";
                    strStream << "map_Kd " << texName << "\n";
                    strStream << "map_d "  << texName << "\n\n";
                }
            }
        }

        if (encodeBase64)
        {
            std::string tmpstr;
            base64<char>::encode(strStream.str().c_str(), strStream.str().length(), tmpstr, base64<>::noline());
            exportStream << tmpstr;
        }
        else
            exportStream << strStream.str();
    }

    bool ObjectExporterObj::getJSON(std::string* JSONdataOut, bool encodeBase64)
    {
        if (!models.empty())
        {
            std::stringstream strStream;
            
            // first insert camera position
            const RenderCamera& camera = visualEngine->getCamera();
            Matrix3 camMat = camera.getViewMatrix().upper3x3().inverse();
            Vector3 pos = camera.getPosition();
            Vector3 dir = camMat.column(2);
            AABox aabb = computeAABB();

            strStream << "{\n";
            strStream << "\t\"camera\": \n\t{\n";
            exportVectorJSON(strStream, "position", pos);
            strStream << ",\n";
            exportVectorJSON(strStream, "direction", dir);
            strStream << "\n\t},\n\n";

            strStream << "\t\"AABB\": \n\t{\n";
            exportVectorJSON(strStream, "min", aabb.low());
            strStream << ",\n";
            exportVectorJSON(strStream, "max", aabb.high());
            strStream << "\n\t},\n\n";

            strStream << "\t\"files\": \n\t{\n";
            strStream <<  "\t\t\"scene.obj\": {\"content\": \"";
            exportObj(strStream, encodeBase64);
			strStream << "\"},\n";

            strStream << "\t\t\"scene.mtl\": {\"content\": \"";
            exportMtl(strStream, encodeBase64);
			strStream << "\"}";


            // Now textures, they are base64 allways
            for (TexFiles::iterator it = textureMap.begin(); it != textureMap.end(); ++it)
            {
                Texture* tex = it->first;
                std::string texFile = it->second + "Tex.png";
                std::string texOut;

                if (!getBase64Tex(tex, texOut))
                    return false;
                strStream << ",\n\t\t\""+ texFile +"\": {\"content\": \"";
                strStream << texOut;
                strStream << "\"}";
            }

            strStream << "\n\t}";
            strStream << "\n}";

            *JSONdataOut = strStream.str();

            return true;
        }
        return false;
    }

	bool ObjectExporterObj::saveFiles(const std::string& fileName)
	{
		if (!models.empty())
		{
			std::string objFileName = fileName + ".obj";
			std::string mtlFileName = fileName + ".mtl";
			std::string mtlName;
			std::string folder;
    
			size_t pos = mtlFileName.find_last_of('/');
			mtlName = mtlFileName.substr(pos+1);
			folder = mtlFileName.substr(0, pos);

			// Lets dump .OBJ on disk
            std::ofstream exportFile;
			exportFile.open(objFileName.c_str());
			exportFile << "# This file is brought to you by ROBLOX Corporation\n";
			exportFile << "# Generated by ROBLOX Studio\n\n";
			exportFile << "mtllib " << mtlName << "\n\n";

            exportObj(exportFile);
			exportFile.close();

			// Now create MTL		
            exportFile.open(mtlFileName.c_str());
            exportMtl(exportFile);
            exportFile.close();

			// Lets dump the textures
			for (TexFiles::iterator it = textureMap.begin(); it != textureMap.end(); ++it)
			{
				Texture* tex = it->first;

				std::string texFile = folder;
				texFile.append("/").append(it->second).append("Tex.png");
				if (!saveTexture(texFile, tex))
					return false;
			}
            return true;
		}
        return false;
		
	}

	void ObjectExporterObj::exportVector(std::ostream& exportFile, const char* keyWord, const Vector3& v)
	{
		exportFile << keyWord << " ";
		exportFile << v.x << " ";
		exportFile << v.y << " ";
		exportFile << v.z << " ";
		exportFile << "\n";
	}

	void ObjectExporterObj::exportVector(std::ostream& exportFile, const char* keyWord, const Vector2& v)
	{
		exportFile << keyWord << " ";
		exportFile << v.x << " ";
		exportFile << v.y << " ";
		exportFile << "\n";
	}

    void ObjectExporterObj::exportVectorJSON(std::ostream& exportFile, const char* keyWord, const Vector3& v)
    {
        exportFile << "\t\t\"" << keyWord << "\"" << ": {";
        exportFile << "\"x\": " << v.x << ", ";
        exportFile << "\"y\": " << v.y << ", ";
        exportFile << "\"z\": " << v.z << " ";
        exportFile << "}";
    }

	void ObjectExporterObj::exportIndices(std::ostream& exportFile, const char* keyWord, const unsigned short* indices, unsigned startIndex, unsigned offset)
	{
		exportFile << keyWord << " ";
		for (char i = 0; i < 3; ++i)
		{
			// dump it three times, because pos, normal and UVs has same indices
			exportFile << indices[startIndex + i] + offset << "/" << indices[startIndex + i] + offset << "/" << indices[startIndex + i] + offset << " "; 
		}
		exportFile << "\n";	
	}
}
}
