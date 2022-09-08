#include "stdafx.h"
#include "Sky.h"

#include "VisualEngine.h"
#include "TextureManager.h"
#include "ShaderManager.h"
#include "SceneManager.h"
#include "Util.h"
#include "EnvMap.h"

#include "GfxCore/Geometry.h"
#include "GfxCore/Device.h"
#include "GfxCore/States.h"

#include "v8datamodel/Sky.h"

#include "g3d/LightingParameters.h"
#include "GfxCore/pix.h"

#include "rbx/Profiler.h"

FASTFLAG(DebugRenderDownloadAssets)
FASTFLAGVARIABLE(RenderMoonBillboard, true)

namespace RBX
{
namespace Graphics
{

static const float kSkySize = 2700;
static const float kStarDistance = 2500;
static const int kStarTwinkleRate = 40;
static const float kStarTwinkleRatio = 0.2f;

static TextureRef::Status getCombinedStatus(const TextureRef (&textures)[6])
{
    for (int i = 0; i < 6; ++i)
	{
        TextureRef::Status status = textures[i].getStatus();

		if (status != TextureRef::Status_Loaded)
            return status;
	}

	return TextureRef::Status_Loaded;
}

static Matrix4 getBillboardTransform(const RenderCamera& camera, const Vector3& position, float size)
{
	if (FFlag::RenderMoonBillboard)
	{
		Matrix3 rotation = Math::getWellFormedRotForZVector(normalize(position - camera.getPosition()));

		return Matrix4(rotation * Matrix3::fromDiagonal(Vector3(size)), position);
	}
	else
	{
		Matrix4 viewInverse = camera.getViewMatrix().inverse();

		return Matrix4(viewInverse.upper3x3() * Matrix3::fromDiagonal(Vector3(size)), position);
	}
}

struct SkyVertex
{
    Vector3 position;
    unsigned int color;
    Vector2 texcoord;
};

static GeometryBatch* createQuad(Device* device, const shared_ptr<VertexLayout>& layout)
{
    SkyVertex vertices[] =
	{
		{ Vector3(-1, -1, 0), 0xffffffff, Vector2(0, 1) },
		{ Vector3(-1, +1, 0), 0xffffffff, Vector2(0, 0) },
		{ Vector3(+1, -1, 0), 0xffffffff, Vector2(1, 1) },
		{ Vector3(+1, +1, 0), 0xffffffff, Vector2(1, 0) },
	};

	shared_ptr<VertexBuffer> vb = device->createVertexBuffer(sizeof(SkyVertex), ARRAYSIZE(vertices), GeometryBuffer::Usage_Static);

    vb->upload(0, vertices, sizeof(vertices));

	shared_ptr<Geometry> geometry = device->createGeometry(layout, vb, shared_ptr<IndexBuffer>());

	return new GeometryBatch(geometry, Geometry::Primitive_TriangleStrip, 4, 4);
}

void Sky::StarData::resize(Sky* sky, unsigned int count, bool dynamic)
{
    if (count == 0)
        reset();
	else
	{
		stars.resize(count);

		buffer = sky->visualEngine->getDevice()->createVertexBuffer(sizeof(SkyVertex), count, dynamic ? GeometryBuffer::Usage_Dynamic : GeometryBuffer::Usage_Static);

		batch.reset(new GeometryBatch(sky->visualEngine->getDevice()->createGeometry(sky->layout, buffer, shared_ptr<IndexBuffer>()), Geometry::Primitive_Points, count, count));
	}
}

void Sky::StarData::reset()
{
    stars.clear();
    batch.reset();
    buffer.reset();
}

Sky::Sky(VisualEngine* visualEngine)
	: visualEngine(visualEngine)
	, starLight(Brightness_None)
	, starTwinkleCounter(0)
    , readyState(false)
{
	std::vector<VertexLayout::Element> elements;
	elements.push_back(VertexLayout::Element(0, offsetof(SkyVertex, position), VertexLayout::Format_Float3, VertexLayout::Semantic_Position));
	elements.push_back(VertexLayout::Element(0, offsetof(SkyVertex, color), VertexLayout::Format_Color, VertexLayout::Semantic_Color));
	elements.push_back(VertexLayout::Element(0, offsetof(SkyVertex, texcoord), VertexLayout::Format_Float2, VertexLayout::Semantic_Texture));

	layout = visualEngine->getDevice()->createVertexLayout(elements);

	quad.reset(createQuad(visualEngine->getDevice(), layout));

    // preload default skybox so that we can show it even while fetching custom skies over HTTP
    if (!FFlag::DebugRenderDownloadAssets)
        loadSkyBoxDefault(skyBox);

    // preload sun/mon
	sun = visualEngine->getTextureManager()->load(ContentId("rbxasset://sky/sun.jpg"), TextureManager::Fallback_BlackTransparent);
	moon = visualEngine->getTextureManager()->load(ContentId("rbxasset://sky/moon.jpg"), TextureManager::Fallback_BlackTransparent);
}

Sky::~Sky()
{
}

void Sky::update(const G3D::LightingParameters& lighting, int starCount, bool drawCelestialBodies, float sunSize, float moonSize)
{
	skyColor = lighting.skyAmbient;
    skyColor2 = lighting.skyAmbient2;

	drawSunMoon = drawCelestialBodies;

	sunAngularSize = sunSize * 23.8095238095;   // kSunSize was 500, angular size 21 equates to it, so 500/21=23.8095238095
	moonAngularSize = moonSize * 23.8095238095; // however kMoonSize was 250, angular size 11 equates to it, and 250/11=22.7272727273
												// need to get exact size or get exact angle soon.

	sunPosition = (lighting.physicallyCorrect ? lighting.trueSunPosition : lighting.sunPosition);
	sunColor = Color3(lighting.emissiveScale * .8f * G3D::clamp((sunPosition.y + 0.1f) * 10.0f, 0.f, 1.f));

	moonPosition = (lighting.physicallyCorrect ? lighting.trueMoonPosition : lighting.moonPosition);
    moonColor = Color3(G3D::min((1.0f-lighting.skyAmbient[1])+0.1f, 1.0f));

	if (!drawCelestialBodies || starCount == 0)
	{
		starsNormal.reset();
		starsTwinkle.reset();
	}
	else
	{
        Brightness oldStarLight = starLight;

        if (lighting.moonPosition.y <= -0.3)
            starLight = Brightness_None;
        else if (lighting.moonPosition.y <= -0.1)
			starLight = Brightness_Dim;
        else if(lighting.moonPosition.y <= 0.2)
            starLight = Brightness_Dimmer;
        else
			starLight = Brightness_Bright;

		if (starsNormal.stars.size() + starsTwinkle.stars.size() != starCount)
		{
			createStarField(starCount);

			updateStarsNormal();
			updateStarsTwinkle();
		}
		else if (oldStarLight != starLight)
		{
			updateStarsNormal();
			updateStarsTwinkle();
		}
	}
}

void Sky::prerender()
{
    TextureRef::Status loadingStatus = getCombinedStatus(skyBoxLoading);

    if (loadingStatus == TextureRef::Status_Loaded)
    {
        std::copy(skyBoxLoading, skyBoxLoading + 6, skyBox);
        readyState = true;

        if (EnvMap* envMap = visualEngine->getSceneManager()->getEnvMap())
        {
            envMap->markDirty(false);
        }
    }

    if (loadingStatus != TextureRef::Status_Waiting)
        std::fill(skyBoxLoading, skyBoxLoading + 6, TextureRef());

    // Update star twinkle
    starTwinkleCounter++;
}

void Sky::render(DeviceContext* context, const RenderCamera& camera, bool drawStars)
{
    RBXPROFILER_SCOPE("Render", "Sky");
	RBXPROFILER_SCOPE("GPU", "Sky");
    PIX_SCOPE(context, "Sky");

    // Update load-in-progress
	if (starTwinkleCounter >= kStarTwinkleRate)
	{
		starTwinkleCounter = 0;

		if (starLight != Brightness_None && !starsTwinkle.stars.empty())
			updateStarsTwinkle();
	}

    shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgramOrFFP("SkyVS", "SkyFS");

    if (program)
    {
		int colorHandle = program->getConstantHandle("Color");
        int color2Handle = program->getConstantHandle("Color2");

        context->bindProgram(program.get());

        context->setRasterizerState(RasterizerState::Cull_None);
        context->setDepthState(DepthState(DepthState::Function_LessEqual, false));

        // Render sky surface, if loaded
        if (1)
        {
            context->setBlendState(BlendState::Mode_None);

            for (int i = 0; i < 6; ++i)
			{
                PIX_SCOPE(context, "Skybox face %d", i);
				float adjustAngle = (i == NORM_Y) ? G3D::pif() / 2 : (i == NORM_Y_NEG) ? -G3D::pif() / 2 : 0;
				Matrix3 rotation = Matrix3::fromAxisAngle(Vector3(0, 1, 0), adjustAngle) * Matrix3::fromDiagonal(Vector3(-1, 1, 1)) * normalIdToMatrix3(static_cast<NormalId>(i));
				Matrix4 transform(rotation * Matrix3::fromDiagonal(Vector3(kSkySize)), camera.getPosition() + rotation.column(2) * kSkySize);

				context->bindTexture(0, skyBox[i].getTexture().get() ? skyBox[i].getTexture().get() : visualEngine->getTextureManager()->getFallbackTexture(TextureManager::Fallback_White).get(), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));

				float colorData[] = {skyColor.r, skyColor.g, skyColor.b, 1};
                float color2Data[] = {skyColor2.r, skyColor2.g, skyColor2.b, 1 };
				context->setConstant(colorHandle, colorData, 1);
                context->setConstant(color2Handle, color2Data, 1);

				context->setWorldTransforms4x3(transform[0], 1);
				context->draw(*quad);
			}
		}

        // Render sun/moon
		if (drawSunMoon)
		{
            PIX_SCOPE(context, "Sun/moon");
            context->setBlendState(BlendState::Mode_Additive);

			Matrix4 sunTransform = getBillboardTransform(camera, camera.getPosition() + sunPosition * kSkySize, sunAngularSize);

            context->setConstant(colorHandle, &sunColor.r, 1);
            context->setConstant(color2Handle,&sunColor.r, 1);

            context->bindTexture(0, sun.getTexture().get(), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));

            context->setWorldTransforms4x3(sunTransform[0], 1);
            context->draw(*quad);
            context->draw(*quad);
            context->draw(*quad);

            PIX_MARKER(context, "Moon:");

			Matrix4 moonTransform = getBillboardTransform(camera, camera.getPosition() + moonPosition * kSkySize, moonAngularSize);

            context->setConstant(colorHandle, &moonColor.r, 1);

            context->bindTexture(0, moon.getTexture().get(), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));

