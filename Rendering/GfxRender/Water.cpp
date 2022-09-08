#include "stdafx.h"
#include "Water.h"

#include "VisualEngine.h"
#include "SceneManager.h"
#include "ShaderManager.h"
#include "TextureManager.h"
#include "LightGrid.h"
#include "Material.h"
#include "GfxCore/Device.h"
#include "EnvMap.h"
#include "SSAO.h"

#include "V8DataModel/DataModel.h"
#include "V8DataModel/Workspace.h"
#include "v8datamodel/MegaCluster.h"

#include "Voxel2/Grid.h"

namespace RBX
{
namespace Graphics
{

static const double kAnimLength = 2.f;

#ifdef RBX_PLATFORM_IOS
static const char* kTextureExt = "pvr";
#elif defined(__ANDROID__)
static const char* kTextureExt = "pvr";
#else
static const char* kTextureExt = "dds";
#endif

static bool isCameraUnderwaterLegacy(MegaClusterInstance* terrain)
{
    Camera* cam = DataModel::get(terrain)->getWorkspace()->getCamera();
    if (!cam)
        return false;

    Vector3 cameraPos = cam->getCameraCoordinateFrame().translation;
    Vector3 cameraDir = (cam->getCameraFocus().translation - cameraPos).unit();

    // this makes popping a bit less evident at steep diving angles
    cameraPos.y += 0.8f * cameraDir.y + 0.15f; // - kWaveHeight b/c the shader lowers the water level a little bit

    Vector3int16 cellPos = Voxel::worldToCell_floor(cameraPos);

    return terrain->getVoxelGrid()->getCell(cellPos).isExplicitWaterCell();
}

static float getWaterDisplacement(const Vector3& position, float waveFrequency, float wavePhase, float waveHeight)
{
    // Has to match displacePosition from smoothwater.hlsl
    float x = sin((position.z - position.x) * waveFrequency - wavePhase);
    float z = sin((position.z + position.x) * waveFrequency + wavePhase);
    float p = (x + z) * waveHeight;
    
    return p;
}

static bool isCameraUnderwater(MegaClusterInstance* terrain)
{
    Camera* cam = DataModel::get(terrain)->getWorkspace()->getCamera();
    if (!cam)
        return false;

    Vector3 cameraPos = cam->getCameraCoordinateFrame().translation;
    Vector3 cameraDir = (cam->getCameraFocus().translation - cameraPos).unit();

    // this makes popping a bit less evident at steep diving angles
    cameraPos.y += 0.8f * cameraDir.y + 0.15f; // - kWaveHeight b/c the shader lowers the water level a little bit

    Vector3int16 cellPos = Voxel::worldToCell_floor(cameraPos);

    return terrain->getSmoothGrid()->getCell(cellPos.x, cellPos.y, cellPos.z).getMaterial() == Voxel2::Cell::Material_Water;
}

static bool isCameraUnderwater(MegaClusterInstance* terrain, float waveFrequency, float wavePhase, float waveHeight)
{
    Camera* cam = DataModel::get(terrain)->getWorkspace()->getCamera();
    if (!cam)
        return false;

    Vector3 cameraPos = cam->getCameraCoordinateFrame().translation;

    // counter-displace camera to match vertex deformation
    cameraPos.y -= getWaterDisplacement(cameraPos, waveFrequency, wavePhase, waveHeight);
 
    Vector3 cameraCellPos = Voxel::worldSpaceToCellSpace(cameraPos);
    
    for (int x = -1; x <= 1; ++x)
        for (int z = -1; z <= 1; ++z)
        {
            // look around +-0.5 voxels to compensate for water filling neighboring cells
            int cellPosX = floorf(cameraCellPos.x + x * 0.5f);
            int cellPosY = floorf(cameraCellPos.y);
            int cellPosZ = floorf(cameraCellPos.z + z * 0.5f);

            Voxel2::Cell cameraCell0 = terrain->getSmoothGrid()->getCell(cellPosX, cellPosY + 0, cellPosZ);
            Voxel2::Cell cameraCell1 = terrain->getSmoothGrid()->getCell(cellPosX, cellPosY + 1, cellPosZ);
            
            if (cameraCell0.getMaterial() == Voxel2::Cell::Material_Water)
            {
                // camera is in water cell and cell above is not air => we're either completely in water or at the water-solid boundary
                // so we can safely assume underwater
                if (cameraCell1.getMaterial() != Voxel2::Cell::Material_Air)
                    return true;
                
                // cell above is air => have to estimate the plane based on occupancy
                float occScale = 1.f / (Voxel2::Cell::Occupancy_Max + 1);
                float waterHeightCell = (cameraCell0.getOccupancy() + 1) * occScale;
                
                if (cameraCellPos.y < cellPosY + waterHeightCell)
                    return true;
            }
        }
    
    return false;
}

Water::Water(VisualEngine* visualEngine)
	: visualEngine(visualEngine)
	, currentTimeScaled(0)
{
}

Water::~Water()
{
}

void Water::update(DataModel* dataModel, float dt)
{
    MegaClusterInstance* terrain = static_cast<MegaClusterInstance*>(dataModel->getWorkspace()->getTerrain());

	if (!terrain)
		return;

    if (terrain->isSmooth())
    {
		Color3 waterColor = terrain->getWaterColor();
		float waterDensity = 1 - terrain->getWaterTransparency();
		float waterDepth = G3D::lerp(10.f, 200.f, terrain->getWaterTransparency());
		
		float waveLength = 85 * sqrtf(terrain->getWaterWaveSize());
		float waveSpeed = terrain->getWaterWaveSpeed();
		float waveHeight = terrain->getWaterWaveSize();
		
		// update stored time so that we can change wave speed without changing state
		currentTimeScaled += dt * waveSpeed;
		
		// this is a pre-multiplied frequency (as in, 2pif)
		float waveFrequency = 2 * Math::pif() / std::max(waveLength, 1e-3f);
		float wavePhase = fmodf(currentTimeScaled * waveFrequency, 2 * Math::pif());
		
		double frame = fmod(currentTimeScaled / 10.f, kAnimLength) / kAnimLength * kAnimFrames;
		double ratio = frame - floor(frame);
		
		unsigned int frame1 = static_cast<unsigned int>(frame) % ARRAYSIZE(normalMaps);
		unsigned int frame2 = (frame1 + 1) % ARRAYSIZE(normalMaps);
		
		bool disableWaterFog = visualEngine->getFrameRateManager()->GetQualityLevel() == 0;
		
		if (isCameraUnderwater(terrain, waveFrequency, wavePhase, waveHeight) && !disableWaterFog)
		{
			// At depth=0 fog factor = -fogStart/(fogEnd - fogStart) = waterDensity
			float fogStart = -waterDepth * waterDensity / (1 - waterDensity);
			float fogEnd = waterDepth;
			
			SceneManager* sceneManager = visualEngine->getSceneManager();
			
			sceneManager->setSkyEnabled(false);
			sceneManager->setClearColor(Color4(waterColor));
			sceneManager->setFog(waterColor, fogStart, fogEnd);
		}
		
		if (smoothMaterial)
		{
			std::vector<Technique>& techniques = smoothMaterial->getTechniques();
			
			for (size_t i = 0; i < techniques.size(); ++i)
			{
				Technique& t = techniques[i];
				
				t.setTexture(0, getNormalMap(frame1), SamplerState::Filter_Linear);
				t.setTexture(1, getNormalMap(frame2), SamplerState::Filter_Linear);
				
				t.setConstant("WaveParams", Vector4(waveFrequency, wavePhase, waveHeight, ratio));
				t.setConstant("WaterColor", Vector4(Color4(waterColor)));
				t.setConstant("WaterParams", Vector4((1.f - waterDensity) / waterDepth, waterDensity, 0, 0));
			}
		}
	}
    else
    {
        static const Color3 kWaterColor = Color3(13, 85, 93) / 255;

        if (isCameraUnderwaterLegacy(terrain))
        {
            SceneManager* sceneManager = visualEngine->getSceneManager();

            sceneManager->setSkyEnabled(false);
            sceneManager->setClearColor(Color4(kWaterColor));
            sceneManager->setFog(kWaterColor, -50, 60);
        }

    	double currentTime = Time::nowFastSec();
    	double frame = fmod(currentTime, kAnimLength) / kAnimLength * kAnimFrames;

        unsigned int frame1 = static_cast<unsigned int>(frame) % ARRAYSIZE(normalMaps);
        unsigned int frame2 = (frame1 + 1) % ARRAYSIZE(normalMaps);

        // waves
        float freq  = 0.2f;
        float speed = 1.1f;
        float height = 0.15f;
        float phase = fmodf(currentTime * speed * 9, 2*3.1415926f / freq);
    	double ratio = frame - floor(frame);

        if (legacyMaterial)
    	{
            std::vector<Technique>& techniques = legacyMaterial->getTechniques();

            for (size_t i = 0; i < techniques.size(); ++i)
            {
                Technique& t = techniques[i];

                t.setTexture(0, getNormalMap(frame1), SamplerState::Filter_Linear);
                t.setTexture(1, getNormalMap(frame2), SamplerState::Filter_Linear);

                t.setConstant("waveParams", Vector4(freq, phase, height, 0));
                t.setConstant("nmAnimLerp", Vector4(ratio, 0, 0, 0));
                t.setConstant("WaterColor", Vector4(Color4(kWaterColor)));
            }
        }
    }
}

static void setupTechnique(Technique& technique, VisualEngine* visualEngine, const TextureRef& nm0, const TextureRef& nm1)
{
    LightGrid* lightGrid = visualEngine->getLightGrid();

	technique.setRasterizerState(RasterizerState(RasterizerState::Cull_None, 4));

    technique.setTexture(0, nm0, SamplerState::Filter_Linear);
    technique.setTexture(1, nm1, SamplerState::Filter_Linear);
    technique.setTexture(2, visualEngine->getSceneManager()->getEnvMap()->getTexture(), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp) );

