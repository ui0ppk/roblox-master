#include "ShaderGL.h"

#include "GeometryGL.h"
#include "DeviceGL.h"

#include "HeadersGL.h"

#include <set>
#include <map>

LOGGROUP(Graphics)

namespace RBX
{
namespace Graphics
{

static int getShaderParameter(GLuint shader, GLenum pname)
{
    GLint result;
    glGetShaderiv(shader, pname, &result);
    return result;
}

static int getProgramParameter(GLuint program, GLenum pname)
{
    GLint result;
    glGetProgramiv(program, pname, &result);
    return result;
}

template <typename GetParameter, typename GetLog>
static std::string getInfoLog(unsigned int id, GetParameter getParameter, GetLog getLog)
{
    int infoLength = getParameter(id, GL_INFO_LOG_LENGTH);

    if (infoLength > 0)
	{
        std::vector<char> buffer(infoLength);

        getLog(id, infoLength, NULL, &buffer[0]);

        return std::string(&buffer[0]);
	}
	else
	{
        return "";
	}
}

template <typename GetParameter, typename GetLog>
static void dumpInfoLog(unsigned int id, GetParameter getParameter, GetLog getLog)
{
	std::string log = getInfoLog(id, getParameter, getLog);

    // Intel and AMD drivers like to say "No errors" for every shader.
	if (strstr(log.c_str(), "warn") || strstr(log.c_str(), "Warn") || strstr(log.c_str(), "WARN"))
	{
        FASTLOG2(FLog::Graphics, "Shader %d has a non-empty infolog (length %d)", id, log.length());

		ShaderProgram::dumpToFLog(log, FLog::Graphics);
	}
}

static unsigned int compileShader(const std::string& source, GLenum type)
{
    unsigned int id = glCreateShader(type);
    RBXASSERT(id);
    
    int length = source.length();
    const char* data = source.c_str();
    
    glShaderSource(id, 1, &data, &length);
    
    glCompileShader(id);

	if (getShaderParameter(id, GL_COMPILE_STATUS) != 1)
	{
        std::string infoLog = getInfoLog(id, getShaderParameter, glGetShaderInfoLog);

        glDeleteShader(id);

		throw std::runtime_error(infoLog);
	}

    dumpInfoLog(id, getShaderParameter, glGetShaderInfoLog);

    return id;
}

static unsigned int parseAttribs(const std::string& source, const char* prefix)
{
    std::set<std::string> attribs;

    size_t offset = 0;

    while (offset < source.length())
	{
        std::string::size_type start = source.find(prefix, offset);
        if (start == std::string::npos)
            break;

        std::string::size_type eol = source.find('\n', start);
        if (eol == std::string::npos)
            break;

        offset = eol;

        std::string::size_type semi = source.find(';', start);
        std::string::size_type space = source.find_last_of(' ', eol);

        if ((start == 0 || source[start - 1] == '\n') && semi != std::string::npos && space != std::string::npos && space < semi)
            attribs.insert(source.substr(space + 1, semi - space - 1));
    }
    
    unsigned int result = 0;

    for (unsigned int attr = 0; attr < 16; ++attr)
    {
        const char* name = VertexLayoutGL::getShaderAttributeName(attr);

        if (name && attribs.count(name))
            result |= 1 << attr;
    }

    return result;
}

static void applyAttribs(unsigned int id, unsigned int attribMask)
{
    for (unsigned int attr = 0; attr < 16; ++attr)
        if (attribMask & (1 << attr))
        {
            const char* name = VertexLayoutGL::getShaderAttributeName(attr);
            RBXASSERT(name);
            glBindAttribLocation(id, attr, name);
        }
}

static std::vector<FragmentShaderGL::Sampler> parseSamplers(const std::string& source)
{
	std::vector<FragmentShaderGL::Sampler> result;

    size_t offset = 0;

    while (offset < source.length())
	{
        std::string::size_type start = source.find("//$$", offset);
        if (start == std::string::npos)
            return result;

        std::string::size_type equals = source.find('=', start);
        if (equals == std::string::npos)
            return result;

        std::string::size_type end = source.find('\n', equals);
        if (end == std::string::npos)
            return result;

		std::string name(source.begin() + start + 4, source.begin() + equals);
		std::string value(source.begin() + equals + 1, source.begin() + end);

		if (!value.empty() && value[0] == 's')
		{
			FragmentShaderGL::Sampler entry = {name, atoi(value.c_str() + 1)};

			result.push_back(entry);
		}

        offset = end;
	}

    return result;
}

static int getSamplerLocation(unsigned int id, const std::string& name)
{
    int location;
	
	if ((location = glGetUniformLocation(id, name.c_str())) >= 0)
        return location;
	
	if ((location = glGetUniformLocation(id, ("xlu_" + name).c_str())) >= 0)
        return location;

    return -1;
}

static unsigned int applySamplers(unsigned int id, const std::vector<FragmentShaderGL::Sampler>& samplers)
{
    unsigned int mask = 0;

    glUseProgram(id);

    for (size_t i = 0; i < samplers.size(); ++i)
	{
		int location = getSamplerLocation(id, samplers[i].name);

        if (location >= 0)
		{
			glUniform1i(location, samplers[i].slot);

			mask |= 1 << samplers[i].slot;
		}
	}

    glUseProgram(0);

    return mask;
}

static ShaderProgramGL::Uniform::Type getUniformType(GLenum type)
{
    switch (type)
	{
	case GL_FLOAT:
		return ShaderProgramGL::Uniform::Type_Float;
	case GL_FLOAT_VEC2:
		return ShaderProgramGL::Uniform::Type_Float2;
	case GL_FLOAT_VEC3:
		return ShaderProgramGL::Uniform::Type_Float3;
	case GL_FLOAT_VEC4:
		return ShaderProgramGL::Uniform::Type_Float4;
	case GL_FLOAT_MAT4:
		return ShaderProgramGL::Uniform::Type_Float4x4;
	default:
		return ShaderProgramGL::Uniform::Type_Unknown;
	}
}

typedef std::map<std::string, ShaderProgramGL::Uniform> UniformTable;

static UniformTable extractUniforms(unsigned int id)
{
	int uniformCount = getProgramParameter(id, GL_ACTIVE_UNIFORMS);
	int uniformMaxLength = getProgramParameter(id, GL_ACTIVE_UNIFORM_MAX_LENGTH);

	std::vector<char> uniformName(uniformMaxLength + 1);

    UniformTable result;

    for (int i = 0; i < uniformCount; ++i)
	{
        GLint size;
        GLenum type;
		glGetActiveUniform(id, i, uniformMaxLength, NULL, &size, &type, &uniformName[0]);

		ShaderProgramGL::Uniform::Type uniformType = getUniformType(type);

		int location = glGetUniformLocation(id, &uniformName[0]);

        if (location >= 0 && uniformType != ShaderProgramGL::Uniform::Type_Unknown)
		{
			ShaderProgramGL::Uniform uniform = {location, uniformType, size, 0};

			std::string name(&uniformName[0]);

            // Remove xlu_ prefix for entrypoint-local uniforms
			if (name.compare(0, 4, "xlu_") == 0)
                name.erase(name.begin(), name.begin() + 4);

            // Remove [0] from arrays
			std::string::size_type index = name.find('[');

            if (index != std::string::npos)
				name.erase(name.begin() + index, name.end());

            result[name] = uniform;
		}
	}

    return result;
}

static const ShaderProgramGL::Uniform* findUniform(const UniformTable& ctab, const std::string& name)
{
	UniformTable::const_iterator it = ctab.find(name);

    return (it == ctab.end()) ? NULL : &it->second;
}
    
static inline void transposeMatrix(float* dest, const float* source, const float* lastColumn)
{
    dest[0] = source[0]; dest[1] = source[4]; dest[2] = source[8]; dest[3] = lastColumn[0];
    dest[4] = source[1]; dest[5] = source[5]; dest[6] = source[9]; dest[7] = lastColumn[1];
    dest[8] = source[2]; dest[9] = source[6]; dest[10] = source[10]; dest[11] = lastColumn[2];
    dest[12] = source[3]; dest[13] = source[7]; dest[14] = source[11]; dest[15] = lastColumn[3];
}

VertexShaderGL::VertexShaderGL(Device* device, const std::string& source)
	: VertexShader(device)
    , id(0)
    , attribMask(0)
{
    id = compileShader(source, GL_VERTEX_SHADER);

    attribMask = parseAttribs(source, static_cast<DeviceGL*>(device)->getCapsGL().ext3 ? "in " : "attribute ");
}

VertexShaderGL::~VertexShaderGL()
{
	glDeleteShader(id);
}

void VertexShaderGL::reloadBytecode(const std::vector<char>& bytecode)
{
	throw std::runtime_error("Bytecode reloading is not supported");
}

FragmentShaderGL::FragmentShaderGL(Device* device, const std::string& source)
	: FragmentShader(device)
    , id(0)
{
    id = compileShader(source, GL_FRAGMENT_SHADER);

	samplers = parseSamplers(source);
}

FragmentShaderGL::~FragmentShaderGL()
{
	glDeleteShader(id);
}

void FragmentShaderGL::reloadBytecode(const std::vector<char>& bytecode)
{
	throw std::runtime_error("Bytecode reloading is not supported");
}

ShaderProgramGL::ShaderProgramGL(Device* device, const shared_ptr<VertexShader>& vertexShader, const shared_ptr<FragmentShader>& fragmentShader)
	: ShaderProgram(device, vertexShader, fragmentShader)
    , id(0)
	, uniformWorldMatrix(-1)
	, uniformWorldMatrixArray(-1)
	, cachedGlobalVersion(0)
	, maxWorldTransforms(0)
    , samplerMask(0)
{
	id = glCreateProgram();
	RBXASSERT(id);

	glAttachShader(id, static_cast<VertexShaderGL*>(vertexShader.get())->getId());
	glAttachShader(id, static_cast<FragmentShaderGL*>(fragmentShader.get())->getId());

    // Setup attribute locations
    applyAttribs(id, static_cast<VertexShaderGL*>(vertexShader.get())->getAttribMask());

    glLinkProgram(id);

	if (getProgramParameter(id, GL_LINK_STATUS) != 1)
	{
		std::string infoLog = getInfoLog(id, getProgramParameter, glGetProgramInfoLog);

        glDeleteProgram(id);

		throw std::runtime_error(infoLog);
	}

    dumpInfoLog(id, getProgramParameter, glGetProgramInfoLog);

    // Make sure the program uses correct sampler slot assignments
    // Note: this resets current program to 0!
	static_cast<DeviceGL*>(device)->getImmediateContextGL()->invalidateCachedProgram();

	samplerMask = applySamplers(id, static_cast<FragmentShaderGL*>(fragmentShader.get())->getSamplers());

    // Populate constant tables
	const std::vector<ShaderGlobalConstant>& globalConstants = static_cast<DeviceGL*>(device)->getGlobalConstants();

    UniformTable ctab = extractUniforms(id);

	for (size_t i = 0; i < globalConstants.size(); ++i)
		if (const Uniform* uniform = findUniform(ctab, globalConstants[i].name))
		{
			Uniform u = *uniform;
			u.offset = globalConstants[i].offset;

			globalUniforms.push_back(u);

			ctab.erase(globalConstants[i].name);
		}

    // Populate world matrix information
    if (const Uniform* uniform = findUniform(ctab, "WorldMatrix"))
	{
		RBXASSERT(uniform->type == Uniform::Type_Float4x4);

		uniformWorldMatrix = uniform->location;
		maxWorldTransforms = 1;

        ctab.erase("WorldMatrix");
	}

    if (const Uniform* uniform = findUniform(ctab, "WorldMatrixArray"))
	{
		RBXASSERT(uniform->type == Uniform::Type_Float4);
        RBXASSERT(uniform->size % 3 == 0);

		uniformWorldMatrixArray = uniform->location;
		maxWorldTransforms = uniform->size / 3;

        ctab.erase("WorldMatrixArray");
	}

    // Populate uniform information
	for (UniformTable::iterator it = ctab.begin(); it != ctab.end(); ++it)
	{
		uniforms.push_back(std::make_pair(it->first, it->second));
	}
}

ShaderProgramGL::~ShaderProgramGL()
{
	glDeleteProgram(id);

    // Shader program destruction invalidates currently bound program id
	static_cast<DeviceGL*>(device)->getImmediateContextGL()->invalidateCachedProgram();
}

int ShaderProgramGL::getConstantHandle(const char* name) const
{
    for (size_t i = 0; i < uniforms.size(); ++i)
		if (uniforms[i].first == name)
            return i;

    return -1;
}

unsigned int ShaderProgramGL::getMaxWorldTransforms() const
{
    return maxWorldTransforms;
}

unsigned int ShaderProgramGL::getSamplerMask() const
{
    return samplerMask;
}

void ShaderProgramGL::bind(const void* globalData, unsigned int globalVersion, ShaderProgramGL** cache)
{
    if (*cache != this)
    {
        *cache = this;

        glUseProgram(id);
    }

	if (cachedGlobalVersion != globalVersion)
	{
        cachedGlobalVersion = globalVersion;

        for (size_t i = 0; i < globalUniforms.size(); ++i)
		{
            const Uniform& u = globalUniforms[i];

            const float* data = reinterpret_cast<const float*>(static_cast<const char*>(globalData) + u.offset);

            switch (u.type)
			{
			case Uniform::Type_Float:
				glUniform1fv(u.location, 1, data);
                break;

			case Uniform::Type_Float2:
				glUniform2fv(u.location, 1, data);
                break;

			case Uniform::Type_Float3:
				glUniform3fv(u.location, 1, data);
                break;

			case Uniform::Type_Float4:
				glUniform4fv(u.location, 1, data);
                break;

			case Uniform::Type_Float4x4:
            {
                float matrix[16];
                transposeMatrix(matrix, data, data + 12);
                
                glUniformMatrix4fv(u.location, 1, false, matrix);
                break;
            }
                    
            default:
                break;
			}
		}
	}
}

void ShaderProgramGL::setWorldTransforms4x3(const float* data, size_t matrixCount)
{
    if (matrixCount == 0)
        return;

	if (uniformWorldMatrix >= 0)
	{
        static const float lastColumn[] = {0, 0, 0, 1};
        
        float matrix[16];
        transposeMatrix(matrix, data, lastColumn);
        
		glUniformMatrix4fv(uniformWorldMatrix, 1, false, matrix);
	}

	if (uniformWorldMatrixArray >= 0)
	{
		RBXASSERT(matrixCount <= maxWorldTransforms);

		glUniform4fv(uniformWorldMatrixArray, matrixCount * 3, data);
	}
}

void ShaderProgramGL::setConstant(int handle, const float* data, size_t vectorCount)
{
    if (handle < 0)
        return;

    RBXASSERT(static_cast<size_t>(handle) < uniforms.size());

	const Uniform& u = uniforms[handle].second;

    switch (u.type)
    {
    case Uniform::Type_Float:
        RBXASSERT(vectorCount == 1);
        glUniform1fv(u.location, 1, data);
        break;

    case Uniform::Type_Float2:
        RBXASSERT(vectorCount == 1);
        glUniform2fv(u.location, 1, data);
        break;

    case Uniform::Type_Float3:
        RBXASSERT(vectorCount == 1);
        glUniform3fv(u.location, 1, data);
        break;

    case Uniform::Type_Float4:
		RBXASSERT(vectorCount > 0 && vectorCount <= u.size);
        glUniform4fv(u.location, vectorCount, data);
        break;

	default:
		RBXASSERT(false);
    }
}

}
}