            context->setWorldTransforms4x3(moonTransform[0], 1);
            context->draw(*quad);
		}

        // Render stars
		if (drawStars && starLight != Brightness_None && (starsNormal.batch || starsTwinkle.batch))
		{
            PIX_SCOPE(context, "Stars");
            context->setBlendState(BlendState::Mode_AlphaBlend);

            float colorData[] = {1, 1, 1, 1};
            context->setConstant(colorHandle, colorData, 1);
            context->setConstant(color2Handle, colorData, 1);

			context->bindTexture(0, visualEngine->getTextureManager()->getFallbackTexture(TextureManager::Fallback_White).get(), SamplerState::Filter_Linear);

			Matrix4 transform(Matrix3::identity(), camera.getPosition());

            context->setWorldTransforms4x3(transform[0], 1);

			if (starsNormal.batch)
				context->draw(*starsNormal.batch);

			if (starLight != Brightness_Dim && starsTwinkle.batch)
				context->draw(*starsTwinkle.batch);
		}
	}
}

void Sky::setSkyBox(const ContentId& rt, const ContentId& lf, const ContentId& bk, const ContentId& ft, const ContentId& up, const ContentId& dn)
{
	loadSkyBox(skyBoxLoading, rt, lf, bk, ft, up, dn);
}

