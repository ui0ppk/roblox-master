#pragma once

#include "g3d/Random.h"
#include "g3d/CoordinateFrame.h"
#include "TextureRef.h"

namespace RBX { class ColorSequence; class NumberSequence; }

namespace RBX{namespace Graphics{

class VisualEngine;
class RenderQueue;
struct EmitterShared;
class VertexBuffer;
class Geometry;
class GeometryBatch;
class Technique;

class Emitter
{
public:

    enum ShaderType
    {
        Shader_Add,
        Shader_Modulate,
        Shader_Crazy,         // a weird blend of alpha blend and additive blend
        Shader_CrazySparkles, // and we need to go deeper...
        Shader_Custom,
        Shader__Count,
    };

    // Numbers MUST match BlendState::Mode, see gfxcore/states.h
    enum BlendMode
    {
       Blend_None,
       Blend_Additive,
       Blend_Multiplicative,
       Blend_AlphaBlend,
       Blend_PremultipliedAlpha,
       Blend_AlphaOne,
       Blend__Count    
    };

    struct Appearance
    {
        std::string  mainTexture;        // texture on the particle itself
        std::string  colorStripTexture;  // a color strip that determines particle color over its lifetime
        std::string  alphaStripTexture;  // a color strip that determines particle alpha over its lifetime. RED CHANNEL!
        int          colorStripBaseline; // line on the color strip texture to use, -1 = use random
        BlendMode    blendCode;
        ShaderType   shader;
    };

    Emitter(VisualEngine* ve, bool enableCurves = false, const std::string& context = "");
    ~Emitter();

    G3D::CoordinateFrame& transform() { return cframe; }
    void draw(RenderQueue& rq);

    struct                      Particle;

    void setLife(Vector2 v);
    void setEmitterShape(int shape, Box bbox) { emitterShape = shape; emitterBox = bbox; }
    void setEmissionRate(float v);

    void setSpeed(Vector2 v)                           { speed = v; }
    void setSpread(Vector2 v)                          { spread = v; }
    void setLocalForce(Vector3 v)                      { localForce = v; }
    void setGlobalForce(Vector3 v)                     { globalForce = v; }
    void setDampening(float v)                         { dampening = v; }
    void setRotation(Vector2 v)                        { rotation = v; }
    void setSpin(Vector2 v)                            { spin = v; }
    void setSizeX(Vector2 v)                           { sizeX = v; }
    void setSizeY(Vector2 v)                           { sizeY = v; }
    void setGrowth(Vector2 v)                          { growth = v; }
	void setVelocity(Velocity v)					   { velocity = v; }
	void setVelocityInheritance(float v)			   { velocityInheritance = v; }
	void setLockedToLocalSpace(bool v)				   { lockedToLocalSpace = v; }

    void setAppearance(const Appearance& a, const std::string& context = "");
    void setModulateColor(const Vector4& v)            { modulateColor = v; }
    void setZOffset(float v)                           { zOffset = v; }
    void setInheritMotion(float v)                     { inheritMotion = v; }
    void setBrightenOnThrottle(float b)                { brightenOnThrottle = b; }
    void setBlendRatio(float v)                        { blendRatio = v; }

	void setSphericalDirection(Vector2 v)			   { sphericalDirection = v; }
   
    int  emit(int n);
    Extents computeBBox();  // computes maximum possible bbox for this 
    int  pcount() const;

    // new stuff:
    void setColorCurve(const ColorSequence* v);
    void setAlphaCurve(const NumberSequence* v );
    void setSizeCurve(const NumberSequence* v );

    enum { kNumCachePoints = 32};
private:
    EmitterShared*              sharedState;
    shared_ptr<Geometry>        geom;
    std::vector<Particle>       plist;
    G3D::CoordinateFrame        cframe;
    G3D::CoordinateFrame        prevCframe;
    shared_ptr<GeometryBatch>   batch;
    float                       clock;
    float                       emissionCounter;

    // parameters

    // emission
    Vector2                     life;   // .x = min, .y = max
    int                         emitterShape;
    float                       emissionRate;
    Box                         emitterBox;
    
    // motion
    Vector2                     speed; // .x = min, .y = max
    Vector2                     spread; // .x = x, .y = .z  (i know...)
    Vector3                     localForce;
    Vector3                     globalForce;
    float                       dampening;
    Vector2                     rotation; // .x = min, .y = max
    Vector2                     spin; // .x = min, .y = max
    Vector2                     sizeX;   // .x = min, .y = max
    Vector2                     sizeY;   // .x = min, .y = max
    Vector2                     growth;  // per second
	Velocity					velocity;
	bool						lockedToLocalSpace;
	float						velocityInheritance; //determines whether the particles gain the velocity of the emitting part
	Vector2						sphericalDirection; //basically is the direction of the emission in spherical theta/phi format

    
    // NEW STUFF!
    const bool enableCurves; // new mode
    Vector3   colorMin[kNumCachePoints];
    Vector3   colorMax[kNumCachePoints];
    Vector2   alphaCurve[kNumCachePoints];
    Vector2   sizeCurve[kNumCachePoints];
    
    float     maxSize;



    // appearance
    Appearance                  appearance;
    shared_ptr<Technique>       teq;
    TextureRef                  colorStripTex; // color strip height
    float                       brightenOnThrottle; // should the particles be brightened proportionally when throttled? (sometimes makes sense for some additives, default=false)
    float                       blendRatio; // blend ratio for particles crazy shaders, not used otherwise

    // misc
    Vector4                     modulateColor;
    float                       zOffset;
    float                       inheritMotion;
	
    int  kill(int nth);
    void sim(float dt);
	void simulateParticle(Particle& p, float dt, Vector3 wsAccel, CoordinateFrame disp, float weight);
};

}}

