#include "util/StreamHelpers.h"

namespace RBX
{
    void readStreamIntoString(std::istream &stream, std::string& content)
    {
        size_t contentLength = 0;
        stream.seekg(0, std::ios::end);
        contentLength = static_cast<size_t>(stream.tellg());
        stream.seekg(0, std::ios::beg);

        content = std::string(contentLength, '\0');
        stream.read(&content[0], contentLength);
    }
} // RBX