    if (lightGrid)
    {
        technique.setTexture(3, lightGrid->getTexture(), SamplerState::Filter_Linear);
        technique.setTexture(4, lightGrid->getLookupTexture(), SamplerState::Filter_Point);
    }
}

const shared_ptr<Material>& Water::getLegacyMaterial()
{
	if (legacyMaterial)
        return legacyMaterial;

    legacyMaterial = shared_ptr<Material>(new Material());

    const char* vsNameHQ = "WaterHQVS";
    const char* psNameHQ = "WaterHQFS";
    const char* vsNameLQ = "WaterVS";
    const char* psNameLQ = "WaterFS";
        
   
    if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram(vsNameHQ, psNameHQ))
    {
        Technique technique(program, 1);

        setupTechnique(technique, visualEngine, getNormalMap(0), getNormalMap(1));

        legacyMaterial->addTechnique(technique);
    }

    if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram(vsNameLQ, psNameLQ))
    {
        Technique technique(program, 2);

        setupTechnique(technique, visualEngine, getNormalMap(0), getNormalMap(1));

        legacyMaterial->addTechnique(technique);
    }

    if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgramFFP())
    {
        Technique technique(program, 3);

        TextureManager* tm = visualEngine->getTextureManager();
        technique.setTexture(0, tm->load(ContentId("rbxasset://textures/water_Subsurface.dds"), TextureManager::Fallback_White), SamplerState::Filter_Linear);

        legacyMaterial->addTechnique(technique);
    }
   
    return legacyMaterial;
}

