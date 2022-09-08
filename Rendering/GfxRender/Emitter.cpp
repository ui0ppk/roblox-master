#include "stdafx.h"
#include "Emitter.h"

#include "GfxCore/Device.h"
#include "util/G3DCore.h"
#include "VisualEngine.h"
#include "ShaderManager.h"
#include "RenderQueue.h"
#include "RenderCamera.h"
#include "TextureManager.h"
#include "GfxBase/FrameRateManager.h"

#include "GfxCore/Geometry.h"
#include "Material.h"
#include "EmitterShared.h"
#include "RenderNode.h"

#include <boost/cstdint.hpp>

#include "v8datamodel/NumberSequence.h"
#include "v8datamodel/ColorSequence.h"

#undef min
#undef max

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

FASTFLAG(GlowEnabled)
FASTINTVARIABLE(RenderMaxParticleSize, 200);

DYNAMIC_FASTFLAG(EnableParticleDrag)

static int gEmitterCount = 0;

using G3D::clamp;

namespace RBX{ namespace Graphics{

typedef boost::int16_t  int16;
typedef boost::uint8_t  uint8;
typedef boost::uint32_t uint32;
typedef boost::uint16_t index_t;


static const float kThrottleDist = 200.0f;
static const float kCutoffDist = 1000.0f;
static const float kMinThrottle = 0.1f;
static const float kCutoffAlpha = 10/255.0f;
static const float kAlphaBoost  = 0.6f;
static const float kMaxLife = 20;

#if defined(RBX_PLATFORM_IOS) || defined(__ANDROID__)
static const int kMaxParticles = 14000/4;
static const float kMaxThrottle = 0.7f;
static const float kMaxEmissionRate = 100;
#else
static const int kMaxParticles = 64000/4;
static const float kMaxThrottle = 1.0f;
static const float kMaxEmissionRate = 400;
#endif


static const float kLongFrameSimStep = 0.016f; // ~60fps
static const int kMaxParticlesPerEmitter = 60000;

char dummy[ sizeof(EmitterShared().shaders) / sizeof(EmitterShared().shaders[0]) == Emitter::Shader__Count ];

struct ParticleVertex
{
    float   x,y,z;            //
    int16   scaleRotLifeq[4];    // .x = sx, .y = sy, .z = angle, .w = normalized 0..1 lifetime
    int16   disp[2];      // constant 0,0  0,1  1,0  1,1  for corner displacement from the center
    int16   cline[2];          // color line ( .y is reserved )
    uint8   color[4];   // r,g,b,a
};

static const VertexLayout::Element kVertexDecl[] =   
{
    VertexLayout::Element(0, offsetof(ParticleVertex, x),      VertexLayout::Format_Float3, VertexLayout::Semantic_Position, 0),
    VertexLayout::Element(0, offsetof(ParticleVertex, scaleRotLifeq), VertexLayout::Format_Short4, VertexLayout::Semantic_Texture,  0),
    VertexLayout::Element(0, offsetof(ParticleVertex, disp),   VertexLayout::Format_Short2, VertexLayout::Semantic_Texture,  1),
    VertexLayout::Element(0, offsetof(ParticleVertex, cline),  VertexLayout::Format_Short2, VertexLayout::Semantic_Texture,  2),
    VertexLayout::Element(0, offsetof(ParticleVertex, color),  VertexLayout::Format_UByte4, VertexLayout::Semantic_Texture,  3),
};

/*

   This is all we need to know about particles:

   0               1
   *---------------*
   |             / |
   |           /   |
   |         /     |
   |       /       |
   |     /         |
   |   /           |
   | /             |
   *---------------*
   2               3

*/
struct Emitter::Particle
{
    float  zpos; // sort key
    Vector3 pos;
    Vector3 vel;
    float  rot;
    float  spin;
    float  life;
    float  lifeSpan;
    float  sx;
    float  sy;
    uint32 cline;
};

void EmitterShared::init(VisualEngine* ve)
{
    if (ibuf) return; // already there

    visualEngine = ve;
    colorBGR = ve->getDevice()->getCaps().colorOrderBGR;
    Device* dev = visualEngine->getDevice(); 

    int numParticles = kMaxParticles;
    int numIndices = kMaxParticles * 6;

    vbuf = dev->createVertexBuffer(sizeof(ParticleVertex), kMaxParticles * 4, VertexBuffer::Usage_Dynamic);

    ibuf = dev->createIndexBuffer(2, numIndices, IndexBuffer::Usage_Static);
    index_t* ptr = (index_t*) ibuf->lock();
    for (int j=0; j<numParticles; j++)
    {
        ptr[6*j + 0] = 4*j + 0;
        ptr[6*j + 1] = 4*j + 1;
        ptr[6*j + 2] = 4*j + 2;
        ptr[6*j + 3] = 4*j + 2;
        ptr[6*j + 4] = 4*j + 1;
        ptr[6*j + 5] = 4*j + 3;
    }
    ibuf->unlock();

    if (!vlayout)
    {
        vlayout = dev->createVertexLayout(std::vector< VertexLayout::Element >(kVertexDecl, kVertexDecl + sizeof(kVertexDecl)/sizeof(kVertexDecl[0])));
    }

    shaders[0] = ve->getShaderManager()->getProgram("ParticleVS","ParticleAddFS");
    shaders[1] = ve->getShaderManager()->getProgram("ParticleVS","ParticleModulateFS");
    shaders[2] = ve->getShaderManager()->getProgram("ParticleVS","ParticleCrazyFS");
    shaders[3] = ve->getShaderManager()->getProgram("ParticleVS","ParticleCrazySparklesFS");
    shaders[4] = ve->getShaderManager()->getProgram("ParticleCustomVS","ParticleCustomFS");

    vblock = vbptr = vbend = 0;
}

void* EmitterShared::lock(int* retIndex, int vcnt)
{
    *retIndex = 0xbaadf00d;
    if (!vblock)
    {
        vblock = vbptr = vbuf->lock(VertexBuffer::Lock_Discard);
        vbend = (char*)vbptr + vbuf->getElementCount() * vbuf->getElementSize();
    }
    
    int bsize = vcnt * vbuf->getElementSize();

    if ((char*)vbptr + bsize > vbend) return 0; // won't fit

    void* ret = vbptr;
    *retIndex = ((char*)vbptr - (char*)vblock) / vbuf->getElementSize();
    vbptr = (char*)vbptr + bsize;
    return ret;
}

void EmitterShared::flush()
{
    if (!vbuf || !vblock) return;
    vbuf->unlock();
    vblock = vbptr = vbend = 0;
}

Emitter::Emitter(VisualEngine* ve, bool enableCurves_, const std::string& context)
: enableCurves(enableCurves_)
{
    
    Device* dev = ve->getDevice();
    RBXASSERT(dev);

    sharedState = ve->getEmitterSharedState();
    sharedState->init(ve);

    geom = dev->createGeometry(sharedState->vlayout, sharedState->vbuf, sharedState->ibuf, 0);
    batch.reset(new GeometryBatch(geom, Geometry::Primitive_Triangles, 0, 0, 0, 0));
    
    plist.reserve(60);
    emissionCounter = 0;

    Vector2 zz(0,0); 
    Vector3 zzz(0,0,0);

    life = Vector2(5,5);
    emitterShape = 0;
    emitterBox = Box(Vector3(-0.5f,-0.5f,-0.5f), Vector3(0.5f,0.5f,0.5f));
    emissionRate = 10;


    speed = Vector2(0,0);
    spread = zz;
    globalForce = localForce = zzz;
    dampening = 0;
    rotation = zz;
    spin = zz;
    sizeX = Vector2(1,1);
    sizeY = Vector2(1,1);
    growth = zz;
    maxSize = 0;
	velocity = Velocity();
	velocityInheritance = 0;
	lockedToLocalSpace = false;
	sphericalDirection = Vector2(M_PI / 2, M_PI / 2);

    Appearance def = {};
    def.blendCode = Blend_AlphaBlend;
    def.shader    = Shader_Modulate;
    def.mainTexture = "rbxasset://textures/particles/sparkles_main.dds";
    def.colorStripTexture = "rbxasset://textures/particles/sparkles_color.dds";
    def.alphaStripTexture = "rbxasset://textures/particles/common_alpha.dds";
    def.colorStripBaseline = -1;

    modulateColor = Vector4(1,1,1,1);
    zOffset       = 0;
    inheritMotion = 0;
    brightenOnThrottle = 0;
    blendRatio = 0.5f; // doesn't do anything unless configured to use a Crazy shader

    setAppearance(def, context);

    gEmitterCount++;
}

Emitter::~Emitter()
{
    gEmitterCount--;
}

static inline float lerp( float a, float b, float s ) { return a + (b-a)*s; }

static inline float sampleCurve( const Vector2* ptr, float t, float tr)
{
    static const int   kNumIntervals = Emitter::kNumCachePoints - 1;

    float ut = kNumIntervals * t; // un-normalized time, e.g.: 15.33 means that we're at the 15th interval, 33% towards the next one

    int i(ut); // current interval index
    float r = ut - i; // ratio towards the next point

    float min = lerp( ptr[i].x, ptr[i+1].x, r );
    float max = lerp( ptr[i].y, ptr[i+1].y, r );
    return lerp( min, max, tr );
}

static inline void sampleCurve( Vector3* val, const Vector3* a, const Vector3* b, float t, float tr )
{
    static const int kNumIntervals = Emitter::kNumCachePoints - 1;

    float ut = kNumIntervals * t; // un-normalized time, e.g.: 15.33 means that we're at the 15th interval, 33% towards the next one

    int i(ut); // current interval index
    float r = ut - i; // ratio towards the next point

    val->x = lerp( lerp( a[i].x, a[i+1].x, r ), lerp( b[i].x, b[i+1].x, r), tr  );
    val->y = lerp( lerp( a[i].y, a[i+1].y, r ), lerp( b[i].y, b[i+1].y, r), tr  );
    val->z = lerp( lerp( a[i].z, a[i+1].z, r ), lerp( b[i].z, b[i+1].z, r), tr  );
}

struct ZSortPr
{
    bool operator() (const Emitter::Particle& a, const Emitter::Particle& b) const
    {
        return a.zpos > b.zpos;
    }
};

void Emitter::simulateParticle(Particle& p, float dt, Vector3 wsAccel, CoordinateFrame disp, float weight)
{
	p.life = std::max(p.life - dt, 0.0f);
			
	Vector3 accel = wsAccel;
	if (lockedToLocalSpace)
	{
		accel = cframe.vectorToWorldSpace(globalForce) + cframe.vectorToWorldSpace(localForce);
		p.vel = cframe.vectorToWorldSpace(prevCframe.vectorToObjectSpace(p.vel));
		p.pos = cframe.pointToWorldSpace(prevCframe.pointToObjectSpace(p.pos));
	}

	p.pos += p.vel * dt + accel * (0.5f * dt*dt);
	p.vel += accel * dt;
			
	if (DFFlag::EnableParticleDrag)
	{
        p.vel *= powf(2.f, -dampening * dt);
    }
    else
    {
        p.vel = p.vel.lerp(Vector3(0,0,0), dampening*dt);
    }

	p.sx += growth.x * dt;
	p.sy += growth.y * dt;
}

void Emitter::sim(float dt)
{
    const RenderCamera& cam = sharedState->visualEngine->getCamera();

    FrameRateManager* frm = sharedState->visualEngine->getFrameRateManager();

    float ptf = (float)frm->GetParticleThrottleFactor();
    float emissionRateMul = G3D::clamp( ptf, kMinThrottle, kMaxThrottle );
    float camDist = (cam.getPosition() - cframe.translation).length();
    float distFactor = G3D::clamp(1 - (camDist - kThrottleDist) / (kCutoffDist - kThrottleDist), 0, 1);

    emissionCounter -= emit(emissionCounter);
    emissionCounter += dt * emissionRate * emissionRateMul * distFactor;

    if ( !plist.empty() )
    {
        // Motion inheritance: figure out how far the emitter has moved, then apply the displacement to each particle
        // (subject to inheritMotion property)
        CoordinateFrame disp =  cframe * prevCframe.inverse();

        Vector3 wsAccel = cframe.vectorToWorldSpace(localForce) + globalForce;
        float   weight = inheritMotion;

        for (unsigned j=0, e = plist.size(); j<e;)
        {
            //////////////////////////////////////////////////////////////////////////
			if (plist[j].life < 0.001f || plist[j].sx < 0 || plist[j].sy < 0)
			{
				e = kill(j);
				continue;
			}

			Particle& p = plist[j];
			simulateParticle(p, dt, wsAccel, disp, weight);

			plist[j].zpos = (plist[j].pos - cam.getPosition()).dot(cam.getDirection());

            ++j;
        }
    }

    prevCframe = cframe;
}

void Emitter::draw(RenderQueue& rq)
{
    if (!teq) return;
    
    float dt = sharedState->visualEngine->getFrameRateManager()->GetFrameTimeStats().getLatest() / 1000.f;
    bool longFrame = !!sharedState->visualEngine->getSettings()->getEagerBulkExecution(); // are we having a long frame?
    FrameRateManager* frm = sharedState->visualEngine->getFrameRateManager();
    float ptf = (float)frm->GetParticleThrottleFactor();
    float emissionRateMul = G3D::clamp( ptf, kMinThrottle, kMaxThrottle );
    const RenderCamera& cam = sharedState->visualEngine->getCamera();

    if (!longFrame)
    {
        dt = std::min( dt, 0.066f );
        sim(dt);
    }
    else
    {
        for (float st = dt;  st > 0; st -= kLongFrameSimStep)
        {
            sim(kLongFrameSimStep);
        }
    }

    if (plist.empty()) return;

    const int visiblePCount = plist.size();
    int startIndex;
    ParticleVertex* ptr = (ParticleVertex*) sharedState->lock(&startIndex, 4*visiblePCount);
    if (!ptr) return; // won't fit

    if (!plist.empty())
    {
        Emitter::Particle* p = &plist[0];
        std::sort(p, p+plist.size(), ZSortPr());
    }

    for (unsigned j=0, e = visiblePCount; j<e; ++j)
    {
        Particle& p = plist[j];

        ParticleVertex v;
        v.x = p.pos.x;
        v.y = p.pos.y;
        v.z = p.pos.z;
        v.cline[0] = p.cline;
        v.cline[1] = 0;

        float sx = p.sx;
        float sy = p.sy;
        Vector3 color;

        if (enableCurves)
        {
            float nl = 1 - p.life/p.lifeSpan; // normalized life
            float tr = p.cline/32767.0f; // trajectory

            sx = sy = sampleCurve( sizeCurve, nl, tr );
            sampleCurve( &color, colorMin, colorMax, nl, tr );

            // transparency
            v.color[3] = 255.0f * sampleCurve( alphaCurve, nl, tr );

            // color
            v.color[0] = 255.0f * color.x;
            v.color[1] = 255.0f * color.y;
            v.color[2] = 255.0f * color.z;
        }


        v.scaleRotLifeq[0] = short((sx - 127) * 256 + 0.5f );
        v.scaleRotLifeq[1] = short((sy - 127) * 256 + 0.5f );
        v.scaleRotLifeq[2] = short((p.rot + p.spin * p.life) * (32767.f / (2 * 3.1415926f)) + 0.5f);
        v.scaleRotLifeq[3] = short( (p.life / p.lifeSpan) * 32767.f + 0.5f);

        v.disp[0] = 0; v.disp[1] = 0;
        *ptr++ = v;

        v.disp[0] = 1; v.disp[1] = 0;
        *ptr++ = v;

        v.disp[0] = 0; v.disp[1] = 1;
        *ptr++ = v;

        v.disp[0] = 1; v.disp[1] = 1;
        *ptr++ = v;
    }
    
    RBXASSERT((char*)sharedState->vblock <= (char*)sharedState->vbptr  &&  (char*)sharedState->vbptr <= (char*)sharedState->vbend);

    teq->setConstant("throttleFactor", Vector4(kCutoffAlpha * (1-ptf), kAlphaBoost * (1-ptf), 0, blendRatio));
    teq->setConstant("modulateColor", lerp( modulateColor, modulateColor/emissionRateMul, brightenOnThrottle)  );
    teq->setConstant("zOffset", Vector4(zOffset,0,0,0));

    *batch = GeometryBatch(geom, Geometry::Primitive_Triangles, 6*startIndex/4, 6*visiblePCount, startIndex, startIndex + 4*visiblePCount);

    RenderOperation rop;
    rop.renderable = 0;
    rop.distanceKey = RenderEntity::computeViewDepth(cam, cframe.translation, -0.1f - zOffset);
    rop.geometry = this->batch.get();
    rop.technique = teq.get();

    rq.getGroup(rq.Id_Transparent).push(rop);

}

static Vector3 f3rand(G3D::Random& rnd, Vector3 ext)
{
    ext.x = rnd.uniform(-ext.x, ext.x);
    ext.y = rnd.uniform(-ext.y, ext.y);
    ext.z = rnd.uniform(-ext.z, ext.z);
    return ext;
}
/*
uint32 color(Vector4 v, bool d3d)
{
    v.x += 0.5f/255;
    v.y += 0.5f/255;
    v.z += 0.5f/255;
    v.w += 0.5f/255;
    v *= 255;

    v = v.clamp(0, 255.5f);

    if (d3d)
    {
        return (uint32(v.w) << 24) | (uint32(v.x) << 16) |  (uint32(v.y) << 8 ) |    (uint32(v.z));
    }
    else
    {
        return (uint32(v.w) << 24) | (uint32(v.z) << 16) |  (uint32(v.y) << 8 ) |    (uint32(v.x));
    }
}
*/

static Vector3 vrand(G3D::Random& rnd, Vector2 speed, Vector2 spread, Vector2 sphericalDirection)
{
	float deltaTheta = rnd.uniform(-spread.x, spread.x);
	float theta = sphericalDirection.x + deltaTheta;

	float deltaPhi = rnd.uniform(-spread.y, spread.y);
	float phi = sphericalDirection.y + deltaPhi;

	float radius = rnd.uniform(speed.x, speed.y);

	Vector3 ret;

	ret.x = radius * sinf(phi) * cosf(theta);
	ret.y = radius * sinf(phi) * sinf(theta);
	ret.z = radius * cosf(phi);

	return ret;
}

static Vector4 v4rand(G3D::Random& rnd)
{
    return Vector4(rnd.uniform(-1,1), rnd.uniform(-1,1), rnd.uniform(-1,1), rnd.uniform(-1,1));
}


int Emitter::emit(int n)
{
    if (!teq) return 0;
    if (plist.empty()) 
        prevCframe = cframe;

    n = std::min(n, int(kMaxParticlesPerEmitter - plist.size()));

    if (n <= 0) return 0;

    Vector3 ext = emitterBox.extent() * 0.5f; // because G3D::Box::extent() is not 'extent' as the rest of the world knows it.

    G3D::Random& rnd = sharedState->rnd;

    uint32 csSizeY = colorStripTex.getTexture() ? colorStripTex.getTexture()->getHeight() : 256;
    uint32 fixedCLine = (uint32) (32766.99f * appearance.colorStripBaseline / csSizeY);

    for (int j=0; j<n; ++j)
    {
        Particle p;
        
		Vector3 particlePosition = f3rand(rnd, ext);
		if (lockedToLocalSpace)
			p.pos = prevCframe.pointToWorldSpace(particlePosition);
		else
			p.pos = cframe.pointToWorldSpace(particlePosition);
		
		Vector3 particleVelocity = vrand(rnd, speed, spread, sphericalDirection);
		if (lockedToLocalSpace)
		{
			p.vel = prevCframe.vectorToWorldSpace(particleVelocity);
		}
		else
		{
			p.vel   = cframe.vectorToWorldSpace(particleVelocity);

			if (velocityInheritance != 0)
			{
				p.vel += velocity.linearVelocityAtOffset(p.pos - cframe.translation) * velocityInheritance;
			}
		}

        p.life   = p.lifeSpan = rnd.uniform(life.x, life.y);
        p.sx     = rnd.uniform(sizeX.x, sizeX.y );
        p.sy     = rnd.uniform(sizeY.x, sizeY.y );
        p.rot    = rnd.uniform(rotation.x, rotation.y);
        p.spin   = rnd.uniform(spin.x, spin.y);

        if (appearance.colorStripBaseline < 0 )
            p.cline  = rnd.uniform(0,32766.99f);
        else
            p.cline  = fixedCLine;
        
        plist.push_back(p);
    }
    return n;
}

int Emitter::kill(int n)
{
    std::vector< Emitter::Particle >& vec = plist;

    int last = vec.size();
    if (!last)
        return 0;

    last--;
    vec[n] = vec[last];
    vec.resize(last);
    return last;
}

static TextureRef gettex( VisualEngine* ve, const std::string& name, const std::string& context)
{
    return ve->getTextureManager()->load( ContentId(name), TextureManager::Fallback_BlackTransparent, context);
}

static BlendState createBlendState(Emitter::BlendMode blendMode)
{
    // to understand values of separate alpha blend, check the setupTechnique function in MaterialGenerator.cpp
	if (FFlag::GlowEnabled)
    {
        switch (blendMode)
        {
        case Emitter::Blend_None: 
            return BlendState(BlendState::Factor_One, BlendState::Factor_Zero, BlendState::Factor_One, BlendState::Factor_One);
            break;
        case Emitter::Blend_Additive:
            return BlendState(BlendState::Factor_One, BlendState::Factor_One, BlendState::Factor_InvDstAlpha, BlendState::Factor_One);
            break;
        case Emitter::Blend_Multiplicative:
            return BlendState(BlendState::Factor_DstColor, BlendState::Factor_Zero, BlendState::Factor_InvDstAlpha, BlendState::Factor_One);
            break;
        case Emitter::Blend_AlphaBlend:
            return BlendState(BlendState::Factor_SrcAlpha, BlendState::Factor_InvSrcAlpha, BlendState::Factor_InvDstAlpha, BlendState::Factor_One);
            break;
        case Emitter::Blend_PremultipliedAlpha:
            return BlendState(BlendState::Factor_One, BlendState::Factor_InvSrcAlpha, BlendState::Factor_InvDstAlpha, BlendState::Factor_One);
            break;
        case Emitter::Blend_AlphaOne: 
            return BlendState(BlendState::Factor_SrcAlpha, BlendState::Factor_One, BlendState::Factor_InvDstAlpha, BlendState::Factor_One);
            break;
        default:
            RBXASSERT(false); // did you add new mode?
            return BlendState(BlendState::Mode_None);
        }
    }
    else
    {
        switch (blendMode)
        {
        case Emitter::Blend_None: 
            return BlendState(BlendState::Factor_One, BlendState::Factor_Zero);
            break;
        case Emitter::Blend_Additive:
            return BlendState(BlendState::Factor_One, BlendState::Factor_One);
            break;
        case Emitter::Blend_Multiplicative:
            return BlendState(BlendState::Factor_DstColor, BlendState::Factor_Zero);
            break;
        case Emitter::Blend_AlphaBlend:
            return BlendState(BlendState::Factor_SrcAlpha, BlendState::Factor_InvSrcAlpha);
            break;
        case Emitter::Blend_PremultipliedAlpha:
            return BlendState(BlendState::Factor_One, BlendState::Factor_InvSrcAlpha);
            break;
        case Emitter::Blend_AlphaOne: 
            return BlendState(BlendState::Factor_SrcAlpha, BlendState::Factor_One);
            break;
        default:
            RBXASSERT(false); // did you add new mode?
            return BlendState(BlendState::Mode_None);
        }
    }
    
}

void Emitter::setAppearance(const Appearance& a, const std::string& context)
{
    appearance = a;
    teq.reset();

    if( !sharedState->shaders[a.shader] ) return;
    teq.reset( new Technique(sharedState->shaders[a.shader], 0 ) );

    if( !teq ) return;

    RasterizerState rs(RasterizerState::Cull_None, 0);
    DepthState ds(DepthState::Function_LessEqual, false, DepthState::Stencil_None);
    
    teq->setRasterizerState(rs);
    teq->setDepthState(ds);
    teq->setBlendState(createBlendState(a.blendCode));

    TextureRef black = sharedState->visualEngine->getTextureManager()->getFallbackTexture(TextureManager::Fallback_Black);
    
    TextureRef particleTexture = gettex( sharedState->visualEngine, a.mainTexture, context);
    TextureRef colorStripTexture = !a.colorStripTexture.empty() ? gettex( sharedState->visualEngine, a.colorStripTexture, context ) : black;
    TextureRef alphaStripTexture = !a.alphaStripTexture.empty() ? gettex( sharedState->visualEngine, a.alphaStripTexture, context ) : black;

    teq->setTexture( 0, particleTexture, SamplerState(SamplerState::Filter_Linear) );
    if (!enableCurves)
    {
        teq->setTexture( 1, colorStripTexture, SamplerState(SamplerState::Filter_Linear,SamplerState::Address_Clamp) );
        teq->setTexture( 2, alphaStripTexture, SamplerState(SamplerState::Filter_Linear,SamplerState::Address_Clamp) );
    }

    this->colorStripTex = colorStripTexture;
}

Extents Emitter::computeBBox()
{
    // estimates the bounding box around all these particles
    // does take dampening effect into account for now

	bool preComputeLockedToLocalSpace = lockedToLocalSpace;
	lockedToLocalSpace = false;

	Extents boundingBox = Extents(Vector3(-1, -1, -1), Vector3(1, 1, 1));
	Vector3 ext = emitterBox.extent() * 0.5f;

	Vector3 wsAccel = globalForce;
	if (preComputeLockedToLocalSpace)
		wsAccel = cframe.vectorToWorldSpace(wsAccel);

	for (int direction = 0; direction < 6; direction++)
	{
		Particle p;
		p.life = p.lifeSpan = life.y;
			
		switch(direction)
		{
		case 0:
			p.vel = cframe.vectorToWorldSpace(Vector3(1, 0, 0) * speed.y);
			p.pos =  cframe.pointToWorldSpace(Vector3(ext.x, 0, 0));
			break;
		case 1:
			p.vel = cframe.vectorToWorldSpace(Vector3(-1, 0, 0) * speed.y);
			p.pos =  cframe.pointToWorldSpace(Vector3(-ext.x, 0, 0));
			break;
		case 2:
			p.vel = cframe.vectorToWorldSpace(Vector3(0, 1, 0) * speed.y);
			p.pos =  cframe.pointToWorldSpace(Vector3(0, ext.y, 0));
			break;
		case 3:
			p.vel = cframe.vectorToWorldSpace(Vector3(0, -1, 0) * speed.y);
			p.pos =  cframe.pointToWorldSpace(Vector3(0, -ext.y, 0));
			break;
		case 4:
			p.vel = cframe.vectorToWorldSpace(Vector3(0, 0, 1) * speed.y);
			p.pos =  cframe.pointToWorldSpace(Vector3(0, 0, ext.z));
			break;
		case 5:
			p.vel = cframe.vectorToWorldSpace(Vector3(0, 0, -1) * speed.y);
			p.pos =  cframe.pointToWorldSpace(Vector3(0, 0, -ext.z));
			break;
		}

		if (velocityInheritance != 0)
			p.vel += velocity.linearVelocityAtOffset(p.pos - cframe.translation) * velocityInheritance;

		while (p.life > 0)
		{
			simulateParticle(p, 0.05, wsAccel, CoordinateFrame(), inheritMotion);
			boundingBox.expandToContain(cframe.pointToObjectSpace(p.pos));
		}
	}
        
	lockedToLocalSpace = preComputeLockedToLocalSpace;
	return boundingBox;
}

int Emitter::pcount() const
{
    return plist.size();
}

void Emitter::setEmissionRate(float v)
{
    this->emissionRate = clamp(v, 0, kMaxEmissionRate);
}

void Emitter::setLife(Vector2 v)
{
    v = Vector2(std::min(v.x, v.y), std::max(v.x, v.y));
    v.x = clamp(v.x, 0, kMaxLife);
    v.y = clamp(v.y, 0, kMaxLife);
    this->life = v;
}

//////////////////////////////////////////////////////////////////////////
// new stuff:

void Emitter::setColorCurve(const ColorSequence* v)
{
   v->resample(colorMin, colorMax, ARRAYSIZE(colorMin));
}

void Emitter::setAlphaCurve(const NumberSequence* v )
{
    v->resample(alphaCurve, ARRAYSIZE(alphaCurve), 0, 1);
    for( int j=0; j<kNumCachePoints; ++j)
    {
        alphaCurve[j] = G3D::Vector2(1,1) - alphaCurve[j].yx(); // flip
    }
}

void Emitter::setSizeCurve(const NumberSequence* v )
{
    maxSize = 0;
    v->resample(sizeCurve, ARRAYSIZE(sizeCurve), 0, (float)FInt::RenderMaxParticleSize);
    for( int j=0; j<kNumCachePoints; ++j)
    {
        maxSize = std::max( maxSize, sizeCurve[j].y );
    }
}

}}
