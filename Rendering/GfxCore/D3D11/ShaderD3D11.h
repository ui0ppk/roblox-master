#pragma once

#include "GfxCore/Shader.h"

#include <vector>
#include <boost/function.hpp>
#include <boost/unordered_map.hpp>
#include <boost/enable_shared_from_this.hpp>


struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11Buffer;
struct ID3D11Device;
struct ID3D11InputLayout;

namespace RBX
{
namespace Graphics
{
    class VertexLayoutD3D11;
    class DeviceD3D11;

    struct UniformD3D11
    {
        std::string name;

        unsigned offset;
        unsigned size;
    };

    class CBufferD3D11 : public Resource
    {
    public:
        CBufferD3D11(Device* device, int registerId, const std::string& name, unsigned sizeIn, const std::vector<UniformD3D11>& uniformsIn);
        ~CBufferD3D11();

        const std::vector<UniformD3D11>& getUniforms () const { return uniforms; }
        const std::string& getName() const { return name; }
        int getRegisterId() { return registerId; }

        ID3D11Buffer* getObject() { return object; }
        const UniformD3D11& getUniform(int id) const;

        void updateUniform(int uniformId, const float* uniformData, unsigned vectorCount);
        void updateBuffer();

        int findUniform(const std::string& uniformName);

    private: 
        int registerId;
        std::vector<UniformD3D11> uniforms;
        std::string name;
        ID3D11Buffer* object;
        bool dirty;

        size_t size;
        char* data;
    };

    class BaseShaderD3D11 
    {
    public:
        BaseShaderD3D11(const std::vector<char>& bytecode);

        const std::vector<char>& getByteCode() const { return bytecode; }

        int findUniform(const std::string& name);
        void setConstant(int handle, const float* data, size_t vectorCount);
        
        void updateConstantBuffers();
        
        typedef std::vector<shared_ptr<CBufferD3D11>> CBufferList;
        const CBufferList& getCBuffers() const { return cBuffers; }

    protected:
        typedef CBufferList CBufferList;

        std::vector<shared_ptr<CBufferD3D11>> cBuffers;
        int uniformsBufferId;

        std::vector<char> bytecode;
    };

    class VertexShaderD3D11: public VertexShader, public BaseShaderD3D11, public boost::enable_shared_from_this<VertexShaderD3D11>
    {
    public:
        VertexShaderD3D11(Device* device, const std::vector<char>& bytecode);
        ~VertexShaderD3D11();

        virtual void reloadBytecode(const std::vector<char>& bytecode);
        ID3D11VertexShader* getObject() const { return object; }

        ID3D11InputLayout* getInputLayout11(VertexLayoutD3D11* vertexLayout);
        void removeLayout(VertexLayoutD3D11* vertexLayout);
        
        void setWorldTransforms4x3(const float* data, size_t matrixCount);
        unsigned int getMaxWorldTransforms() const { return maxWorldTransforms; }

    private:
        ID3D11VertexShader* object;

        int worldMatrixArray;
        int worldMatrix;
        int worldMatrixCbuffer;
        unsigned int maxWorldTransforms;

        typedef boost::unordered_map<VertexLayoutD3D11*, ID3D11InputLayout*> InputLayoutMap;
        InputLayoutMap inputLayoutMap;

        shared_ptr<VertexShaderD3D11> sharedThis;
    };

    class FragmentShaderD3D11: public FragmentShader, public BaseShaderD3D11
    {
    public:
        FragmentShaderD3D11(Device* device, const std::vector<char>& bytecode);
        ~FragmentShaderD3D11();

        virtual void reloadBytecode(const std::vector<char>& bytecode);

        ID3D11PixelShader* getObject() const { return object; }
        const std::vector<shared_ptr<CBufferD3D11>>& getCBuffers() const { return cBuffers; }
        unsigned int getSamplerMask() { return samplerMask; }

    private:
        ID3D11PixelShader* object;
        unsigned int samplerMask;
    };

    class ShaderProgramD3D11: public ShaderProgram
    {
    public:
        ShaderProgramD3D11(Device* device, const shared_ptr<VertexShader>& vertexShader, const shared_ptr<FragmentShader>& fragmentShader);
        ~ShaderProgramD3D11();

        virtual int getConstantHandle(const char* name) const;

        virtual unsigned int getMaxWorldTransforms() const;
        virtual unsigned int getSamplerMask() const;

        ID3D11InputLayout* getInputLayout11(VertexLayoutD3D11* vertexLayout);

        void bind();
        void setWorldTransforms4x3(const float* data, size_t matrixCount);
        void setConstant(int handle, const float* data, size_t vectorCount);
        void uploadConstantBuffers();

        static std::string createShaderSource(const std::string& path, const std::string& defines, const DeviceD3D11* device11, boost::function<std::string (const std::string&)> fileCallback);
        static std::vector<char> createShaderBytecode(const std::string& source, const std::string& target, const DeviceD3D11* device, const std::string& entrypoint);
        static HMODULE loadShaderCompilerDLL();
    };

}
}
