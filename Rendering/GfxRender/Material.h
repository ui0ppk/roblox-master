#pragma once

#include "GfxCore/States.h"
#include "g3d/Vector4.h"

#include "TextureRef.h"
#include "RenderQueue.h"

#include <vector>
#include <cstdlib>

namespace RBX
{
namespace Graphics
{

class DeviceContext;
class ShaderProgram;

class Technique
{
public:
    Technique(const shared_ptr<ShaderProgram>& program, unsigned int lodIndex, RenderQueue::Pass pass = RenderQueue::Pass_Default);

    void setRasterizerState(const RasterizerState& state);
	const RasterizerState& getRasterizerState() const { return rasterizerState; }

    void setDepthState(const DepthState& state);
	const DepthState& getDepthState() const { return depthState; }

    void setBlendState(const BlendState& state);
	const BlendState& getBlendState() const { return blendState; }

	void setTexture(unsigned int stage, const TextureRef& texture, const SamplerState& state);
	const TextureRef& getTexture(unsigned int stage) const;

    void setConstant(int handle, const Vector4& value);
    void setConstant(const char* name, const Vector4& value);

    void apply(DeviceContext* context) const;

	unsigned int getLodIndex() const { return lodIndex; }
	RenderQueue::Pass getPass() const { return pass; }

	const ShaderProgram* getProgram() const { return program.get(); }

private:
    struct TextureUnit
	{
		TextureRef texture;
        SamplerState state;
	};

    struct Constant
	{
        int handle;
        Vector4 value;
	};

    unsigned int lodIndex;
    RenderQueue::Pass pass;

    RasterizerState rasterizerState;
    DepthState depthState;
    BlendState blendState;

    shared_ptr<ShaderProgram> program;

    std::vector<TextureUnit> textures;
    std::vector<Constant> constants;
};

class Material
{
public:
    Material();

    void addTechnique(const Technique& technique);

    const Technique* getBestTechnique(unsigned int lodIndex, RenderQueue::Pass pass) const;

	const std::vector<Technique>& getTechniques() const { return techniques; }
	std::vector<Technique>& getTechniques() { return techniques; }

private:
    std::vector<Technique> techniques;
};

}
}
