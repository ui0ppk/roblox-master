#include "stdafx.h"
#include "voxel2/MaterialTable.h"

#include <fstream>
#include <iterator>

#include <rapidjson/document.h>

#include "StringConv.h"

namespace RBX { namespace Voxel2 {

    static double getNumberOr(const rapidjson::Value& node, double def)
	{
		return node.IsNumber() ? node.GetDouble() : def;
	}

    static const char* getStringOr(const rapidjson::Value& node, const char* def)
	{
		return node.IsString() ? node.GetString() : def;
	}

    static void parseDeformation(MaterialTable::Material& material, MaterialTable::Deformation deformation, const rapidjson::Value& node)
    {
        if (node.IsNumber())
        {
            material.deformation = deformation;
            material.parameter = node.GetDouble();
        }
    }

    static MaterialTable::Type parseType(const char* value)
    {
        if (strcmp(value, "hard") == 0)
            return MaterialTable::Type_Hard;

        if (strcmp(value, "hardsoft") == 0)
            return MaterialTable::Type_HardSoft;

        return MaterialTable::Type_Soft;
    }

    static MaterialTable::Mapping parseMapping(const char* value)
    {
        if (strcmp(value, "cube") == 0)
            return MaterialTable::Mapping_Cube;

        return MaterialTable::Mapping_Default;
    }

    static unsigned int parseMaterialLayer(std::vector<MaterialTable::Layer>& layers, const rapidjson::Value& node)
    {
        MaterialTable::Layer desc;

		desc.tiling = getNumberOr(node["tiling"], 1);
		desc.detiling = getNumberOr(node["detiling"], 0);

        layers.push_back(desc);

        return layers.size() - 1;
    }

    MaterialTable::MaterialTable(const std::string& file, unsigned int materialCount)
    {
		try
		{
			load(file);
		}
		catch (RBX::base_exception& e)
		{
			StandardOut::singleton()->printf(MESSAGE_ERROR, "MaterialTable: failed to load %s: %s", file.c_str(), e.what());

			Atlas dummy = {};
			dummy.width = 1;
			dummy.height = 1;
			dummy.tileCount = 1;

			atlas = dummy;
		}

		if (layers.size() < 1)
		{
			Layer dummy = {};
			dummy.tiling = 1;

			layers.resize(1, dummy);
		}

		if (materials.size() < materialCount)
		{
			Material dummy = {};

			materials.resize(materialCount, dummy);
		}
    }
    
    MaterialTable::~MaterialTable()
    {
    }
    
    void MaterialTable::load(const std::string& file)
    {
        using namespace rapidjson;

       std::ifstream in(utf8_decode(file).c_str(), std::ios::in | std::ios::binary);

        if (!in)
            throw RBX::runtime_error("Error opening file %s", file.c_str());

        std::ostringstream data;
        data << in.rdbuf();

        std::string datastring = data.str();
 
        Document root;
        root.Parse<kParseDefaultFlags>(datastring.c_str());

        if (root.HasParseError())
			throw RBX::runtime_error("Failed to parse JSON: %s at %d", root.GetParseError(), int(root.GetErrorOffset()));

		std::string platform = getStringOr(root["platform"], "");
		const Value& atlasJson = root["atlas"][platform.c_str()];

		if (atlasJson.IsObject())
		{
			atlas.width = getNumberOr(atlasJson["width"], 1);
			atlas.height = getNumberOr(atlasJson["height"], 1);
			atlas.tileSize = getNumberOr(atlasJson["tileSize"], 0);
			atlas.tileCount = getNumberOr(atlasJson["tileCount"], 1);
			atlas.borderSize = getNumberOr(atlasJson["borderSize"], 0);
		}
		else
		{
			throw RBX::runtime_error("Failed to find atlas definition for platform %s", platform.c_str());
		}

        const Value& materialsJson = root["materials"];

		for (Value::ConstValueIterator it = materialsJson.Begin(); it != materialsJson.End(); ++it)
        {
            const Value& m = *it;

            Material desc;
            
			desc.name = m["name"].GetString();
            
			if (m["texture_top"].IsObject() && m["texture_side"].IsObject())
            {
                desc.topLayer = parseMaterialLayer(layers, m["texture_top"]);
                desc.sideLayer = parseMaterialLayer(layers, m["texture_side"]);

				if (m["texture_bottom"].IsObject())
				{
					desc.bottomLayer = parseMaterialLayer(layers, m["texture_bottom"]);
				}
				else
				{
					desc.bottomLayer = desc.topLayer;
				}
            }
			else if (m["texture"].IsObject())
            {
                desc.topLayer = desc.sideLayer = desc.bottomLayer = parseMaterialLayer(layers, m["texture"]);
            }
            else
            {
				desc.topLayer = desc.sideLayer = desc.bottomLayer = -1;
            }

            desc.type = parseType(getStringOr(m["type"], ""));
            desc.mapping = parseMapping(getStringOr(m["mapping"], ""));
            desc.deformation = Deformation_None;
            desc.parameter = 0.f;
            
            parseDeformation(desc, Deformation_Shift, m["shift"]);
            parseDeformation(desc, Deformation_Cubify, m["cubify"]);
            parseDeformation(desc, Deformation_Quantize, m["quantize"]);
            parseDeformation(desc, Deformation_Barrel, m["barrel"]);
            parseDeformation(desc, Deformation_Water, m["water"]);

            materials.push_back(desc);
        }
    }

} }
