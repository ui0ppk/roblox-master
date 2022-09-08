// Force asserts in this file
#ifdef NDEBUG
#undef NDEBUG
#endif

#include "mojoshader/mojoshader.h"
#include "hlsl2glslfork/include/hlsl2glsl.h"
#include "glsl-optimizer/src/glsl/glsl_optimizer.h"
#include "md5/md5.h"

#ifdef _WIN32
#include <d3d11.h>
#include <D3Dcompiler.h>
#endif

#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <fstream>

#include <assert.h>

#include "rapidjson/document.h"

#define error(...) (fprintf(stderr, __VA_ARGS__), 1)

std::string readFile(const std::string& path)
{
	std::ifstream in(path.c_str(), std::ios::in | std::ios::binary);
	if (!in)
		throw std::runtime_error("Can't read file: " + path);
	
    std::string result;

	while (in)
	{
        char buf[4096];

		in.read(buf, sizeof(buf));
		result.insert(result.end(), buf, buf + in.gcount());
	}
	
	return result;
}

std::vector<std::string> split(const std::string& str)
{
	std::vector<std::string> result;

	std::istringstream iss(str);
    std::string item;
    while (iss >> item)
		result.push_back(item);

    return result;
}

class IShaderCompiler
{
public:
	virtual ~IShaderCompiler() {}

    virtual std::string preprocess(const std::string& folder, const std::string& path, const std::string& defines) = 0;
    virtual std::vector<char> compile(const std::string& source, const std::string& target, const std::string& entry, bool optimize) = 0;
};

class ShaderCompilerHLSL2GLSL: public IShaderCompiler
{
public:
	ShaderCompilerHLSL2GLSL(const std::string& language)
		: language(language)
	{
        int rc = Hlsl2Glsl_Initialize();
        assert(rc);
	}

	~ShaderCompilerHLSL2GLSL()
	{
		Hlsl2Glsl_Shutdown();
	}

    virtual std::string preprocess(const std::string& folder, const std::string& path, const std::string& defines)
	{
        // read file into memory
        std::string source = readFile(folder + "/" + path);
            
        // create mojoshader defines
        std::vector<std::string> defineStrings = split(defines);
        std::vector<MOJOSHADER_preprocessorDefine> macros;
        
        for (size_t i = 0; i < defineStrings.size(); ++i)
        {
            std::string& def = defineStrings[i];
            std::string::size_type pos = def.find('=');
            
            if (pos == std::string::npos)
            {
                MOJOSHADER_preprocessorDefine mdef = {def.c_str(), "1"};
                macros.push_back(mdef);
            }
            else
            {
                // split string into name and value
                def[pos] = 0;
                
                MOJOSHADER_preprocessorDefine mdef = {def.c_str(), def.c_str() + pos + 1};
                macros.push_back(mdef);
            }
        }
            
        // preprocess shader
		IncludeHandler handler(folder);

        const MOJOSHADER_preprocessData* pp = MOJOSHADER_preprocess(
            path.c_str(), source.c_str(), source.size(), macros.empty() ? NULL : &macros[0], macros.size(),
			IncludeHandler::open, IncludeHandler::close, IncludeHandler::resolve, MOJOSHADER_PREPROCESS_EMITLINE, NULL, NULL, &handler);
        
        if (pp->error_count)
        {
			std::ostringstream oss;
            
            for (int i = 0; i < pp->error_count; ++i)
                oss << pp->errors[i].filename << "(" << pp->errors[i].error_position << "): " << pp->errors[i].error << std::endl;
            
            MOJOSHADER_freePreprocessData(pp);
            
			throw std::runtime_error(oss.str());
        }
        
        std::string result(pp->output, pp->output_len);
        
        MOJOSHADER_freePreprocessData(pp);
        
        return result;
	}

