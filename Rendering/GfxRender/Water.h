#pragma once

#include "TextureRef.h"

namespace RBX
{
	class DataModel;
}

namespace RBX
{
namespace Graphics
{

class VisualEngine;
class Material;

class Water
{
public:
    Water(VisualEngine* visualEngine);
    ~Water();

    void update(DataModel* dataModel, float dt);

    const shared_ptr<Material>& getLegacyMaterial();
    const shared_ptr<Material>& getSmoothMaterial();

private:
	enum { kAnimFrames = 24 };

    VisualEngine* visualEngine;

    shared_ptr<Material> legacyMaterial;
    shared_ptr<Material> smoothMaterial;

    TextureRef normalMaps[kAnimFrames + 1];
    
    double currentTimeScaled;

    const TextureRef& getNormalMap(unsigned int frame);
};

}
}
