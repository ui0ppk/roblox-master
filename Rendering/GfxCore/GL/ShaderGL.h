#pragma once

#include "GfxCore/Shader.h"

#include <vector>

namespace RBX
{
namespace Graphics
{

class VertexShaderGL: public VertexShader
{
public:
	VertexShaderGL(Device* device, const std::string& source);
    ~VertexShaderGL();

    virtual void reloadBytecode(const std::vector<char>& bytecode);

	unsigned int getId() const { return id; }

    unsigned int getAttribMask() const { return attribMask; }

private:
    unsigned int id;
    unsigned int attribMask;
};

class FragmentShaderGL: public FragmentShader
{
public:
    struct Sampler
	{
        std::string name;
        int slot;
	};

	FragmentShaderGL(Device* device, const std::string& source);
    ~FragmentShaderGL();

    virtual void reloadBytecode(const std::vector<char>& bytecode);

	unsigned int getId() const { return id; }

	const std::vector<Sampler>& getSamplers() const { return samplers; }

private:
    unsigned int id;
    std::vector<Sampler> samplers;
};

class ShaderProgramGL: public ShaderProgram
{
public:
    struct Uniform
	{
        enum Type
		{
            Type_Unknown,

            Type_Float,
            Type_Float2,
            Type_Float3,
            Type_Float4,
            Type_Float4x4,

            Type_Count
		};

        int location;
        Type type;
        unsigned int size;

        unsigned int offset;
	};

	ShaderProgramGL(Device* device, const shared_ptr<VertexShader>& vertexShader, const shared_ptr<FragmentShader>& fragmentShader);
    ~ShaderProgramGL();

    virtual int getConstantHandle(const char* name) const;

    virtual unsigned int getMaxWorldTransforms() const;
    virtual unsigned int getSamplerMask() const;

    void bind(const void* globalData, unsigned int globalVersion, ShaderProgramGL** cache);

    void setWorldTransforms4x3(const float* data, size_t matrixCount);
    void setConstant(int handle, const float* data, size_t vectorCount);

	unsigned int getId() const { return id; }

private:
    unsigned int id;

    int uniformWorldMatrix;
    int uniformWorldMatrixArray;

    unsigned int cachedGlobalVersion;

    std::vector<Uniform> globalUniforms;
    std::vector<std::pair<std::string, Uniform> > uniforms;

    unsigned int maxWorldTransforms;
    unsigned int samplerMask;
};

}
}