    virtual std::vector<char> compile(const std::string& source, const std::string& target, const std::string& entry, bool optimize)
	{
        bool isVertexShader = target[0] == 'v';

		ETargetVersion trnTarget = language == "glsles" ? ETargetGLSL_ES_100 : language == "glsles3" ? ETargetGLSL_ES_300 : language == "glsl3" ? ETargetGLSL_140 : ETargetGLSL_110;
		glslopt_target optTarget = language == "glsles" ? kGlslTargetOpenGLES20 : language == "glsles3" ? kGlslTargetOpenGLES30 : language == "metal" ? kGlslTargetMetal : kGlslTargetOpenGL;

		std::pair<std::string, std::string> p = translateHLSL(source, entry, trnTarget, isVertexShader ? EShLangVertex : EShLangFragment);

        if (optimize)
		{
			std::string optimized = optimizeGLSL(p.first, optTarget, isVertexShader ? kGlslOptShaderVertex : kGlslOptShaderFragment);
            std::string result = optimized + p.second;

            return std::vector<char>(result.begin(), result.end());
		}
		else
		{
            std::string result = p.first + p.second;

            return std::vector<char>(result.begin(), result.end());
		}
	}

private:
    struct IncludeHandler
    {
        std::string folder;

        IncludeHandler(const std::string& folder)
			: folder(folder)
		{
		}

        static int open(MOJOSHADER_includeType inctype, const char *fname, const char *parent, const char **outdata, unsigned int *outbytes, MOJOSHADER_malloc m, MOJOSHADER_free f, void *d)
        {
            IncludeHandler* self = static_cast<IncludeHandler*>(d);

            try
            {
				std::string source = readFile(self->folder + "/" + fname);

                char* result = new char[source.length()];
                memcpy(result, source.c_str(), source.length());

                *outdata = result;
                *outbytes = source.length();

                return 1;
            }
            catch (...)
            {
                return 0;
            }
        }

        static void close(const char *data, MOJOSHADER_malloc m, MOJOSHADER_free f, void *d)
        {
            delete[] data;
        }

        static const char* resolve(MOJOSHADER_includeType inctype, const char *fname, const char *parent, const char* (*duplicate)(void*, const char*), void* duplicateContext)
        {
            std::string path = parent;
            
            std::string::size_type slash = path.find_last_of("\\/");
            path.erase(path.begin() + (slash == std::string::npos ? 0 : slash + 1), path.end());

            path += fname;
            
            return duplicate(duplicateContext, path.c_str());
        }
    };

    static void setupVertexAttributeInputs(ShHandle parser)
    {
        // Maps vertex streams to OGRE streams
        static const struct { EAttribSemantic key; const char* value; } mapping[] =
        {
            { EAttrSemPosition, "vertex" },
            { EAttrSemBlendWeight, "blendWeights" },
            { EAttrSemNormal, "normal" },
            { EAttrSemColor0, "colour" },
            { EAttrSemColor1, "secondary_colour" },
            { EAttrSemBlendIndices, "blendIndices" },
            { EAttrSemTex0, "uv0" },
            { EAttrSemTex1, "uv1" },
            { EAttrSemTex2, "uv2" },
            { EAttrSemTex3, "uv3" },
            { EAttrSemTex4, "uv4" },
            { EAttrSemTex5, "uv5" },
            { EAttrSemTex6, "uv6" },
            { EAttrSemTex7, "uv7" },
            { EAttrSemTangent, "tangent" },
            { EAttrSemBinormal, "binormal" },
        };
        
        // Setup mappings
        EAttribSemantic keys[sizeof(mapping) / sizeof(mapping[0])];
        const char* values[sizeof(mapping) / sizeof(mapping[0])];
        
        for (size_t i = 0; i < sizeof(mapping) / sizeof(mapping[0]); ++i)
        {
            keys[i] = mapping[i].key;
            values[i] = mapping[i].value;
        }
        
        Hlsl2Glsl_SetUserAttributeNames(parser, keys, values, sizeof(mapping) / sizeof(mapping[0]));
    }

    static bool isSamplerType(EShType type)
    {
        return 
            type == EShTypeSampler ||
            type == EShTypeSampler1D ||
            type == EShTypeSampler1DShadow ||
            type == EShTypeSampler2D ||
            type == EShTypeSampler2DShadow ||
            type == EShTypeSampler3D ||
            type == EShTypeSamplerCube ||
            type == EShTypeSamplerRect ||
            type == EShTypeSamplerRectShadow;
    }

