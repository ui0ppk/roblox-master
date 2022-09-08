#pragma once

#include "Resource.h"

#include <vector>
#include <string>

namespace RBX
{
namespace Graphics
{

class VertexShader: public Resource
{
public:
    ~VertexShader();

    virtual void reloadBytecode(const std::vector<char>& bytecode) = 0;

protected:
	VertexShader(Device* device);
};

class FragmentShader: public Resource
{
public:
    ~FragmentShader();

    virtual void reloadBytecode(const std::vector<char>& bytecode) = 0;

protected:
	FragmentShader(Device* device);
};

class ShaderProgram: public Resource
{
public:
    ~ShaderProgram();

    virtual int getConstantHandle(const char* name) const = 0;

    virtual unsigned int getMaxWorldTransforms() const = 0;
    virtual unsigned int getSamplerMask() const = 0;

    static void dumpToFLog(const std::string& text, int channel);

protected:
	ShaderProgram(Device* device, const shared_ptr<VertexShader>& vertexShader, const shared_ptr<FragmentShader>& fragmentShader);

    shared_ptr<VertexShader> vertexShader;
    shared_ptr<FragmentShader> fragmentShader;
};

struct ShaderGlobalConstant
{
    const char* name;
    unsigned int offset;
    unsigned int size;

    ShaderGlobalConstant(const char* name, unsigned int offset, unsigned int size);
};

}
}
