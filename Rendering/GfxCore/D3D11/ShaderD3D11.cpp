#define INITGUID 
#include "ShaderD3D11.h"

#include "GeometryD3D11.h"
#include "DeviceD3D11.h"

#include "HeadersD3D11.h"

#include <iostream>
#include <fstream>

#include <sstream>
#include <map>

LOGGROUP(Graphics)

namespace RBX
{
namespace Graphics
{
    typedef HRESULT (WINAPI *TypeD3DCompile)(LPCVOID, SIZE_T, LPCSTR, const D3D_SHADER_MACRO *, ID3DInclude *, LPCSTR, LPCSTR, UINT, UINT, ID3DBlob **, ID3DBlob **);
    typedef HRESULT (WINAPI *TypeD3DPreprocess)(LPCVOID, SIZE_T, LPCSTR, const D3D_SHADER_MACRO *, ID3DInclude *, ID3DBlob **, ID3DBlob **);
    typedef HRESULT (WINAPI *TypeD3DReflect)(LPCVOID, SIZE_T, REFIID, void**);

    static TypeD3DCompile loadShaderCompiler()
    {
#if !defined(RBX_PLATFORM_DURANGO)
        HMODULE d3dCompiler = ShaderProgramD3D11::loadShaderCompilerDLL();

        return d3dCompiler ? (TypeD3DCompile)GetProcAddress(d3dCompiler, "D3DCompile") : NULL;
#else
        return &D3DCompile;
#endif
    }

    static TypeD3DPreprocess loadShaderPreprocessor()
    {
#if !defined(RBX_PLATFORM_DURANGO)
        HMODULE d3dCompiler = ShaderProgramD3D11::loadShaderCompilerDLL();

        return d3dCompiler ? (TypeD3DPreprocess)GetProcAddress(d3dCompiler, "D3DPreprocess") : NULL;
#else
        return &D3DPreprocess;
#endif
    }

    static TypeD3DReflect loadShaderReflector()
    {
#if !defined(RBX_PLATFORM_DURANGO)
        HMODULE d3dCompiler = ShaderProgramD3D11::loadShaderCompilerDLL();

        return d3dCompiler ? (TypeD3DReflect)GetProcAddress(d3dCompiler, "D3DReflect") : NULL;
#else
        return &D3DReflect;
#endif
    }
    
    static void extractCbuffers(Device* device, const std::vector<char>& bytecode, std::vector<shared_ptr<CBufferD3D11>>& cbuffers, unsigned globalSize, unsigned int* outSamplerMask)
    {
        TypeD3DReflect D3DReflect = loadShaderReflector();
        RBXASSERT(D3DReflect);

        cbuffers.clear();

        ID3D11ShaderReflection* shaderReflection11 = NULL;
        HRESULT hr = D3DReflect(bytecode.data(), bytecode.size(), IID_ID3D11ShaderReflection, (void**) &shaderReflection11);
        RBXASSERT(SUCCEEDED(hr));

        D3D11_SHADER_DESC shaderDesc;
        shaderReflection11->GetDesc( &shaderDesc );

        unsigned int samplerMask = 0;
        for (unsigned i = 0; i < shaderDesc.BoundResources; ++i)
        {
            D3D11_SHADER_INPUT_BIND_DESC desc;
            HRESULT hr =shaderReflection11->GetResourceBindingDesc (i, &desc);
            RBXASSERT(SUCCEEDED(hr));
           
            if(desc.Type == D3D_SIT_TEXTURE)
            {
                samplerMask |= 1 << desc.BindPoint;
            }
            else if(desc.Type == D3D_SIT_CBUFFER)
            {
                ID3D11ShaderReflectionConstantBuffer* cb = shaderReflection11->GetConstantBufferByName(desc.Name);

                D3D11_SHADER_BUFFER_DESC cbDesc;
                cb->GetDesc( &cbDesc );

                if (cbDesc.Type == D3D11_CT_CBUFFER)
                {

                    if (desc.BindPoint == 0)
                    {
                        bool isGlobals = strcmp(cbDesc.Name, "Globals") == 0;
                        
                        if (!isGlobals)
                            throw std::runtime_error("D3D11 Shader compilation: Globals are not bound to register 0 (or not bound at all)");
                        
                        if (isGlobals && cbDesc.Size != globalSize)
                            throw std::runtime_error("D3D11 Shader compilation: Globals CBuffer size is not the same as defined CBuffer size");

                        continue; // globals are same for all the shaders globally in deviceContext, so we want to skip it here
                    }

                    std::vector<UniformD3D11> uniforms;

                    for ( unsigned varId = 0; varId < cbDesc.Variables; ++varId )
                    {
                        ID3D11ShaderReflectionVariable* var = cb->GetVariableByIndex( varId );

                        D3D11_SHADER_VARIABLE_DESC varDesc;
                        var->GetDesc(&varDesc);

                        ID3D11ShaderReflectionType* type = var->GetType();
                        D3D11_SHADER_TYPE_DESC typeDesc;
                        type->GetDesc( &typeDesc );

                        UniformD3D11 uniform = UniformD3D11();
                        uniform.name = varDesc.Name;
                        uniform.offset = varDesc.StartOffset;
                        uniform.size = varDesc.Size;
                        uniforms.push_back(uniform);
                    }

                    cbuffers.push_back(shared_ptr<CBufferD3D11>(new CBufferD3D11(device, desc.BindPoint, cbDesc.Name, cbDesc.Size, uniforms)));
                }
            }
        }

        if (outSamplerMask)
            *outSamplerMask = samplerMask; 

        ReleaseCheck(shaderReflection11);
    }