    static std::string getSamplerInfo(ShHandle parser, const std::string& source)
    {
        const ShUniformInfo* uniforms = Hlsl2Glsl_GetUniformInfo(parser);
        int uniformCount = Hlsl2Glsl_GetUniformCount(parser);

        std::map<std::string, std::string> utab;

        for (int i = 0; i < uniformCount; ++i)
        {
            const ShUniformInfo& u = uniforms[i];

            if (isSamplerType(u.type))
            {
                if (!u.registerSpec)
					throw std::runtime_error(std::string("Sampler ") + u.name + " does not have a register specifier");

                std::string& target = utab[u.registerSpec];

                if (!target.empty())
					throw std::runtime_error(std::string("Samplers ") + u.name + " and " + target + " both use register " + u.registerSpec);

                target = u.name;
            }
        }

        std::string result;

        for (std::map<std::string, std::string>::iterator it = utab.begin(); it != utab.end(); ++it)
        {
            result += "//$$";
            result += it->second;
            result += "=";
            result += it->first;
            result += "\n";
        }

        return result;
    }

    struct ShHandleRAII
	{
        ShHandleRAII(ShHandle handle): handle(handle)
		{
		}

        ~ShHandleRAII()
		{
            Hlsl2Glsl_DestructCompiler(handle);
		}

        ShHandle handle;
	};

	static std::pair<std::string, std::string> translateHLSL(const std::string& source, const std::string& entry, ETargetVersion version, EShLanguage shader_type)
    {
        ShHandleRAII parser = Hlsl2Glsl_ConstructCompiler(shader_type);
        
        if (Hlsl2Glsl_Parse(parser.handle, source.c_str(), version, NULL, 0))
        {
            setupVertexAttributeInputs(parser.handle);
            
            if (Hlsl2Glsl_Translate(parser.handle, entry.c_str(), version, 0))
            {
                std::string result = Hlsl2Glsl_GetShader(parser.handle);
                std::string samplers = getSamplerInfo(parser.handle, source);

				if (version == ETargetGLSL_ES_300)
				{
					result = "#version 300 es\n" + result;
				}

                if ((version == ETargetGLSL_140 || version == ETargetGLSL_ES_300) && shader_type == EShLangFragment)
                {
					std::string::size_type newline = result.find('\n');

					if (newline != std::string::npos)
					{
						// Insert gl_FragData definition after #version
						std::string precision = (version == ETargetGLSL_ES_300) ? "lowp" : "";

						result =
							result.substr(0, newline + 1) +
							"#define gl_FragData _glFragData\n"
							"out " + precision + " vec4 _glFragData[4];\n" +
							result.substr(newline + 1);
					}
                }

                return std::make_pair(result, samplers);
            }
        }
        
        // error
        std::string err = Hlsl2Glsl_GetInfoLog(parser.handle);

		throw std::runtime_error(err);
    }

	static std::string optimizeGLSL(const std::string& source, glslopt_target target, glslopt_shader_type shader_type)
    {
        glslopt_ctx* ctx = glslopt_initialize(target);
        assert(ctx);

		glslopt_set_max_unroll_iterations(ctx, 64);
        
        glslopt_shader* shader = glslopt_optimize(ctx, shader_type, source.c_str(), 0);
        assert(shader);
        
        if (glslopt_get_status(shader))
        {
            std::string result = glslopt_get_output(shader);
            
            glslopt_shader_delete(shader);
            glslopt_cleanup(ctx);
            
            return result;
        }
        
        std::string err = glslopt_get_log(shader);
        
        glslopt_shader_delete(shader);
        glslopt_cleanup(ctx);
        
		throw std::runtime_error("Optimization failed:\n" + err);
    }

    std::string language;
};

#ifdef _WIN32
class ShaderCompilerD3D: public IShaderCompiler
{
public:

    enum ShaderProfile
    {
        shaderProfile_DX_9,
        shaderProfile_DX_11,
        shaderProfile_DX_11_DURANGO,
        shaderProfile_DX_11_9_3,
        shaderProfile_Count
    };