void Sky::setSkyBoxDefault()
{
	loadSkyBoxDefault(skyBoxLoading);
}

void Sky::loadSkyBoxDefault(TextureRef* textures)
{
    RBX::Sky dummy;

	loadSkyBox(textures, dummy.skyRt, dummy.skyLf, dummy.skyBk, dummy.skyFt, dummy.skyUp, dummy.skyDn);
}

void Sky::loadSkyBox(TextureRef* textures, const ContentId& rt, const ContentId& lf, const ContentId& bk, const ContentId& ft, const ContentId& up, const ContentId& dn)
{
	TextureManager* tm = visualEngine->getTextureManager();

	textures[NORM_X] = tm->load(rt, TextureManager::Fallback_None, "Sky.SkyboxRt");
	textures[NORM_Y] = tm->load(up, TextureManager::Fallback_None, "Sky.SkyboxUp");
	textures[NORM_Z] = tm->load(bk, TextureManager::Fallback_None, "Sky.SkyboxBk");
	textures[NORM_X_NEG] = tm->load(lf, TextureManager::Fallback_None, "Sky.SkyboxLf");
	textures[NORM_Y_NEG] = tm->load(dn, TextureManager::Fallback_None, "Sky.SkyboxDn");
	textures[NORM_Z_NEG] = tm->load(ft, TextureManager::Fallback_None, "Sky.SkyboxFt");
}

void Sky::setCelestialBodies(const ContentId& sunTexture, const ContentId& moonTexture)
{
	loadCelestialBodies(celestialBodiesLoading, sunTexture, moonTexture);
}

void Sky::setCelestialBodiesDefault()
{
	loadCelestialBodiesDefault(celestialBodiesLoading);
}

void Sky::loadCelestialBodiesDefault(TextureRef* textures)
{
	RBX::Sky dummy;

	loadCelestialBodies(textures, dummy.sunTexture, dummy.moonTexture);
}

void Sky::loadCelestialBodies(TextureRef* textures, const ContentId& sunTexture, const ContentId& moonTexture)
{
	TextureManager* tm = visualEngine->getTextureManager();

	sun = tm->load(sunTexture, TextureManager::Fallback_BlackTransparent, "Sky.SunTextureId");
	moon = tm->load(moonTexture, TextureManager::Fallback_BlackTransparent, "Sky.MoonTextureId");
}

