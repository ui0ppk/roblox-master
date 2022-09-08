#include "ShaderD3D9.h"

#include "GeometryD3D9.h"
#include "DeviceD3D9.h"

#include <d3d9.h>
#include <d3dx9.h>

#include <sstream>
#include <map>

#pragma comment(lib, "d3dx9.lib")

LOGGROUP(Graphics)

namespace RBX
{
namespace Graphics
{

static void extractUniforms(ID3DXConstantTable* ctab, const std::string& prefix, D3DXHANDLE container, unsigned int size, std::vector<UniformD3D9>& result, unsigned int& samplerMask)
{
	for (unsigned int i = 0; i < size; ++i)
	{
		D3DXHANDLE constant = ctab->GetConstant(container, i);

		D3DXCONSTANT_DESC cdesc;
        unsigned int cdescCount = 1;
		ctab->GetConstantDesc(constant, &cdesc, &cdescCount);

        // Trim leading $ (for function-local uniforms)
		const char* name = cdesc.Name + (cdesc.Name[0] == '$');

		if (cdesc.Class == D3DXPC_STRUCT)
		{
			extractUniforms(ctab, prefix + name + ".", constant, cdesc.StructMembers, result, samplerMask);
		}
		else if (cdesc.RegisterSet == D3DXRS_SAMPLER)
		{
            samplerMask |= 1 << cdesc.RegisterIndex;
		}
		else if (cdesc.RegisterCount > 0)
        {
            UniformD3D9 u;

            u.name = prefix + name;
            u.registerSet = (cdesc.RegisterSet == D3DXRS_BOOL) ? UniformD3D9::Register_Bool : (cdesc.RegisterSet == D3DXRS_INT4) ? UniformD3D9::Register_Int : UniformD3D9::Register_Float;
            u.registerIndex = cdesc.RegisterIndex;
            u.registerCount = cdesc.RegisterCount;

            result.push_back(u);
        }
	}
}

static std::vector<UniformD3D9> extractUniforms(ID3DXConstantTable* ctab, unsigned int* outSamplerMask)
{
	std::vector<UniformD3D9> result;
    unsigned int samplerMask = 0;

    D3DXCONSTANTTABLE_DESC desc;
	ctab->GetDesc(&desc);

	extractUniforms(ctab, "", NULL, desc.Constants, result, samplerMask);

    if (outSamplerMask)
        *outSamplerMask = samplerMask; 

    return result;
}

static std::vector<UniformD3D9> extractUniforms(const DWORD* bytecodePointer, unsigned int* outSamplerMask)
{
	ID3DXConstantTable* ctab;
    HRESULT hr = D3DXGetShaderConstantTable(bytecodePointer, &ctab);

	if (SUCCEEDED(hr))
	{
        if (!ctab)
            throw RBX::runtime_error("Error parsing bytecode: no constant table present");

		std::vector<UniformD3D9> result = extractUniforms(ctab, outSamplerMask);

		ctab->Release();

        return result;
	}
	else
		throw RBX::runtime_error("Error parsing bytecode: %x", hr);
}

static void disableUniform(UniformD3D9& u)
{
    u.name.clear();
}

struct UniformDisabledPredicate
{
	bool operator()(const UniformD3D9& u) const
	{
		return u.name.empty();
	}
};

static void validateUniformsAndDisableGlobal(std::vector<UniformD3D9>& uniforms, size_t globalDataSize, const std::map<std::string, ShaderGlobalConstant>& globalConstants)
{
    size_t globalDataRegisterCount = globalDataSize / 16;

    for (size_t i = 0; i < uniforms.size(); ++i)
	{
        UniformD3D9& u = uniforms[i];

		if (u.registerSet != UniformD3D9::Register_Float)
			throw RBX::runtime_error("Uniform %s has unsupported register set %d", u.name.c_str(), u.registerSet);

		if (u.registerIndex + u.registerCount <= globalDataRegisterCount)
		{
			std::map<std::string, ShaderGlobalConstant>::const_iterator git = globalConstants.find(u.name);

			if (git == globalConstants.end())
				throw RBX::runtime_error("Uniform %s register range [%d..%d) overlaps with global register range [0..%d)",
                    u.name.c_str(), u.registerIndex, u.registerIndex + u.registerCount, globalDataRegisterCount);

			unsigned int expectedRegisterIndex = git->second.offset / 16;
			unsigned int expectedRegisterCount = git->second.size / 16;

			if (u.registerIndex != expectedRegisterIndex || u.registerCount > expectedRegisterCount)
				throw RBX::runtime_error("Uniform %s register range [%d..%d) is not a prefix of expected register range [%d..%d)",
                    u.name.c_str(), u.registerIndex, u.registerIndex + u.registerCount, expectedRegisterIndex, expectedRegisterIndex + expectedRegisterCount);

			disableUniform(u);
		}
	}
}

static void removeDisabledUniforms(std::vector<UniformD3D9>& uniforms)
{
	uniforms.erase(std::remove_if(uniforms.begin(), uniforms.end(), UniformDisabledPredicate()), uniforms.end());
}

static int findUniform(const std::vector<UniformD3D9>& uniforms, const char* name)
{
    for (size_t i = 0; i < uniforms.size(); ++i)
		if (uniforms[i].name == name)
            return i;

    return -1;
}

static void reloadUniforms(std::vector<UniformD3D9>& uniforms, const std::vector<UniformD3D9>& newUniforms)
{
    for (size_t i = 0; i < uniforms.size(); ++i)
	{
		int newIndex = findUniform(newUniforms, uniforms[i].name.c_str());

        if (newIndex >= 0)
		{
			uniforms[i] = newUniforms[newIndex];
		}
		else
		{
			uniforms[i].registerIndex = 0;
			uniforms[i].registerCount = 0;
		}
	}
}

static IDirect3DVertexShader9* createVertexShader(Device* device, const std::vector<char>& bytecode,
    std::vector<UniformD3D9>& uniforms, int& registerWorldMatrix, int& registerWorldMatrixArray, unsigned int& maxWorldTransforms)
{
    RBXASSERT(bytecode.size() > 0 && bytecode.size() % sizeof(DWORD) == 0);
	const DWORD* bytecodePointer = reinterpret_cast<const DWORD*>(&bytecode[0]);

	uniforms = extractUniforms(bytecodePointer, NULL);

	validateUniformsAndDisableGlobal(uniforms, static_cast<DeviceD3D9*>(device)->getGlobalDataSize(), static_cast<DeviceD3D9*>(device)->getGlobalConstants());

	int uniformWorldMatrix = findUniform(uniforms, "WorldMatrix");

	if (uniformWorldMatrix >= 0)
	{
		UniformD3D9& u = uniforms[uniformWorldMatrix];

		registerWorldMatrix = u.registerIndex;
		maxWorldTransforms = 1;

		disableUniform(u);
	}

	int uniformWorldMatrixArray = findUniform(uniforms, "WorldMatrixArray");

	if (uniformWorldMatrixArray >= 0)
	{
		UniformD3D9& u = uniforms[uniformWorldMatrixArray];

		RBXASSERT(u.registerCount % 3 == 0);

		registerWorldMatrixArray = u.registerIndex;
		maxWorldTransforms = u.registerCount / 3;

		disableUniform(u);
	}

	removeDisabledUniforms(uniforms);

	IDirect3DDevice9* device9 = static_cast<DeviceD3D9*>(device)->getDevice9();

	IDirect3DVertexShader9* object = NULL;
	HRESULT hr = device9->CreateVertexShader(bytecodePointer, &object);
    if (FAILED(hr))
		throw RBX::runtime_error("Error creating vertex shader: %x", hr);

    return object;
}

static IDirect3DPixelShader9* createPixelShader(Device* device, const std::vector<char>& bytecode,
    std::vector<UniformD3D9>& uniforms, unsigned int& samplerMask)
{
    RBXASSERT(bytecode.size() > 0 && bytecode.size() % sizeof(DWORD) == 0);
	const DWORD* bytecodePointer = reinterpret_cast<const DWORD*>(&bytecode[0]);

	uniforms = extractUniforms(bytecodePointer, &samplerMask);

	validateUniformsAndDisableGlobal(uniforms, static_cast<DeviceD3D9*>(device)->getGlobalDataSize(), static_cast<DeviceD3D9*>(device)->getGlobalConstants());
	removeDisabledUniforms(uniforms);

	IDirect3DDevice9* device9 = static_cast<DeviceD3D9*>(device)->getDevice9();

	IDirect3DPixelShader9* object = NULL;
	HRESULT hr = device9->CreatePixelShader(bytecodePointer, &object);
    if (FAILED(hr))
		throw RBX::runtime_error("Error creating fragment shader: %x", hr);

    return object;
}

VertexShaderD3D9::VertexShaderD3D9(Device* device, const std::vector<char>& bytecode)
	: VertexShader(device)
    , object(NULL)
    , registerWorldMatrix(-1)
    , registerWorldMatrixArray(-1)
	, maxWorldTransforms(0)
{
	object = createVertexShader(device, bytecode, uniforms, registerWorldMatrix, registerWorldMatrixArray, maxWorldTransforms);
}

VertexShaderD3D9::~VertexShaderD3D9()
{
	object->Release();
}

void VertexShaderD3D9::reloadBytecode(const std::vector<char>& bytecode)
{
	std::vector<UniformD3D9> newUniforms;
    int newRegisterWorldMatrix = -1;
    int newRegisterWorldMatrixArray = -1;
	unsigned int newMaxWorldTransforms = 0;

	IDirect3DVertexShader9* newObject = createVertexShader(device, bytecode, newUniforms, newRegisterWorldMatrix, newRegisterWorldMatrixArray, newMaxWorldTransforms);
    if (object)
        object->Release();

    object = newObject;
	registerWorldMatrix = newRegisterWorldMatrix;
	registerWorldMatrixArray = newRegisterWorldMatrixArray;
	maxWorldTransforms = newMaxWorldTransforms;
    reloadUniforms(uniforms, newUniforms);
}

FragmentShaderD3D9::FragmentShaderD3D9(Device* device, const std::vector<char>& bytecode)
	: FragmentShader(device)
    , object(NULL)
    , samplerMask(0)
{
	object = createPixelShader(device, bytecode, uniforms, samplerMask);
}

FragmentShaderD3D9::~FragmentShaderD3D9()
{
	object->Release();
}

void FragmentShaderD3D9::reloadBytecode(const std::vector<char>& bytecode)
{
	std::vector<UniformD3D9> newUniforms;
	unsigned int newSamplerMask = 0;

	IDirect3DPixelShader9* newObject = createPixelShader(device, bytecode, newUniforms, newSamplerMask);
    if (object)
        object->Release();

    object = newObject;
	samplerMask = newSamplerMask;
    reloadUniforms(uniforms, newUniforms);
}

ShaderProgramD3D9::ShaderProgramD3D9(Device* device, const shared_ptr<VertexShader>& vertexShader, const shared_ptr<FragmentShader>& fragmentShader)
	: ShaderProgram(device, vertexShader, fragmentShader)
{
}

ShaderProgramD3D9::~ShaderProgramD3D9()
{
	static_cast<DeviceD3D9*>(device)->getImmediateContextD3D9()->invalidateCachedProgram();
}

void ShaderProgramD3D9::bind()
{
	IDirect3DDevice9* device9 = static_cast<DeviceD3D9*>(device)->getDevice9();

	device9->SetVertexShader(static_cast<VertexShaderD3D9*>(vertexShader.get())->getObject());
	device9->SetPixelShader(static_cast<FragmentShaderD3D9*>(fragmentShader.get())->getObject());
}

void ShaderProgramD3D9::setWorldTransforms4x3(const float* data, size_t matrixCount)
{
    if (matrixCount == 0)
        return;

	IDirect3DDevice9* device9 = static_cast<DeviceD3D9*>(device)->getDevice9();
	VertexShaderD3D9* vs9 = static_cast<VertexShaderD3D9*>(vertexShader.get());

	if (vs9->getRegisterWorldMatrix() >= 0)
	{
        static const float lastColumn[] = {0, 0, 0, 1};

        float matrix[16];
        memcpy(matrix, data, 12 * sizeof(float));
        memcpy(matrix + 12, lastColumn, 4 * sizeof(float));

		device9->SetVertexShaderConstantF(vs9->getRegisterWorldMatrix(), matrix, 4);
	}

	if (vs9->getRegisterWorldMatrixArray() >= 0)
	{
        RBXASSERT(matrixCount <= vs9->getMaxWorldTransforms());
    
		device9->SetVertexShaderConstantF(vs9->getRegisterWorldMatrixArray(), data, matrixCount * 3);
	}
}

void ShaderProgramD3D9::setConstant(int handle, const float* data, size_t vectorCount)
{
    if (handle < 0)
        return;

	IDirect3DDevice9* device9 = static_cast<DeviceD3D9*>(device)->getDevice9();

    int vs = (handle & 0xffff) - 1;
    int fs = (handle >> 16) - 1;

    if (vs >= 0)
	{
        const std::vector<UniformD3D9>& uniforms = static_cast<VertexShaderD3D9*>(vertexShader.get())->getUniforms();

        RBXASSERT(static_cast<unsigned int>(vs) < uniforms.size());
		const UniformD3D9& u = uniforms[vs];

		if (u.registerCount)
		{
            RBXASSERT(vectorCount >= u.registerCount);

			device9->SetVertexShaderConstantF(u.registerIndex, data, u.registerCount);
		}
	}

    if (fs >= 0)
	{
        const std::vector<UniformD3D9>& uniforms = static_cast<FragmentShaderD3D9*>(fragmentShader.get())->getUniforms();

        RBXASSERT(static_cast<unsigned int>(fs) < uniforms.size());
		const UniformD3D9& u = uniforms[fs];

		if (u.registerCount)
		{
            RBXASSERT(vectorCount >= u.registerCount);

			device9->SetPixelShaderConstantF(u.registerIndex, data, u.registerCount);
		}
	}
}

int ShaderProgramD3D9::getConstantHandle(const char* name) const
{
	int vs = findUniform(static_cast<VertexShaderD3D9*>(vertexShader.get())->getUniforms(), name);
	int fs = findUniform(static_cast<FragmentShaderD3D9*>(fragmentShader.get())->getUniforms(), name);

    RBXASSERT(vs >= -1 && fs >= -1);

    if (vs < 0 && fs < 0)
        return -1;
	else
        return (vs + 1) | ((fs + 1) << 16);
}

unsigned int ShaderProgramD3D9::getMaxWorldTransforms() const
{
	return static_cast<VertexShaderD3D9*>(vertexShader.get())->getMaxWorldTransforms();
}

unsigned int ShaderProgramD3D9::getSamplerMask() const
{
	return static_cast<FragmentShaderD3D9*>(fragmentShader.get())->getSamplerMask();
}

struct IncludeCallback: ID3DXInclude
{
	boost::function<std::string (const std::string&)> fileCallback;
    std::map<const void*, std::string> paths;
	
