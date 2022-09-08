#pragma once 

#include "GfxBase/FrameRateManager.h"
#include "TextureRef.h"
#include "GfxCore/Resource.h"
#include "ScreenSpaceEffect.h"

namespace RBX
{
namespace Graphics
{

class Framebuffer;
class VisualEngine;
class DeviceContext;

class SSAO: public Resource
{
public:
    SSAO(VisualEngine* visualEngine);
    ~SSAO();
    
	bool isActive() const { return !!data.get(); }

    void update(SSAOLevel level, unsigned int width, unsigned int height);
    
    void renderCompute(DeviceContext* context);
    void renderApply(DeviceContext* context);
    void renderComposit(DeviceContext* context);

    virtual void onDeviceLost();
   
private:
    struct Data;

    VisualEngine* visualEngine;

    shared_ptr<Texture> noise;

    bool forceDisabled;
    SSAOLevel currentLevel;
    scoped_ptr<Data> data;

    Data* createData(unsigned int width, unsigned int height);
//    void renderFullscreen(DeviceContext* context, const char* vsName, const char* fsName);
};

}
}