void Sky::createStarField(int starCount)
{
	int twinkle = static_cast<int>(starCount * kStarTwinkleRatio);
    int normal = starCount - twinkle;

    // create random positions and intensities for regular stars
	starsNormal.resize(this, normal, false);

	for (size_t i = 0; i < starsNormal.stars.size(); ++i)
    {
		Star& s = starsNormal.stars[i];

        Vector3 r = Vector3::random();

		s.position = r * kStarDistance;
		s.intensity = G3D::uniformRandom(0.3f, 0.8f);

        float tpos = fabs(r.y);

        if (tpos < 0.15f)
			s.intensity *= 0.6f;
        else if (tpos < 0.3f)
			s.intensity *= 0.75f;
        else if (tpos < 0.6f)
			s.intensity *= 0.9f;

		s.intensity *= s.intensity;
    }

    // create random positions and intensities for twinkling stars
	starsTwinkle.resize(this, twinkle, true);

	for (size_t i = 0; i < starsTwinkle.stars.size(); ++i)
    {
		Star& s = starsTwinkle.stars[i];

        Vector3 r = Vector3::random();

        // only top hemisphere has twinkling stars
        r.y = fabsf(r.y);

		s.position = r * kStarDistance;
		s.intensity = G3D::uniformRandom(0.2f, 0.7f);

        float tpos = fabs(r.y);

        if (tpos < 0.3f)
			s.intensity *= 0.7f;
        else if (tpos < 0.6f)
			s.intensity *= 0.9f;

		s.intensity *= s.intensity;
    }
}

void Sky::updateStarsNormal()
{
	bool colorOrderBGR = visualEngine->getDevice()->getCaps().colorOrderBGR;

	const StarData& data = starsNormal;

    if (data.stars.empty())
        return;

	SkyVertex* vertices = static_cast<SkyVertex*>(data.buffer->lock());

	for (unsigned int i = 0; i < data.stars.size(); ++i)
	{
		vertices[i].position = data.stars[i].position;

		float intensity = data.stars[i].intensity;

		if (starLight == Brightness_Bright)
		{
            intensity *= 5 * 0.3f;
        }
		else if (starLight == Brightness_Dimmer)
		{
            intensity *= 2.5f * 0.15f;
        }
        else
		{
            intensity *= 0.6f;
        }

        float tnum = G3D::uniformRandom(0.0, 1.0);

        if (tnum < 0.15)
			vertices[i].color = packColor(Color4(1.0, 0.80f, 0.70f, intensity), colorOrderBGR);	// yellow (15%)
        else if (tnum < 0.55)
            vertices[i].color = packColor(Color4(1.0, 1.0, 1.0, intensity), colorOrderBGR);   // white (40%)
        else if (tnum < 0.80)
			vertices[i].color = packColor(Color4(0.70f, 0.80f, 1.0, intensity), colorOrderBGR);	// blue (25%)
        else
			vertices[i].color = packColor(Color4(0.75f, 0.50f, 1.0, intensity), colorOrderBGR);	// purple (20%)

		vertices[i].texcoord = Vector2(0, 0);
    }

	data.buffer->unlock();
}

void Sky::updateStarsTwinkle()
{
	bool colorOrderBGR = visualEngine->getDevice()->getCaps().colorOrderBGR;

	const StarData& data = starsTwinkle;

    if (data.stars.empty())
        return;

	SkyVertex* vertices = static_cast<SkyVertex*>(data.buffer->lock(GeometryBuffer::Lock_Discard));

	for (unsigned int i = 0; i < data.stars.size(); ++i)
	{
		vertices[i].position = data.stars[i].position;

        float randomBrightness = G3D::uniformRandom(0.3f, 1.7f);
		float brightfactor = (starLight == Brightness_Dimmer) ? 0.5f : 1.f;
		float intensity = data.stars[i].intensity * randomBrightness * brightfactor + 0.2f;

        float tnum = G3D::uniformRandom(0.0, 1.0);

        if (tnum < 0.15)
			vertices[i].color = packColor(Color4(1.0, 0.80f, 0.70f, intensity), colorOrderBGR);	// yellow (15%)
        else if (tnum < 0.55)
            vertices[i].color = packColor(Color4(1.0, 1.0, 1.0, intensity), colorOrderBGR);   // white (40%)
        else if (tnum < 0.80)
			vertices[i].color = packColor(Color4(0.70f, 0.80f, 1.0, intensity), colorOrderBGR);	// blue (25%)
        else
            vertices[i].color = packColor(Color4(0.75f, 0.50f, 1.0, intensity), colorOrderBGR);	// purple (20%)

		vertices[i].texcoord = Vector2(0, 0);
    }

	data.buffer->unlock();
}

}
}