    static int findCBuffer(const std::vector<shared_ptr<CBufferD3D11>>& cBuffers, const std::string& name)
    {
        for(unsigned i = 0; i < cBuffers.size(); ++i)
        {
            if (cBuffers[i]->getName() == name)
                return i;
        }
        return -1;
    }

    static ID3D11VertexShader* createVertexShader(Device* device, const std::vector<char>& bytecode,
        std::vector<shared_ptr<CBufferD3D11>>& cbuffers, int& uniformsCBuffer, unsigned& maxWorldTransforms, int& worldMatCBuffer, int& uniformWorldMatrix, int& uniformWorldMatrixArray)
    {
        DeviceContextD3D11* context = static_cast<DeviceD3D11*>(device)->getImmediateContextD3D11();
        ID3D11Device* device11 = static_cast<DeviceD3D11*>(device)->getDevice11();

        extractCbuffers(device, bytecode, cbuffers, context->getGlobalDataSize(), NULL);
        
        uniformsCBuffer = findCBuffer(cbuffers, "$Globals");
        worldMatCBuffer = findCBuffer(cbuffers, "WorldMatrixCB");
        
        if (worldMatCBuffer >= 0)
        {
            uniformWorldMatrix = cbuffers[worldMatCBuffer]->findUniform("WorldMatrix");
            if (uniformWorldMatrix >= 0)
            {
                maxWorldTransforms = 1;
            }

            uniformWorldMatrixArray = cbuffers[worldMatCBuffer]->findUniform("WorldMatrixArray");
            if (uniformWorldMatrixArray >= 0)
            {
                const UniformD3D11& uniform = cbuffers[worldMatCBuffer]->getUniform(uniformWorldMatrixArray);
                maxWorldTransforms = uniform.size / (4 * 4 * 3); // 4bytes per float * 3 vectors
            }
        }

        ID3D11VertexShader* vertexShader = NULL;
        HRESULT hr = device11->CreateVertexShader( bytecode.data(), bytecode.size(), NULL, &vertexShader);
        RBXASSERT(SUCCEEDED(hr));

        return vertexShader;
    }

    static ID3D11PixelShader* createPixelShader(Device* device, const std::vector<char>& bytecode,
        std::vector<shared_ptr<CBufferD3D11>>& cbuffers, int& uniformsCBuffer, unsigned int& samplerMask)
    {
        DeviceContextD3D11* context = static_cast<DeviceD3D11*>(device)->getImmediateContextD3D11();
        ID3D11Device* device11 = static_cast<DeviceD3D11*>(device)->getDevice11();

        extractCbuffers(device, bytecode, cbuffers, context->getGlobalDataSize(), &samplerMask);
        uniformsCBuffer = findCBuffer(cbuffers, "$Globals");

        ID3D11PixelShader* pixelShader = NULL;
        HRESULT hr = device11->CreatePixelShader(bytecode.data(), bytecode.size(), NULL, &pixelShader);
        RBXASSERT(SUCCEEDED(hr));

        return pixelShader;
    }

