#include "GfxCore/Shader.h"

#include <boost/algorithm/string.hpp>

LOGGROUP(Graphics)

namespace RBX
{
namespace Graphics
{

VertexShader::VertexShader(Device* device)
    : Resource(device)
{
}

VertexShader::~VertexShader()
{
}

FragmentShader::FragmentShader(Device* device)
    : Resource(device)
{
}

FragmentShader::~FragmentShader()
{
}

ShaderProgram::ShaderProgram(Device* device, const shared_ptr<VertexShader>& vertexShader, const shared_ptr<FragmentShader>& fragmentShader)
	: Resource(device)
    , vertexShader(vertexShader)
    , fragmentShader(fragmentShader)
{
}

ShaderProgram::~ShaderProgram()
{
}

void ShaderProgram::dumpToFLog(const std::string& text, int channel)
{
    std::vector<std::string> messages;
	boost::split(messages, text, boost::is_from_range('\n', '\n'));

	while (!messages.empty() && messages.back().empty())
		messages.pop_back();

    for (size_t i = 0; i < messages.size(); ++i)
        FASTLOGS(channel, "%s", messages[i]);
}

ShaderGlobalConstant::ShaderGlobalConstant(const char* name, unsigned int offset, unsigned int size)
	: name(name)
    , offset(offset)
    , size(size)
{
}

}
}