    static ShaderProfile getD3DProfile(const std::string& target)
    {
        if (target == "d3d9")
            return shaderProfile_DX_9;
        if (target == "d3d11")
            return shaderProfile_DX_11;
        if (target == "d3d11_level_9_3")
            return shaderProfile_DX_11_9_3;
        if (target == "d3d11_durango")
            return shaderProfile_DX_11_DURANGO;

        return shaderProfile_Count;
    }


    static bool isX86Profile(ShaderProfile profile)
    {
        static const ShaderProfile kX86Profiles[] = {shaderProfile_DX_9, shaderProfile_DX_11_9_3, shaderProfile_DX_11};

        for (int i = 0; i < ARRAYSIZE(kX86Profiles); ++i)
            if (profile == kX86Profiles[i])
                return true;

        return false;
    }

    static bool isX64Profile(ShaderProfile profile)
    {
        if (profile == shaderProfile_DX_11_DURANGO)
            return true;
        return false;
    }

	ShaderCompilerD3D(ShaderProfile shaderProfile)
        : shaderProfile(shaderProfile)
	{
        HMODULE d3dCompiler = NULL;
        
        if (shaderProfile == shaderProfile_DX_11_DURANGO)
            d3dCompiler = loadShaderCompilerDurangoDLL();
        else
            d3dCompiler = loadShaderCompilerDLL();

        compileShader = (TypeD3DCompile)GetProcAddress(d3dCompiler, "D3DCompile");
        assert(compileShader);

        preprocessShader = (TypeD3DPreprocess)GetProcAddress(d3dCompiler, "D3DPreprocess");
        assert(preprocessShader);
    }


    virtual std::string preprocess(const std::string& folder, const std::string& path, const std::string& defines)
    {
        // create d3dx defines
        std::vector<std::string> defineStrings = split(defines);
        std::vector<D3D_SHADER_MACRO> macros;
        
        for (size_t i = 0; i < defineStrings.size(); ++i)
        {
            std::string& def = defineStrings[i];
            std::string::size_type pos = def.find('=');
            
            if (pos == std::string::npos)
            {
                D3D_SHADER_MACRO macro = {def.c_str(), "1"};
                macros.push_back(macro);
            }
            else
            {
                // split string into name and value
                def[pos] = 0;
                
                D3D_SHADER_MACRO macro = {def.c_str(), def.c_str() + pos + 1};
                macros.push_back(macro);
            }
        }

        D3D_SHADER_MACRO macroEnd = {};
        macros.push_back(macroEnd);

        // let preprocessor know about the original filename
        std::string source = "#include \"" + path + "\"\n";

        // preprocess shader
        IncludeCallback callback(folder);

        ID3DBlob* text = NULL;
        ID3DBlob* messages = NULL;
        HRESULT hr = preprocessShader(source.c_str(), source.size(), path.c_str(), &macros[0], &callback, &text, &messages);

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

    bool needsBackwardCompatibility(const std::string& target)
    {
        char reqShaderProfile = (char)target[3];
        
        // our shaders are written in DX9 target and lower. Backwards compatibility is needed for newer targets and doesn't
        // hurt when shader is written in DX10 or DX11 target, then it doesn't do anything
        return reqShaderProfile >= '4';
    }

    void translateShaderProfile(const std::string& originalTarget, std::string& targetOut)
    {
        switch (shaderProfile)
        {
        case ShaderCompilerD3D::shaderProfile_DX_9:
            {
                targetOut = originalTarget;
                return;
            }
        case ShaderCompilerD3D::shaderProfile_DX_11_DURANGO:
        case ShaderCompilerD3D::shaderProfile_DX_11:
            {
                std::string shaderType = originalTarget.substr(0, 2);
                targetOut = shaderType + "_5_0";
                return;
            }
        case ShaderCompilerD3D::shaderProfile_DX_11_9_3:
            {
                std::string shaderType = originalTarget.substr(0, 2);
                targetOut = shaderType + "_4_0_level_9_3";
                return;
            }
        default:
            assert(false); // new enum?
            break;
        }
    }

    virtual std::vector<char> compile(const std::string& source, const std::string& target, const std::string& entry, bool optimize)
    {
        unsigned int flags = D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

        if (!optimize)
			flags |= D3DCOMPILE_SKIP_OPTIMIZATION;

        std::string realTarget;
        translateShaderProfile(target, realTarget);

        if (needsBackwardCompatibility(realTarget))
            flags |= D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY;

        ID3DBlob* bytecode = NULL;
        ID3DBlob* messages = NULL;
        //HRESULT hr = compileShader(source.c_str(), source.length(), NULL, NULL, entry.c_str(), target.c_str(), flags, &bytecode, &messages, NULL);
        HRESULT hr = compileShader(source.c_str(), source.length(), entry.c_str(), NULL, NULL, entry.c_str(), realTarget.c_str(), flags, 0, &bytecode, &messages);

        return consumeData<char>(hr, bytecode, messages);
    }

private:
    struct IncludeCallback: ID3DInclude
    {
        std::string folder;
        std::map<const void*, std::string> paths;

        IncludeCallback(const std::string& folder)
			: folder(folder)
		{
		}
        
        virtual HRESULT STDMETHODCALLTYPE Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes)
        {
            std::string path;

            if (pParentData)
            {
                assert(paths.count(pParentData));

                path = paths[pParentData];

                std::string::size_type slash = path.find_last_of("\\/");
                path.erase(path.begin() + (slash == std::string::npos ? 0 : slash + 1), path.end());
            }

            path += pFileName;

            try
            {
                std::string source = readFile(folder + "/" + path);

                char* result = new char[source.length()];
                memcpy(result, source.c_str(), source.length());

                paths[result] = path;

                *ppData = result;
                *pBytes = source.length();

                return S_OK;
            }
            catch (...)
            {
                return ERROR_FILE_NOT_FOUND; //  D3DERR_FILENOTFOUND;
            }
        }

        virtual HRESULT STDMETHODCALLTYPE Close(LPCVOID pData)
        {
            assert(paths.count(pData));
            paths.erase(pData);
            delete[] static_cast<const char*>(pData);

            return S_OK;
        }
    };

