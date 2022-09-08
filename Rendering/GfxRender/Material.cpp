#include "stdafx.h"
#include "Material.h"

#include "GfxCore/Device.h"

namespace RBX
{
namespace Graphics
{

Technique::Technique(const shared_ptr<ShaderProgram>& program, unsigned int lodIndex, RenderQueue::Pass pass)
	: lodIndex(lodIndex)
    , pass(pass)
	, rasterizerState(RasterizerState::Cull_Back)
	, depthState(DepthState::Function_LessEqual, true)
	, blendState(BlendState::Mode_None)
    , program(program)
{
    RBXASSERT(program);
}

void Technique::setRasterizerState(const RasterizerState& state)
{
	rasterizerState = state;
}

void Technique::setDepthState(const DepthState& state)
{
    depthState = state;
}

void Technique::setBlendState(const BlendState& state)
{
	blendState = state;
}

void Technique::setTexture(unsigned int stage, const TextureRef& texture, const SamplerState& state)
{
    if (stage >= textures.size())
	{
		TextureUnit dummy = {TextureRef(), SamplerState::Filter_Point};

        textures.resize(stage + 1, dummy);
	}

	TextureUnit tu = {texture, state};

	textures[stage] = tu;
}

void Technique::setConstant(int handle, const Vector4& value)
{
    if (handle < 0)
        return;

    for (size_t i = 0; i < constants.size(); ++i)
        if (constants[i].handle == handle)
		{
            constants[i].value = value;
            return;
		}

    Constant c = {handle, value};
    constants.push_back(c);
}

void Technique::setConstant(const char* name, const Vector4& value)
{
    setConstant(program->getConstantHandle(name), value);
}

void Technique::apply(DeviceContext* context) const
{
	context->setRasterizerState(rasterizerState);
	context->setDepthState(depthState);
	context->setBlendState(blendState);

	context->bindProgram(program.get());

    unsigned int mask = 0;
    unsigned int samplerMask = program->getSamplerMask();

    for (size_t i = 0; i < textures.size(); ++i)
	{
        const TextureUnit& tu = textures[i];
		const shared_ptr<Texture>& tex = tu.texture.getTexture();

		if (tex && (samplerMask & (1 << i)))
		{
            mask |= 1 << i;

            context->bindTexture(i, tex.get(), tu.state);
		}
	}

    for (size_t i = 0; i < constants.size(); ++i)
	{
        const Constant& c = constants[i];

        context->setConstant(c.handle, &c.value.x, 1);
	}

    // Verify that we set all samplers that the shader expected
	RBXASSERT((samplerMask & mask) == samplerMask);
}

const TextureRef& Technique::getTexture(unsigned int stage) const
{
    if (stage < textures.size())
        return textures[stage].texture;
	else
	{
        static const TextureRef dummy;
        return dummy;
	}
}

Material::Material()
{
}

void Material::addTechnique(const Technique& technique)
{
	RBXASSERT(techniques.empty() || (techniques.back().getPass() == technique.getPass() && techniques.back().getLodIndex() < technique.getLodIndex()) || techniques.back().getPass() < technique.getPass());

	techniques.push_back(technique);
}

const Technique* Material::getBestTechnique(unsigned int lodIndex, RenderQueue::Pass pass) const
{
    const Technique* best = NULL;
    
	for (size_t i = 0; i < techniques.size(); ++i)
    {
        const Technique& t = techniques[i];
        
        if (t.getPass() == pass)
        {
            if (t.getLodIndex() >= lodIndex)
                return &t;
            
            best = &t;
        }
    }

    // All techniques are better than LOD we need, return last matching one
	return best;
}

}
}