    const UniformD3D11& CBufferD3D11::getUniform(int id) const
    {
        RBXASSERT(id >= 0 && id < (int)uniforms.size());
        return uniforms[id];
    }

    CBufferD3D11::CBufferD3D11(Device* device, int registerId, const std::string& name, unsigned sizeIn, const std::vector<UniformD3D11>& uniformsIn):
        Resource(device),
        registerId(registerId),
        name(name),
        size(sizeIn),
        data(NULL),
        dirty(true),
        uniforms(uniformsIn),
        object(NULL)
    {
        ID3D11Device* device11 = static_cast<DeviceD3D11*>(device)->getDevice11();

        data = new char[size];
        memset(data, 0, size);

        D3D11_BUFFER_DESC cbDesc;
        cbDesc.Usage = D3D11_USAGE_DEFAULT;
        cbDesc.ByteWidth = size;
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = 0;
        cbDesc.MiscFlags = 0;
        cbDesc.StructureByteStride = 0;

        HRESULT hr = device11->CreateBuffer(&cbDesc, NULL, &object);
        RBXASSERT(SUCCEEDED(hr));
    }

    void CBufferD3D11::updateUniform(int uniformId, const float* uniformData, unsigned vectorCount)
{
        size_t uniformSize = vectorCount * sizeof(float) * 4; // our vectors are always float4
        RBXASSERT(uniformId >= 0 && uniformId < (int)uniforms.size());
        RBXASSERT(uniforms[uniformId].size >= uniformSize);

        if (dirty || memcmp(&data[uniforms[uniformId].offset], uniformData, uniformSize) != 0)
        {
            dirty = true;
            memcpy(&data[uniforms[uniformId].offset], uniformData, uniformSize);
        }
    }

    void CBufferD3D11::updateBuffer()
    {
        if (dirty)
        {
            ID3D11Device* device11 =  static_cast<DeviceD3D11*>(device)->getDevice11();
            ID3D11DeviceContext* context11 = static_cast<DeviceD3D11*>(device)->getImmediateContext11();

            context11->UpdateSubresource(object, 0, NULL, data, 0, 0);

            dirty = false;
        }
    }

    int CBufferD3D11::findUniform(const std::string& uniformName)
    {       
        for(unsigned i = 0; i < uniforms.size(); ++i)
        {
            if (uniforms[i].name == uniformName) 
                return i;
        }
        return -1;
    }

    CBufferD3D11::~CBufferD3D11()
    {
        ReleaseCheck(object);
        delete[] data;
    }

    BaseShaderD3D11::BaseShaderD3D11(const std::vector<char>& bytecode)
        : bytecode(bytecode)
        , uniformsBufferId(-1)
    {
    }

    int BaseShaderD3D11::findUniform(const std::string& name)
    {
        if (uniformsBufferId < 0) return -1;

        return cBuffers[uniformsBufferId]->findUniform(name);
    }

    void BaseShaderD3D11::setConstant(int handle, const float* data, size_t vectorCount)
    {
        if (uniformsBufferId < 0) 
            return;

        cBuffers[uniformsBufferId]->updateUniform(handle, data, vectorCount);
    }

    void BaseShaderD3D11::updateConstantBuffers()
    {
        for (size_t i = 0; i < cBuffers.size(); ++i)
            cBuffers[i]->updateBuffer();
    }

    VertexShaderD3D11::VertexShaderD3D11(Device* device, const std::vector<char>& bytecode)
        : VertexShader(device)
        , BaseShaderD3D11(bytecode)
        , object(NULL)
        , worldMatrixCbuffer(-1)
        , worldMatrixArray(-1)
        , worldMatrix(-1)
        , maxWorldTransforms(0)
    {
        object = createVertexShader(device, bytecode, cBuffers, uniformsBufferId, maxWorldTransforms, worldMatrixCbuffer, worldMatrix, worldMatrixArray);
    }

