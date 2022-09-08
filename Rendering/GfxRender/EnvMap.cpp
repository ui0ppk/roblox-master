

#include "stdafx.h"
#include "EnvMap.h"

#include "VisualEngine.h"
#include "GfxCore/Device.h"
#include "GfxCore/Texture.h"
#include "GfxCore/FrameBuffer.h"
#include "GlobalShaderData.h"
#include "SceneManager.h"
#include "ShaderManager.h"
#include "Sky.h"
#include "TextureManager.h"

#include "GfxCore/pix.h"

static const float MATH_PI = 3.14159265359f;

static const int kCubemapSize = 256;

namespace RBX { namespace Graphics {

EnvMap::EnvMap(VisualEngine* ve): Resource(ve->getDevice())
{
    dirtyState = VeryDirty;
    updateStep = 0;
    visualEngine = ve;
    Device* dev = ve->getDevice();
    envmapLastRealTime = 0;
    envmapLastTimeOfDay = 0;
    setDebugName("Global envmap");

    texture = dev->createTexture(Texture::Type_Cube, Texture::Format_RGBA8, kCubemapSize, kCubemapSize, 1, Texture::getMaxMipCount(kCubemapSize,kCubemapSize,1), Texture::Usage_Renderbuffer);
    for (int j=0; j<6; ++j)
    {
        shared_ptr<Renderbuffer> rb = texture.getTexture()->getRenderbuffer(j, 0);
        fbFaces[j] = dev->createFramebuffer(rb); // no depth
    }

}

EnvMap::~EnvMap()
{

}

static Matrix4 lookat( Vector3 dir, Vector3 up, float rh = 1.0f )
{
    Matrix4 result = Matrix4::identity();
    Vector3 rt = cross(up,rh*dir);
    result.setRow( 0, Vector4(rt,0) );
    result.setRow( 1, Vector4(up,0) );
    result.setRow( 2, Vector4(dir,0) );
    return result;
}


static const double kEnvmapGameTimePeriod = 120;
static const double kEnvmapRealtimePeriod = 30;


void EnvMap::update(DeviceContext* context, double gameTime)
{
    // prepare the envmap
    if (texture.getTexture())
    {
        double realTime = RBX::Time::nowFastSec();
        if( visualEngine->getSettings()->getEagerBulkExecution() )
        {
            markDirty(true);
        }
        else if (fabs(envmapLastTimeOfDay - gameTime ) > kEnvmapGameTimePeriod || fabs(envmapLastRealTime - realTime) > kEnvmapRealtimePeriod) // extra refresh at midnight - who cares
        {
            envmapLastTimeOfDay = gameTime;
            envmapLastRealTime = realTime;
            markDirty();
        }

        switch (dirtyState)
        {
        case VeryDirty:
            fullUpdate(context);
            break;
        case DirtyWait:
            if (visualEngine->getSceneManager()->getSky()->isReady())
            {
                fullUpdate(context);
            }
            break;
        case Dirty:
        case BusyDirty:
            incrementalUpdate(context);
            break;
        default:
            break;
        }
    }
}

void EnvMap::markDirty(bool veryDirty)
{
    if( veryDirty )
    {
        dirtyState = DirtyWait;
    }
    else switch (dirtyState)
    {
    case VeryDirty:  case DirtyWait: break;
    case Dirty: dirtyState = BusyDirty; break;
    case BusyDirty: break;
    default:    dirtyState = Dirty; break;
    }
}

void EnvMap::clearDirty()
{
    switch (dirtyState)
    {
    case Dirty: 
        dirtyState = Ready; 
        break;
    case BusyDirty:
        dirtyState = Dirty;
        break;
    case VeryDirty:
    case DirtyWait:
        if (visualEngine->getSceneManager()->getSky()->isReady())
            dirtyState = Ready;
        else
            dirtyState = DirtyWait;
        break;
    default:
        break;
    }
}

void EnvMap::incrementalUpdate(DeviceContext* context)
{
    if (!texture.getTexture())
        return;

    if (dirtyState == Ready) 
        return;

    if (updateStep>=7)
    {
        updateStep = 0;
        clearDirty();
        return;
    }

    if (updateStep<6)
       renderFace(context,updateStep);
    else if (updateStep == 6)
       texture.getTexture()->generateMipmaps();

    updateStep++;
}

// full re-render in a single frame
void EnvMap::fullUpdate(DeviceContext* context)
{
    /*
    while( !visualEngine->getSceneManager()->getSky()->isReady() )
    {
        visualEngine->getSceneManager()->getSky()->setSkyBoxDefault();
        visualEngine->getSceneManager()->getSky()->prerender(context);
        Sleep(100);
    }
    */

    if (!texture.getTexture())
        return;

    for (int j=0; j<6; ++j)
        renderFace(context,j);

    texture.getTexture()->generateMipmaps();

    updateStep = 0;
    clearDirty();
}

void EnvMap::renderFace(DeviceContext* context, int face)
{
    PIX_SCOPE(context, "EnvMap/update", 0);

    SceneManager* sman = visualEngine->getSceneManager();

    static int isOgl = 6*visualEngine->getDevice()->getCaps().requiresRenderTargetFlipping;

    // THIS IS A HACK!
    static Matrix4 view[6*2] = { 
        lookat(Vector3(-1,0,0),  Vector3(0,1,0),  -1),  
        lookat(Vector3(1,0,0),   Vector3(0,1,0),  -1),  
        lookat(Vector3(0,-1,0),  Vector3(0,0,-1), -1),  
        lookat(Vector3(0,1,0),   Vector3(0,0,1),  -1),
        lookat(Vector3(0,0,-1),  Vector3(0,1,0),  -1),  
        lookat(Vector3(0,0,1),   Vector3(0,1,0),  -1),  

        lookat(Vector3(-1,0,0),  Vector3(0,-1,0)),
        lookat(Vector3(1,0,0),   Vector3(0,-1,0)),  
        lookat(Vector3(0,-1,0),  Vector3(0,0,1)),  
        lookat(Vector3(0,1,0),   Vector3(0,0,-1)),
        lookat(Vector3(0,0,-1),  Vector3(0,-1,0)),  
        lookat(Vector3(0,0,1),   Vector3(0,-1,0)),  

    };

    if (1)
    {
        PIX_SCOPE(context, "EnvMap face %d", face);

        RenderCamera cam;
        cam.setViewMatrix(view[isOgl + face]);
        cam.setProjectionPerspective(MATH_PI/2, 1, 0.5, 3000);
        GlobalShaderData globsd = sman->readGlobalShaderData();
        globsd.setCamera(cam);
        context->updateGlobalConstants(&globsd,sizeof(globsd));
        context->bindFramebuffer(fbFaces[face].get());

        visualEngine->getSceneManager()->getSky()->render(context, cam, false);
        // add more stuff to render to envmap 
    }
}

void EnvMap::onDeviceRestored()
{
    markDirty(true);
}


}}
