#pragma once

#include "GfxCore/Shader.h"

#include <vector>
#include <boost/function.hpp>

struct IDirect3DVertexShader9;
struct IDirect3DPixelShader9;

namespace RBX
{
namespace Graphics
{

struct UniformD3D9
{
    enum RegisterSet
	{
        Register_Float,
        Register_Bool,
        Register_Int,

        Register_Count
	};

    std::string name;

    RegisterSet registerSet;
    unsigned int registerIndex;
    unsigned int registerCount;
};

class VertexShaderD3D9: public VertexShader
{
public:
	VertexShaderD3D9(Device* device, const std::vector<char>& bytecode);
    ~VertexShaderD3D9();

    virtual void reloadBytecode(const std::vector<char>& bytecode);

	IDirect3DVertexShader9* getObject() const { return object; }

	int getRegisterWorldMatrix() const { return registerWorldMatrix; }
	int getRegisterWorldMatrixArray() const { return registerWorldMatrixArray; }
	unsigned int getMaxWorldTransforms() const { return maxWorldTransforms; }

	const std::vector<UniformD3D9>& getUniforms() const { return uniforms; }

private:
    IDirect3DVertexShader9* object;

    int registerWorldMatrix;
    int registerWorldMatrixArray;
    unsigned int maxWorldTransforms;

	std::vector<UniformD3D9> uniforms;
};

class FragmentShaderD3D9: public FragmentShader
{
public:
	FragmentShaderD3D9(Device* device, const std::vector<char>& bytecode);
    ~FragmentShaderD3D9();

    virtual void reloadBytecode(const std::vector<char>& bytecode);

	IDirect3DPixelShader9* getObject() const { return object; }

	unsigned int getSamplerMask() const { return samplerMask; }

	const std::vector<UniformD3D9>& getUniforms() const { return uniforms; }

private:
    IDirect3DPixelShader9* object;

    unsigned int samplerMask;

	std::vector<UniformD3D9> uniforms;
};

class ShaderProgramD3D9: public ShaderProgram
{
public:
	ShaderProgramD3D9(Device* device, const shared_ptr<VertexShader>& vertexShader, const shared_ptr<FragmentShader>& fragmentShader);
    ~ShaderProgramD3D9();

    virtual int getConstantHandle(const char* name) const;

    virtual unsigned int getMaxWorldTransforms() const;
    virtual unsigned int getSamplerMask() const;

    void bind();
    void setWorldTransforms4x3(const float* data, size_t matrixCount);
    void setConstant(int handle, const float* data, size_t vectorCount);

    static std::string createShaderSource(const std::string& path, const std::string& defines, boost::function<std::string (const std::string&)> fileCallback);
    static std::vector<char> createShaderBytecode(const std::string& source, const std::string& target, const std::string& entrypoint);
};

class ShaderProgramFFPD3D9: public ShaderProgram
{
public:
	ShaderProgramFFPD3D9(Device* device);
    ~ShaderProgramFFPD3D9();

    virtual int getConstantHandle(const char* name) const;

    virtual unsigned int getMaxWorldTransforms() const;
    virtual unsigned int getSamplerMask() const;
};

}
}
