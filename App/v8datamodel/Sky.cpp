#include "stdafx.h"

#include "./v8datamodel/Sky.h"

#include "g3d/gimage.h"
#include "util/standardout.h"
#include "Util/RobloxGoogleAnalytics.h"
#include "v8datamodel/contentprovider.h"

const char* const RBX::sSky = "Sky";

using namespace RBX;

Reflection::PropDescriptor<Sky, TextureId> Sky::prop_SkyUp("SkyboxUp", category_Appearance, &Sky::getSkyboxUp, &Sky::setSkyboxUp);
Reflection::PropDescriptor<Sky, TextureId> Sky::prop_SkyLf("SkyboxLf", category_Appearance, &Sky::getSkyboxLf, &Sky::setSkyboxLf);
Reflection::PropDescriptor<Sky, TextureId> Sky::prop_SkyRt("SkyboxRt", category_Appearance, &Sky::getSkyboxRt, &Sky::setSkyboxRt);
Reflection::PropDescriptor<Sky, TextureId> Sky::prop_SkyBk("SkyboxBk", category_Appearance, &Sky::getSkyboxBk, &Sky::setSkyboxBk);
Reflection::PropDescriptor<Sky, TextureId> Sky::prop_SkyFt("SkyboxFt", category_Appearance, &Sky::getSkyboxFt, &Sky::setSkyboxFt);
Reflection::PropDescriptor<Sky, TextureId> Sky::prop_SkyDn("SkyboxDn", category_Appearance, &Sky::getSkyboxDn, &Sky::setSkyboxDn);

Reflection::PropDescriptor<Sky, int> Sky::prop_StarCount("StarCount", category_Appearance, &Sky::getNumStars, &Sky::setNumStars);
Reflection::BoundProp<bool> Sky::prop_CelestialBodiesShown("CelestialBodiesShown", category_Appearance, &Sky::drawCelestialBodies);

Reflection::PropDescriptor<Sky, TextureId> Sky::prop_SunTexture("SunTextureId", category_Appearance, &Sky::getSunTexture, &Sky::setSunTexture);
Reflection::PropDescriptor<Sky, TextureId> Sky::prop_MoonTexture("MoonTextureId", category_Appearance, &Sky::getMoonTexture, &Sky::setMoonTexture);

Reflection::PropDescriptor<Sky, float> Sky::prop_SunAngularSize("SunAngularSize", category_Appearance, &Sky::getSunAngularSize, &Sky::setSunAngularSize);
Reflection::PropDescriptor<Sky, float> Sky::prop_MoonAngularSize("MoonAngularSize", category_Appearance, &Sky::getMoonAngularSize, &Sky::setMoonAngularSize);

static void sendSkyBoxStats(const TextureId& texId)
{
    std::string idStr = texId.getAssetId();
    if (!idStr.empty())
        RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "SkyBox", texId.getAssetId().c_str());
}

Sky::Sky()
:drawCelestialBodies(true)
,numStars(3000)
{
	setName("Sky");

    skyUp = ContentId::fromAssets("textures/sky/sky512_up.tex");	
    skyLf = ContentId::fromAssets("textures/sky/sky512_lf.tex");	
    skyRt = ContentId::fromAssets("textures/sky/sky512_rt.tex");	
    skyBk = ContentId::fromAssets("textures/sky/sky512_bk.tex");	
    skyFt = ContentId::fromAssets("textures/sky/sky512_ft.tex");	
    skyDn = ContentId::fromAssets("textures/sky/sky512_dn.tex");

    sunTexture = ContentId::fromAssets("sky/sun.jpg");
    moonTexture = ContentId::fromAssets("sky/moon.jpg");

    sunAngularSize = 21;
    moonAngularSize = 11;
}

void Sky::setNumStars(int value)
{
	if (value != numStars)
	{
		numStars = value;
		raisePropertyChanged(prop_StarCount);
	}
}

void Sky::setSkyboxBk(const TextureId& texId)
{
    if (texId != skyBk)
    {
        skyBk = texId;
        raisePropertyChanged(prop_SkyBk);
    }
}

void Sky::setSkyboxDn(const TextureId& texId)
{
    if (texId != skyDn)
    {
        skyDn = texId;
        raisePropertyChanged(prop_SkyDn);
    }
}

void Sky::setSkyboxLf(const TextureId& texId)
{
    if (texId != skyLf)
    {
        skyLf = texId;
        raisePropertyChanged(prop_SkyLf);
    }
}

void Sky::setSkyboxRt(const TextureId& texId)
{
    if (texId != skyRt)
    {
        skyRt = texId;
        raisePropertyChanged(prop_SkyRt);
    }
}
void Sky::setSkyboxUp(const TextureId& texId)
{
    if (texId != skyUp)
    {
        skyUp = texId;
        raisePropertyChanged(prop_SkyUp);
    }
}

void Sky::setSkyboxFt(const TextureId&  texId)
{
    if (texId != skyFt)
    {
        skyFt = texId;    
        raisePropertyChanged(prop_SkyFt);

        static boost::once_flag flag = BOOST_ONCE_INIT;
        boost::call_once(flag, &sendSkyBoxStats, texId);
    }
}

void Sky::setSunTexture(const TextureId& texId)
{
    if (texId != sunTexture)
    {
        sunTexture = texId;
        raisePropertyChanged(prop_SunTexture);
    }
}

void Sky::setMoonTexture(const TextureId& texId)
{
    if (texId != moonTexture)
    {
        moonTexture = texId;
        raisePropertyChanged(prop_MoonTexture);
    }
}

void Sky::setSunAngularSize(float angularSize)
{
    if (angularSize != sunAngularSize)
    {
        sunAngularSize = angularSize;
        raisePropertyChanged(prop_SunAngularSize);
    }
}

void Sky::setMoonAngularSize(float angularSize)
{
    if (angularSize != moonAngularSize)
    {
        moonAngularSize = angularSize;
        raisePropertyChanged(prop_MoonAngularSize);
    }
}