    static HMODULE loadShaderCompilerDLL()
    {
        HMODULE compiler = LoadLibraryA("d3dcompiler_47.dll");
        
        if (!compiler)
        {
            fprintf(stderr, "Error: can't load D3DCompile DLL\n");

            throw std::runtime_error("Can't load D3DX DLL");
        }

        return compiler;
    }

    static HMODULE loadShaderCompilerDurangoDLL()
    {
        HMODULE compiler = LoadLibraryA("D3DCompiler_47_xdk.dll");
        DWORD lastError = GetLastError();

        if (!compiler)
        {
            fprintf(stderr, "Error: can't load D3DCompile for xbox DLL\n");

            throw std::runtime_error("Can't load D3DX DLL");
        }

        return compiler;
    }

    template <typename T> static std::vector<T> consumeData(HRESULT hr, ID3DBlob* buffer, ID3DBlob* messages)
    {
        if (SUCCEEDED(hr))
        {
            if (messages)
            {
                std::string log(static_cast<char*>(messages->GetBufferPointer()), messages->GetBufferSize());

                messages->Release();

				fprintf(stderr, "Shader compilation resulted in warnings: %s", log.c_str());
            }

            assert(buffer->GetBufferSize() % sizeof(T) == 0);
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
		{
            char buf[128];
            sprintf(buf, "Unknown error %x", hr);

            throw std::runtime_error(buf);
		}
    }

    ShaderProfile shaderProfile;

    typedef HRESULT (WINAPI *TypeD3DCompile)(LPCVOID, SIZE_T, LPCSTR, const D3D_SHADER_MACRO *, ID3DInclude *, LPCSTR, LPCSTR, UINT, UINT, ID3DBlob **, ID3DBlob **);
    typedef HRESULT (WINAPI *TypeD3DPreprocess)(LPCVOID, SIZE_T, LPCSTR, const D3D_SHADER_MACRO *, ID3DInclude *, ID3DBlob **, ID3DBlob **);
    
	TypeD3DCompile compileShader;
	TypeD3DPreprocess preprocessShader;
};
#endif

int compileOne(int argc, char** argv)
{
	// parse command line
	std::string file;
	std::string target;
	std::string entry = "main";
	std::string defines = "GLSL";
	bool optimize = true;
	bool glsles = false;
	bool gl3 = false;
	bool metal = false;
	
	for (int i = 1; i < argc; ++i)
	{
		const char* arg = argv[i];
		
		if (arg[0] == '/')
		{
			if (arg[1] == 'D')
			{
				if (arg[2])
				{
					if (strcmp(arg + 2, "GLSLES") == 0)
						glsles = true;
					if (strcmp(arg + 2, "GL3") == 0)
						gl3 = true;
					if (strcmp(arg + 2, "METAL") == 0)
						metal = true;
						
					defines += " ";
					defines += arg + 2;
				}
				else
					return error("Error: empty defines are not permitted: %s\n", arg);
			}
			else if (arg[1] == 'T')
			{
				target = arg + 2;
			}
			else if (arg[1] == 'E')
			{
				entry = arg + 2;
			}
			else if (arg[1] == 'O')
			{
				if (arg[2] == 'd')
					optimize = false;
				else
					return error("Error: unrecognized optimization level: %s\n", arg);
			}
			else
			{
				return error("Error: unknown option: %s\n", arg);
			}
		}
		else
		{
			if (file.empty())
				file = arg;
			else
				return error("Error: only one source file argument is permitted: %s\n", arg);
		}
	}
	
	// compile
	if (file.empty())
		return error("Error: no source file specified\n");

	if (target.empty())
		return error("Error: no target specified\n");

	ShaderCompilerHLSL2GLSL compiler(metal ? "metal" : glsles ? (gl3 ? "glsles3" : "glsles") : (gl3 ? "glsl3" : "glsl"));

	std::string::size_type slash = file.find_last_of("\\/");
    std::string folder = (slash == std::string::npos) ? "." : std::string(file.begin(), file.begin() + slash);
    std::string path = (slash == std::string::npos) ? file : std::string(file.begin() + slash + 1, file.end());

    try
	{
		std::string source = compiler.preprocess(folder, path, defines);
		std::vector<char> result = compiler.compile(source, target, entry, optimize);

        fwrite(&result[0], 1, result.size(), stdout);
	}
    catch (const std::exception& e)
	{
		return error("Error compiling %s:\n%s", file.c_str(), e.what());
	}

	return 0;
}

struct PackEntryMemory
{
    std::vector<char> md5;
    std::vector<char> data;
};

struct PackEntryFile
{
    char name[64];
    char md5[16];
    unsigned int offset;
    unsigned int size;
    char reserved[8];
};

static void readData(std::istream& in, void* data, int size)
{
    in.read(static_cast<char*>(data), size);

    if (in.gcount() != size)
	{
        char buf[128];
        sprintf(buf, "Unexpected end of file while reading %d bytes", size);

        throw std::runtime_error(buf);
	}
}

std::map<std::string, PackEntryMemory> readPack(const std::string& path)
{
    std::map<std::string, PackEntryMemory> result;

	std::ifstream in(path.c_str(), std::ios::in | std::ios::binary);
    if (!in) return result;

    char header[4];
    readData(in, header, 4);

    if (memcmp(header, "RBXS", 4) != 0)
	{
		fprintf(stderr, "Warning: shader pack %s is corrupted\n", path.c_str());
        return result;
	}

    unsigned int count = 0;
    readData(in, &count, 4);

    if (!count) return result;

    std::vector<PackEntryFile> entries(count);
	readData(in, &entries[0], count * sizeof(PackEntryFile));

    for (unsigned int i = 0; i < count; ++i)
	{
		const PackEntryFile& e = entries[i];

		PackEntryMemory em;
		em.md5 = std::vector<char>(e.md5, e.md5 + sizeof(e.md5));
		em.data.resize(e.size);

		in.seekg(e.offset);
        readData(in, &em.data[0], e.size);

		result[e.name] = em;
	}

    return result;
}

bool writePack(const std::string& path, const std::map<std::string, PackEntryMemory>& data)
{
	std::vector<PackEntryFile> entries;
    std::vector<char> entryData;

	unsigned int baseDataOffset = 8 + data.size() * sizeof(PackEntryFile);

	for (std::map<std::string, PackEntryMemory>::const_iterator it = data.begin(); it != data.end(); ++it)
	{
		PackEntryFile e;

		assert(it->first.length() < sizeof(e.name));
		strncpy(e.name, it->first.c_str(), sizeof(e.name));

		assert(it->second.md5.size() == sizeof(e.md5));
		memcpy(e.md5, &it->second.md5[0], sizeof(e.md5));

		e.offset = baseDataOffset +  entryData.size();
        e.size = it->second.data.size();

        memset(e.reserved, 0, sizeof(e.reserved));

        entries.push_back(e);

        entryData.insert(entryData.end(), it->second.data.begin(), it->second.data.end());
	}

	std::ofstream out(path.c_str(), std::ios::out | std::ios::binary);
    if (!out)
    {
		fprintf(stderr, "Error: can't open shader pack %s for writing\n", path.c_str());
        return false;
	}

    unsigned int count = entries.size();

	out.write("RBXS", 4);
	out.write(reinterpret_cast<char*>(&count), sizeof(count));
	out.write(reinterpret_cast<char*>(&entries[0]), sizeof(PackEntryFile) * count);
	out.write(reinterpret_cast<char*>(&entryData[0]), entryData.size());

    return true;
}

static std::string getStringOrEmpty(const rapidjson::Value& value)
{
	return value.IsString() ? value.GetString() : "";
}

static bool isExcludedFromPack(const std::string& exclude, const std::string& packName)
{
	if (!exclude.empty())
	{
        std::istringstream iss(exclude);
        std::string item;

        while (iss >> item)
            if (item == packName)
                return true;
	}

    return false;
}

bool compilePack(const std::string& folder, const std::string& packTarget, const std::string& builtinDefines, IShaderCompiler* compiler)
{
	using namespace rapidjson;

    unsigned int succeeded = 0;
    unsigned int failed = 0;

    std::string shaderPackPath = folder + "/shaders_" + packTarget + ".pack";
    std::string sourceFolder = folder + "/source";

	std::string shaderDb = readFile(folder +  "/shaders.json");

    std::map<std::string, PackEntryMemory> shaderPackOld = readPack(shaderPackPath);
    std::map<std::string, PackEntryMemory> shaderPackNew;

    Document root;
	root.Parse<kParseDefaultFlags>(shaderDb.c_str());

    if (root.HasParseError())
		return error("Failed to load shader.json: %s\n", root.GetParseError());

	assert(root.IsArray());

    for (Value::ValueIterator it = root.Begin(); it != root.End(); ++it)
	{
		const Value& name = (*it)["name"];
		const Value& source = (*it)["source"];
		const Value& target = (*it)["target"];
		const Value& entrypoint = (*it)["entrypoint"];
		const Value& defines = (*it)["defines"];
        const Value& exclude = (*it)["exclude"];

		if (!name.IsString() || !source.IsString() || !target.IsString() || !entrypoint.IsString())
		{
			fprintf(stderr, "%s: Error parsing shader info: %s\n", packTarget.c_str(), getStringOrEmpty(name).c_str());

            failed++;
            continue;
		}

		if (isExcludedFromPack(getStringOrEmpty(exclude), packTarget))
            continue;

        try
		{
			std::string shaderSource = compiler->preprocess(sourceFolder, source.GetString(), getStringOrEmpty(defines) + " " + builtinDefines);

            char md5[16];

			MD5_CTX ctx;
            MD5_Init(&ctx);
			MD5_Update(&ctx, const_cast<char*>(shaderSource.c_str()), shaderSource.length());
			MD5_Update(&ctx, const_cast<char*>(target.GetString()), target.GetStringLength());
			MD5_Update(&ctx, const_cast<char*>(entrypoint.GetString()), entrypoint.GetStringLength());
			MD5_Final(reinterpret_cast<unsigned char*>(md5), &ctx);

			if (shaderPackOld.count(name.GetString()))
			{
				const PackEntryMemory& eOld = shaderPackOld[name.GetString()];

				if (memcmp(md5, &eOld.md5[0], sizeof(md5)) == 0)
				{
					shaderPackNew[name.GetString()] = eOld;
                    continue;
				}
			}

			std::vector<char> shaderBytecode = compiler->compile(shaderSource, target.GetString(), entrypoint.GetString(), true);

			PackEntryMemory eNew;
			eNew.md5 = std::vector<char>(md5, md5 + sizeof(md5));
			eNew.data = shaderBytecode;

			shaderPackNew[name.GetString()] = eNew;

			succeeded++;
		}
		catch (const std::exception& e)
		{
			fprintf(stderr, "%s: Error compiling %s\n%s", packTarget.c_str(), name.GetString(), e.what());

            failed++;
		}
	}

    if (failed)
	{
		fprintf(stderr, "%s: failed to compile %d shaders\n", packTarget.c_str(), failed);
        return false;
	}

	if (!writePack(shaderPackPath, shaderPackNew))
        return false;

	if (succeeded)
	{
		fprintf(stderr, "%s: updated %d shaders\n", packTarget.c_str(), succeeded);
	}

    return true;
}

int compilePacks(int argc, char** argv)
{
    if (argc < 3)
        return error("Error: no folder specified");

    std::string folder = argv[2];

    for (int i = 3; i < argc; ++i)
	{
        std::string target = argv[i];
        ShaderCompilerD3D::ShaderProfile profileD3D = ShaderCompilerD3D::getD3DProfile(target);

        if (target == "glsl")
		{
			ShaderCompilerHLSL2GLSL compiler(target);
			if (!compilePack(folder, target, "GLSL", &compiler))
                return 1;
		}
        else if (target == "glsl3")
		{
			ShaderCompilerHLSL2GLSL compiler(target);
			if (!compilePack(folder, target, "GLSL GL3", &compiler))
                return 1;
		}
        else if (target == "glsles")
		{
			ShaderCompilerHLSL2GLSL compiler(target);
			if (!compilePack(folder, target, "GLSL GLSLES", &compiler))
                return 1;
		}
        else if (target == "glsles3")
		{
			ShaderCompilerHLSL2GLSL compiler(target);
			if (!compilePack(folder, target, "GLSL GLSLES GL3", &compiler))
                return 1;
		}
    #ifdef _WIN32
        else if (profileD3D != ShaderCompilerD3D::shaderProfile_Count)
        {
            // check x86 vs x64 compatibility
#ifdef _WIN64
            if (!ShaderCompilerD3D::isX64Profile(profileD3D))
                return error("Not supported in 64bit version: %s\n", target.c_str());
#else
            if (!ShaderCompilerD3D::isX86Profile(profileD3D))
                return error("Not supported in 32bit version: %s\n", target.c_str());
#endif
            
            ShaderCompilerD3D compiler(profileD3D);

            switch (profileD3D)
		{
            case ShaderCompilerD3D::shaderProfile_DX_9:
			if (!compilePack(folder, target, "", &compiler))
                return 1;
                break;
            case ShaderCompilerD3D::shaderProfile_DX_11:
            if (!compilePack(folder, target, "DX11", &compiler))
                return 1;
                break;
            case ShaderCompilerD3D::shaderProfile_DX_11_DURANGO:
                if (!compilePack(folder, target, "DX11 DURANGO", &compiler))
                    return 1;
                break;
            case ShaderCompilerD3D::shaderProfile_DX_11_9_3:
            if (!compilePack(folder, target, "DX11 WIN_MOBILE", &compiler))
                return 1;
                break;
            default:
                break;
            }
        }
    #endif
		else
		{
            return error("Unrecognized pack target: %s\n", target.c_str());
		}
	}

    return 0;
}

int main(int argc, char** argv)
{
    if (argc > 1 && strcmp(argv[1], "/P") == 0)
        return compilePacks(argc, argv);
	else
        return compileOne(argc, argv);
}
	