    void VertexShaderD3D11::reloadBytecode(const std::vector<char>& bytecode)
    {
        ID3D11VertexShader* newObject = createVertexShader(device, bytecode, cBuffers, uniformsBufferId, maxWorldTransforms, worldMatrixCbuffer, worldMatrix, worldMatrixArray);
        ReleaseCheck(object);

        object = newObject;
        this->bytecode = bytecode;
    }

    ID3D11InputLayout* VertexShaderD3D11::getInputLayout11(VertexLayoutD3D11* vertexLayout)
    {
        ID3D11InputLayout* inputLayout11 = inputLayoutMap[vertexLayout];

        if (!inputLayout11)
        {
            ID3D11Device* device11 = static_cast<DeviceD3D11*>(device)->getDevice11();

            HRESULT hr = device11->CreateInputLayout(vertexLayout->getElements11(), vertexLayout->getElementsCount(), bytecode.data(), bytecode.size(), &inputLayout11);
            RBXASSERT(SUCCEEDED(hr));

            vertexLayout->registerShader(shared_from_this());
            inputLayoutMap[vertexLayout] = inputLayout11;
        }
    
        return inputLayout11;
    }

    void VertexShaderD3D11::removeLayout(VertexLayoutD3D11* vertexLayout)
    {
        InputLayoutMap::iterator it = inputLayoutMap.find(vertexLayout);

        if (it != inputLayoutMap.end())
        {
            ReleaseCheck(it->second);
            inputLayoutMap.erase(it);
        }
    }

    void VertexShaderD3D11::setWorldTransforms4x3(const float* data, size_t matrixCount)
    {
        if (worldMatrixCbuffer < 0) return;

        if (worldMatrix >= 0)
        {
            static const float lastRow[4] = {0,0,0,1};
            float matrix[16];

            memcpy(matrix, data, 3 * 4 * 4);
            memcpy(&matrix[12], lastRow, 16);

            cBuffers[worldMatrixCbuffer]->updateUniform(0, matrix, 4);
        }

        if (worldMatrixArray >= 0)
            cBuffers[worldMatrixCbuffer]->updateUniform(0, data, matrixCount * 3);
    }

    FragmentShaderD3D11::FragmentShaderD3D11(Device* device, const std::vector<char>& bytecode)
        : FragmentShader(device)
        , BaseShaderD3D11(bytecode)
        , object(NULL)
        , samplerMask(0)
    {
        object = createPixelShader(device, bytecode, cBuffers, uniformsBufferId, samplerMask);
    }

    void FragmentShaderD3D11::reloadBytecode(const std::vector<char>& bytecode)
    {
        ID3D11PixelShader* newObject = createPixelShader(device, bytecode, cBuffers, uniformsBufferId, samplerMask);
        ReleaseCheck(object);

        object = newObject;
        this->bytecode = bytecode;
    }

    FragmentShaderD3D11::~FragmentShaderD3D11()
    {
        ReleaseCheck(object);
    }

    VertexShaderD3D11::~VertexShaderD3D11()
    {
        ReleaseCheck(object);
        for (InputLayoutMap::iterator it = inputLayoutMap.begin(); it != inputLayoutMap.end(); ++it)
            ReleaseCheck(it->second);
    }

