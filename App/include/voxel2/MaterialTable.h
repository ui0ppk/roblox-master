#pragma once

namespace RBX { namespace Voxel2 {

    class MaterialTable
    {
    public:
        enum Type
        {
            Type_Soft,
            Type_Hard,
            Type_HardSoft,
        };

        enum Deformation
        {
            Deformation_None,
            Deformation_Shift,
            Deformation_Cubify,
            Deformation_Quantize,
            Deformation_Barrel,
            Deformation_Water,
        };

        enum Mapping
        {
            Mapping_Default,
            Mapping_Cube,
        };

        struct Material
        {
            std::string name;

            int topLayer;
            int sideLayer;
			int bottomLayer;

            Type type;
            Mapping mapping;
            
            Deformation deformation;
            float parameter;
        };
        
        struct Layer
        {
            float tiling;
            float detiling;
        };

		struct Atlas
		{
			int width;
			int height;
			int tileSize;
			int tileCount;
			int borderSize;
		};

        MaterialTable(const std::string& file, unsigned int materialCount);
        ~MaterialTable();
        
        const Material& getMaterial(unsigned int index) const { return materials[index]; }
        unsigned int getMaterialCount() const { return materials.size(); }

		const Layer& getLayer(unsigned int index) const { return layers[index]; }
		unsigned int getLayerCount() const { return layers.size(); }

		const Atlas& getAtlas() const { return atlas; }

    private:
		Atlas atlas;
        std::vector<Material> materials;
        std::vector<Layer> layers;

        void load(const std::string& file);
    };

} }