const shared_ptr<Material>& Water::getSmoothMaterial()
{
    if (smoothMaterial)
        return smoothMaterial;

    smoothMaterial = shared_ptr<Material>(new Material());

    if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram("SmoothWaterHQVS", "SmoothWaterHQGBufferFS"))
    {
        Technique technique(program, 0);

        setupTechnique(technique, visualEngine, getNormalMap(0), getNormalMap(1));

		technique.setTexture(5, visualEngine->getSceneManager()->getGBufferColor(), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));
		technique.setTexture(6, visualEngine->getSceneManager()->getGBufferDepth(), SamplerState(SamplerState::Filter_Point, SamplerState::Address_Clamp));

        smoothMaterial->addTechnique(technique);
    }

    if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram("SmoothWaterHQVS", "SmoothWaterHQFS"))
    {
        Technique technique(program, 1);

        setupTechnique(technique, visualEngine, getNormalMap(0), getNormalMap(1));

        smoothMaterial->addTechnique(technique);
    }

    if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram("SmoothWaterVS", "SmoothWaterFS"))
    {
        Technique technique(program, 2);

        setupTechnique(technique, visualEngine, getNormalMap(0), getNormalMap(1));

        smoothMaterial->addTechnique(technique);
    }

    if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgramFFP())
    {
        Technique technique(program, 3);

        TextureManager* tm = visualEngine->getTextureManager();
        technique.setTexture(0, tm->load(ContentId("rbxasset://textures/water_Subsurface.dds"), TextureManager::Fallback_White), SamplerState::Filter_Linear);

        smoothMaterial->addTechnique(technique);
    }

    return smoothMaterial;
}

const TextureRef& Water::getNormalMap(unsigned int frame)
{
    RBXASSERT(frame < ARRAYSIZE(normalMaps));

	if (!normalMaps[frame].getTexture())
	{
        char id[256];
        sprintf( id, "rbxasset://textures/water/normal_%02d.%s", frame+1, kTextureExt );
		normalMaps[frame] = visualEngine->getTextureManager()->load(ContentId(id), TextureManager::Fallback_NormalMap);
	}

    return normalMaps[frame];
}

}
}