    static void verifyShaderSignatures(const VertexShaderD3D11* vs, const FragmentShaderD3D11* fs)
    {
        TypeD3DReflect D3DReflect = loadShaderReflector();
        RBXASSERT(D3DReflect);

        ID3D11ShaderReflection* reflectionVS11 = NULL;
        ID3D11ShaderReflection* reflectionFS11 = NULL;
        D3DReflect(vs->getByteCode().data(), vs->getByteCode().size(), IID_ID3D11ShaderReflection, (void**) &reflectionVS11);
        D3DReflect(fs->getByteCode().data(), fs->getByteCode().size(), IID_ID3D11ShaderReflection, (void**) &reflectionFS11);

        // Get shader info
        D3D11_SHADER_DESC shaderDescVS;
        D3D11_SHADER_DESC shaderDescFS;
        reflectionVS11->GetDesc(&shaderDescVS);
        reflectionFS11->GetDesc(&shaderDescFS);

        RBXASSERT(shaderDescVS.OutputParameters >= shaderDescFS.InputParameters);

        // we have to find matching signature from FS in VS
        for (unsigned fsId = 0; fsId < shaderDescFS.InputParameters; ++fsId)
        {

            D3D11_SIGNATURE_PARAMETER_DESC fsDesc;
            reflectionFS11->GetInputParameterDesc(fsId, &fsDesc);
            bool found = false;

            for ( unsigned i=0; i< shaderDescVS.OutputParameters; i++ )
            {
                D3D11_SIGNATURE_PARAMETER_DESC vsDesc;
                reflectionVS11->GetOutputParameterDesc(i, &vsDesc);

                if ((vsDesc.ComponentType == fsDesc.ComponentType) && 
                    (vsDesc.Register == fsDesc.Register) && 
                    (vsDesc.SemanticIndex == fsDesc.SemanticIndex) &&
                    (strcmp(vsDesc.SemanticName, fsDesc.SemanticName) == 0) &&
                    (vsDesc.SystemValueType == fsDesc.SystemValueType))
                {
                    found = true;
                    break;
                }           
            }
            RBXASSERT(found);
        }

        ReleaseCheck(reflectionVS11);
        ReleaseCheck(reflectionFS11);
    }

    ShaderProgramD3D11::ShaderProgramD3D11(Device* device, const shared_ptr<VertexShader>& vertexShader, const shared_ptr<FragmentShader>& fragmentShader)
        : ShaderProgram(device, vertexShader, fragmentShader)
    {
#ifdef __RBX_NOT_RELEASE
       verifyShaderSignatures(static_cast<VertexShaderD3D11*>(vertexShader.get()), static_cast<FragmentShaderD3D11*>(fragmentShader.get()));
#endif
    }

    ShaderProgramD3D11::~ShaderProgramD3D11()
    {
        static_cast<DeviceD3D11*>(device)->getImmediateContextD3D11()->invalidateCachedProgram();
    }

    struct IncludeCallback: ID3DInclude
    {
        boost::function<std::string (const std::string&)> fileCallback;
        std::map<const void*, std::string> paths;

        IncludeCallback(boost::function<std::string (const std::string&)> fileCallback)
            : fileCallback(fileCallback)
        {
        }

        virtual HRESULT STDMETHODCALLTYPE Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes)
        {
            std::string path;

            if (pParentData)
            {
                RBXASSERT(paths.count(pParentData));

                path = paths[pParentData];

                std::string::size_type slash = path.find_last_of("\\/");
                path.erase(path.begin() + (slash == std::string::npos ? 0 : slash + 1), path.end());
            }

            path += pFileName;

            try
            {
                std::string source = fileCallback(path);

                char* result = new char[source.length()];
                memcpy(result, source.c_str(), source.length());

                paths[result] = path;

                *ppData = result;
                *pBytes = source.length();

                return S_OK;
            }
            catch (...)
            {
                return ERROR_FILE_NOT_FOUND;
            }
        }

        virtual HRESULT STDMETHODCALLTYPE Close(LPCVOID pData)
        {
            RBXASSERT(paths.count(pData));
            paths.erase(pData);
            delete[] static_cast<const char*>(pData);

            return S_OK;
        }
    };

    template <typename T> static std::vector<T> consumeData(HRESULT hr, ID3DBlob* buffer, ID3DBlob* messages)
    {
        if (SUCCEEDED(hr))
        {
            if (messages)
            {
                std::string log(static_cast<char*>(messages->GetBufferPointer()), messages->GetBufferSize());

                messages->Release();

                FASTLOG(FLog::Graphics, "Shader compilation resulted in warnings:");

                ShaderProgram::dumpToFLog(log.c_str(), FLog::Graphics);
            }

            RBXASSERT(buffer->GetBufferSize() % sizeof(T) == 0);
            std::vector<T> result(static_cast<T*>(buffer->GetBufferPointer()), static_cast<T*>(buffer->GetBufferPointer()) + buffer->GetBufferSize() / sizeof(T));

            buffer->Release();

            return result;
        }
        else if (messages)
        {
            std::string log(static_cast<char*>(messages->GetBufferPointer()), messages->GetBufferSize());

            messages->Release();

            throw std::runtime_error(log.c_str());
        }
        else
            throw RBX::runtime_error("Unknown error %x", hr);
    }