    IncludeCallback(boost::function<std::string (const std::string&)> fileCallback)
		: fileCallback(fileCallback)
	{
	}

	virtual HRESULT STDMETHODCALLTYPE Open(D3DXINCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes)
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
			return D3DXFERR_FILENOTFOUND;
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

typedef HRESULT (WINAPI *TypeD3DXCompileShader)(LPCSTR, UINT, CONST D3DXMACRO*, LPD3DXINCLUDE, LPCSTR, LPCSTR, DWORD, LPD3DXBUFFER*, LPD3DXBUFFER*, LPD3DXCONSTANTTABLE*);
typedef HRESULT (WINAPI *TypeD3DXPreprocessShader)(LPCSTR, UINT, CONST D3DXMACRO*, LPD3DXINCLUDE, LPD3DXBUFFER*, LPD3DXBUFFER*);

static HMODULE loadShaderCompilerDLL()
{
    static HMODULE d3dx;

    if (!d3dx)
	{
        for (int version = 43; version >= 24; --version)
        {
            d3dx = LoadLibraryA(format("d3dx9_%d.dll", version).c_str());

            if (d3dx)
            {
                LoadLibraryA(format("D3DCompiler_%d.dll", version).c_str());

                break;
            }
        }
	}

    return d3dx;
}

static TypeD3DXCompileShader loadShaderCompiler()
{
	HMODULE d3dx = loadShaderCompilerDLL();

    return d3dx ? (TypeD3DXCompileShader)GetProcAddress(d3dx, "D3DXCompileShader") : NULL;
}

static TypeD3DXPreprocessShader loadShaderPreprocessor()
{
	HMODULE d3dx = loadShaderCompilerDLL();

    return d3dx ? (TypeD3DXPreprocessShader)GetProcAddress(d3dx, "D3DXPreprocessShader") : NULL;
}

template <typename T> static std::vector<T> consumeData(HRESULT hr, ID3DXBuffer* buffer, ID3DXBuffer* messages)
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

std::string ShaderProgramD3D9::createShaderSource(const std::string& path, const std::string& defines, boost::function<std::string (const std::string&)> fileCallback)
{
	TypeD3DXPreprocessShader preprocessor = loadShaderPreprocessor();

    if (!preprocessor)
		throw RBX::runtime_error("Unable to load shader preprocessor");

    // split define string into strings
    std::vector<std::string> defineStrings;

	std::istringstream defineStream(defines);
    std::string defineTemp;

	while (defineStream >> defineTemp)
		defineStrings.push_back(defineTemp);

	// create d3dx defines
	std::vector<D3DXMACRO> macros;
	
	for (size_t i = 0; i < defineStrings.size(); ++i)
	{
		std::string& def = defineStrings[i];
		std::string::size_type pos = def.find('=');
		
		if (pos == std::string::npos)
		{
			D3DXMACRO macro = {def.c_str(), "1"};
			macros.push_back(macro);
		}
		else
		{
            // split string into name and value
			def[pos] = 0;
			
			D3DXMACRO macro = {def.c_str(), def.c_str() + pos + 1};
			macros.push_back(macro);
		}
	}

	D3DXMACRO macroEnd = {};
	macros.push_back(macroEnd);

    // let preprocessor know about the original filename
	std::string source = "#include \"" + path + "\"\n";

	// preprocess shader
	IncludeCallback callback(fileCallback);

	ID3DXBuffer* text = NULL;
    ID3DXBuffer* messages = NULL;
	HRESULT hr = preprocessor(source.c_str(), source.length(), &macros[0], &callback, &text, &messages);

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

std::vector<char> ShaderProgramD3D9::createShaderBytecode(const std::string& source, const std::string& target, const std::string& entrypoint)
{
	TypeD3DXCompileShader compiler = loadShaderCompiler();

    if (!compiler)
		throw RBX::runtime_error("Unable to load shader compiler");

    unsigned int flags = D3DXSHADER_PACKMATRIX_ROWMAJOR;

	ID3DXBuffer* bytecode = NULL;
	ID3DXBuffer* messages = NULL;
	HRESULT hr = compiler(source.c_str(), source.length(), NULL, NULL, entrypoint.c_str(), target.c_str(), flags, &bytecode, &messages, NULL);

    return consumeData<char>(hr, bytecode, messages);
}

ShaderProgramFFPD3D9::ShaderProgramFFPD3D9(Device* device)
	: ShaderProgram(device, shared_ptr<VertexShader>(), shared_ptr<FragmentShader>())
{
}

ShaderProgramFFPD3D9::~ShaderProgramFFPD3D9()
{
}

int ShaderProgramFFPD3D9::getConstantHandle(const char* name) const
{
    if (strcmp(name, "Color") == 0)
        return 0;

    return -1;
}

unsigned int ShaderProgramFFPD3D9::getMaxWorldTransforms() const
{
    return 1;
}

unsigned int ShaderProgramFFPD3D9::getSamplerMask() const
{
    return 0;
}

}
}