    std::string ShaderProgramD3D11::createShaderSource(const std::string& path, const std::string& defines, const DeviceD3D11* device11, boost::function<std::string (const std::string&)> fileCallback)
    {
        TypeD3DPreprocess D3DPreprocess = loadShaderPreprocessor();
        RBXASSERT(D3DPreprocess);

        // split define string into strings
        std::vector<std::string> defineStrings;

        std::istringstream defineStream(defines);
        std::string defineTemp;

        while (defineStream >> defineTemp)
            defineStrings.push_back(defineTemp);

        // create d3dx defines
        std::vector<D3D_SHADER_MACRO > macros;

        for (size_t i = 0; i < defineStrings.size(); ++i)
        {
            std::string& def = defineStrings[i];
            std::string::size_type pos = def.find('=');

            if (pos == std::string::npos)
            {
                D3D_SHADER_MACRO  macro = {def.c_str(), "1"};
                macros.push_back(macro);
            }
            else
            {
                // split string into name and value
                def[pos] = 0;

                D3D_SHADER_MACRO  macro = {def.c_str(), def.c_str() + pos + 1};
                macros.push_back(macro);
            }
        }

        D3D_SHADER_MACRO  macroDX11 = {"DX11", "1"};
        macros.push_back(macroDX11);
        if (device11->getShaderProfile() == DeviceD3D11::shaderProfile_DX11_level_9_3)
        {
            D3D_SHADER_MACRO  macroWinMobile= {"WIN_MOBILE", "1"};
            macros.push_back(macroWinMobile);
        }
        D3D_SHADER_MACRO  macroEnd = {};
        macros.push_back(macroEnd);

        std::string sourceFolder = "";

        // let preprocessor know about the original filename
        std::string source = "#include \"" + path + "\"\n";

        IncludeCallback includeCallBack = IncludeCallback(fileCallback);

        ID3DBlob* text;
        ID3DBlob* messages;
        HRESULT hr = D3DPreprocess(source.c_str(), source.size(), path.c_str(), &macros[0], &includeCallBack, &text, &messages);

        std::vector<char> resultBuffer = consumeData<char>(hr, text, messages);
        std::string result = &resultBuffer[0];

        // preprocessor output includes a #line 1 "<full-path>\memory"; remove it!
        if (result.size() > 10 && result.compare(0, 9, "#line 1 \"") == 0 && result[10] == ':')
        {
            std::string::size_type pos = result.find_first_of('\n');
            assert(pos != std::string::npos);

            result.erase(result.begin(), result.begin() + pos);
        }

        return result;
    }

    static void translateShaderProfile(const std::string& originalTarget, DeviceD3D11::ShaderProfile shaderProfile, std::string& targetOut)
    {
        switch (shaderProfile)
        {
            
        case DeviceD3D11::shaderProfile_DX11:
            {
                std::string shaderType = originalTarget.substr(0, 2);
                targetOut = shaderType + "_5_0";
                return;
            }
        case DeviceD3D11::shaderProfile_DX11_level_9_3:
            {
                std::string shaderType = originalTarget.substr(0, 2);
                targetOut = shaderType + "_4_0_level_9_3";
                return;
            }
        default:
            break;
        }
    }

    bool needsBackwardCompatibility(const std::string& target)
    {
        char reqShaderProfile = target[3];

        // our shaders are written in DX9 target and lower. Backwards compatibility is needed for newer targets and doesn't
        // hurt when shader is written in DX10 or DX11 target, then it doesn't do anything
        return reqShaderProfile >= '4';
    }

    std::vector<char> ShaderProgramD3D11::createShaderBytecode(const std::string& source, const std::string& target, const DeviceD3D11* device11, const std::string& entrypoint)
    {
        TypeD3DCompile D3DCompile = loadShaderCompiler();
        RBXASSERT(D3DCompile);

        unsigned int flags = D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

        std::string realTarget;
        translateShaderProfile(target, device11->getShaderProfile(), realTarget);

        if (needsBackwardCompatibility(realTarget))
            flags |= D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY;

        ID3DBlob* bytecode = NULL;
        ID3DBlob* messages = NULL;
        HRESULT hr = D3DCompile(source.c_str(), source.length(), entrypoint.c_str(), NULL, NULL, entrypoint.c_str(), realTarget.c_str(), flags, 0, &bytecode, &messages);

        return consumeData<char>(hr, bytecode, messages);
    }

#if !defined(RBX_PLATFORM_DURANGO)
    HMODULE ShaderProgramD3D11::loadShaderCompilerDLL()
    {
        static HMODULE compiler;

        if (!compiler)
            compiler = LoadLibraryA("D3DCompiler_47.dll");

        return compiler;
    }
#endif

    ID3D11InputLayout* ShaderProgramD3D11::getInputLayout11(VertexLayoutD3D11* vertexLayout)
    {
        return static_cast<VertexShaderD3D11*>(vertexShader.get())->getInputLayout11(vertexLayout);
    }

    unsigned int ShaderProgramD3D11::getMaxWorldTransforms() const
    {
        return static_cast<VertexShaderD3D11*>(vertexShader.get())->getMaxWorldTransforms();
    }

    void ShaderProgramD3D11::setWorldTransforms4x3(const float* data, size_t matrixCount)
    {
        static_cast<VertexShaderD3D11*>(vertexShader.get())->setWorldTransforms4x3(data, matrixCount);
    }

    int ShaderProgramD3D11::getConstantHandle(const char* name) const
    {
        int vs = static_cast<VertexShaderD3D11*>(vertexShader.get())->findUniform(name);
        int fs = static_cast<FragmentShaderD3D11*>(fragmentShader.get())->findUniform(name);

        RBXASSERT(vs >= -1 && fs >= -1);

         if (vs < 0 && fs < 0)
            return -1;
        else
            return (vs + 1) | ((fs + 1) << 16);
    }

    void ShaderProgramD3D11::setConstant(int handle, const float* data, size_t vectorCount)
    {
        if (handle < 0)
            return;

        int vs = (handle & 0xffff) - 1;
        int fs = (handle >> 16) - 1;

        if (vs >= 0)
        {
            static_cast<VertexShaderD3D11*>(vertexShader.get())->setConstant(vs, data, vectorCount);
        }

        if (fs >= 0)
        {
            static_cast<FragmentShaderD3D11*>(fragmentShader.get())->setConstant(fs, data, vectorCount);
        }
    }

    void ShaderProgramD3D11::uploadConstantBuffers()
    {
        static_cast<VertexShaderD3D11*>(vertexShader.get())->updateConstantBuffers();
        static_cast<FragmentShaderD3D11*>(fragmentShader.get())->updateConstantBuffers();
    }

    unsigned int ShaderProgramD3D11::getSamplerMask() const
    {
        return static_cast<FragmentShaderD3D11*>(fragmentShader.get())->getSamplerMask();
    }

    void ShaderProgramD3D11::bind()
    {
        ID3D11DeviceContext* context11 = static_cast<DeviceD3D11*>(device)->getImmediateContext11();
        VertexShaderD3D11* vs = static_cast<VertexShaderD3D11*>(vertexShader.get());
        FragmentShaderD3D11* fs = static_cast<FragmentShaderD3D11*>(fragmentShader.get());

		for (auto& cb: vs->getCBuffers())
		{
			ID3D11Buffer* buffer = cb->getObject();
			context11->VSSetConstantBuffers(cb->getRegisterId(), 1, &buffer);
		}

		for (auto& cb: fs->getCBuffers())
		{
			ID3D11Buffer* buffer = cb->getObject();
			context11->PSSetConstantBuffers(cb->getRegisterId(), 1, &buffer);
		}

        context11->VSSetShader(vs->getObject(), NULL, 0);
        context11->PSSetShader(fs->getObject(), NULL, 0);
    }
}
}
