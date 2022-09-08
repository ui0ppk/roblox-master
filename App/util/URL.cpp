#include "stdafx.h"

#include "util/URL.h"

#include "rbx/debug.h"

#include <boost/algorithm/string.hpp>

#include <sstream>
#include <algorithm>
#include <cstring>

DYNAMIC_FASTFLAGVARIABLE(UseNewUrlClass, true);

namespace
{

class RFC3986
{
public:
    enum CharClassBit
    {
        CharClassBit_Unreserved = 0x01,
        CharClassBit_PercentEncoded = 0x02,
        CharClassBit_SubDelim = 0x04,
        CharClassBit_HexDigit = 0x08,
        CharClassBit_Pchar = 0x10 |
            CharClassBit_Unreserved |
            CharClassBit_PercentEncoded |
            CharClassBit_SubDelim,
        CharClassBit_RegName =
            CharClassBit_Unreserved |
            CharClassBit_PercentEncoded |
            CharClassBit_SubDelim,
        CharClassBit_Path = 0x20 | CharClassBit_Pchar,
        CharClassBit_QueryOrFragment = 0x40 | CharClassBit_Pchar,
        CharClassBit_Scheme = 0x80
    };
    
    static const char* findFirstCharNotInClass(const char* begin, const char* end, CharClassBit charclass);
    
    static bool checkStringIsOfClass(const char* begin, const char* end, CharClassBit charclass)
    {
        const char* firstnot = findFirstCharNotInClass(begin, end, charclass);
        return begin != end && firstnot == end;
    }
    
    static bool checkStringIsOfClass(const std::string& str, CharClassBit charclass)
    {
        return checkStringIsOfClass(str.c_str(), str.c_str() + str.length(), charclass);
    }
    
    static uint8_t charClassBits(char c)
    {
        return charClassTable_[static_cast<unsigned char>(c)];
    }

private:
    static const uint8_t charClassTable_[256];
}; // class RFC3986

const uint8_t RFC3986::charClassTable_[256] =
{
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x04, 0x00, 0x00, 0x04, 0x02, 0x04, 0x04, 0x04, 0x04, 0x04, 0x84, 0x04, 0x81, 0x81, 0x60,
0x89, 0x89, 0x89, 0x89, 0x89, 0x89, 0x89, 0x89, 0x89, 0x89, 0x10, 0x04, 0x00, 0x04, 0x00, 0x40,
0x10, 0x89, 0x89, 0x89, 0x89, 0x89, 0x89, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81,
0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x00, 0x00, 0x00, 0x00, 0x01,
0x00, 0x89, 0x89, 0x89, 0x89, 0x89, 0x89, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81,
0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x00, 0x00, 0x00, 0x01, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const char* RFC3986::findFirstCharNotInClass(const char* begin, const char* end, CharClassBit charClass)
{
    for (; begin != end; ++begin)
    {
        const uint8_t classMatch = charClassBits(*begin) & charClass;
        if (!classMatch)
            return begin;
        if (classMatch & CharClassBit_PercentEncoded)
        {
            if (end - begin < 3 || !(charClassBits(begin[1]) & charClassBits(begin[2]) & CharClassBit_HexDigit))
                return begin;
            begin += 2;
        }
    }
    return end;
}
    
} // unnamed namespace

namespace RBX
{

Url Url::fromString(const char* str)
{
    Url url;

    const char* end = str + ::strlen(str);
    
    const char* scheme_begin = str;
    const char *host_begin = scheme_begin;
    
    if (end == str)
        return url;
    
    const char *scheme_end = RFC3986::findFirstCharNotInClass(scheme_begin, end, RFC3986::CharClassBit_Scheme);
    
    if (end - scheme_end >= 3 && scheme_end[0] == ':' && scheme_end[1] == '/' && scheme_end[2] == '/')
    {
        url.scheme_ = std::string(scheme_begin, scheme_end);
        host_begin = scheme_end + 3;
    }
    
    const char *host_end = strchr(host_begin, '/');
    if (!host_end)
    {
        url.host_ = std::string(host_begin, end);
        url.path_ = "/";
        return url;
    }
    else
    {
        url.host_ = std::string(host_begin, host_end);
    }
    
    const char *path_begin = host_end;
    const char *path_end = end;
    
    if (path_begin == path_end)
    {
        url.path_ = "/";
    }
    else
    {
        // both query and fragment allow '?' char, so fragment is detected before query
        const char *fragment_begin = ::strchr(path_begin, '#');
        if (fragment_begin)
        {
            path_end = fragment_begin;
            url.fragment_ = std::string(fragment_begin + 1, end);
        }

        const char *query_begin = ::strchr(path_begin, '?');
        if (query_begin && (!fragment_begin || query_begin < fragment_begin))
        {
            const char *query_end = path_end;
            path_end = query_begin;
            url.query_ = std::string(query_begin + 1, query_end);
        }
        
        url.path_ = std::string(path_begin, path_end);
    }

    url.normalize();
    return url;
} // Url::fromString()

Url Url::fromComponents(
    const std::string& scheme,
    const std::string& host,
    const std::string& path,
    const std::string& query,
    const std::string& fragment)
{
    Url url;
    url.scheme_ = scheme;
    url.host_ = host;
    url.path_ = path;
    url.query_ = query;
    url.fragment_ = fragment;
    url.normalize();
    return url;
}

void Url::normalize()
{
    std::transform(scheme_.begin(), scheme_.end(), scheme_.begin(), ::tolower);
    std::transform(host_.begin(), host_.end(), host_.begin(), ::tolower);
    
    // remove excess things like /.., //, /. from path
    if (path_.empty())
    {
        if (!scheme_.empty() && !host_.empty())
            path_ = "/";
    }
    else
    {
        // Having '/' at the beginning:
        // - required by RFC
        // - really helps with boundary conditions below
        if (path_[0] != '/')
            path_.insert(path_.begin(), '/');

        // Process path components in-place in reverse
        // Every component can be:
        // - passed as is
        // - skipped:
        //   - // or /.
        //   - /.. (and depth++)
        //   - if --depth > 0

        char *begin = &path_[0];
        char *end = begin + path_.length();
        
        char *p = end[-1] == '/' ? end - 1 : end;
        char *skipstart = 0, *skipend = 0;
        size_t depth = 0;
        while (p != begin)
        {
            // get next path component
            char * const compend = p;
            while (*(--p) != '/');
            
            const size_t complen = compend - p;
            RBXASSERT(complen > 0);
            
            bool skip = false;
            // skip "//"
            if (complen == 1 ||
                // skip "/."
                (complen == 2 && p[1] == '.'))
            {
                skip = true;
            }
            // skip "/.."
            else if (complen == 3 && p[1] == '.' && p[2] == '.')
            {
                ++depth;
                skip = true;
            }
            // skip parents if one or more "/.." were encountered
            else if (depth > 0)
            {
                --depth;
                skip = true;
            }
            // anything else is passed
            else
            {
                skip = false;
            }
            
            // handle skipping
            if (skip)
            {
                // remember/extend skipping range
                skipstart = p;
                if (!skipend)
                    skipend = compend;
            }
            // if next component is to be passed
            // cut out any existing range to skip
            else if (skipstart)
            {
                RBXASSERT(skipend);
                const size_t skiplen = skipend - skipstart;
                const size_t taillen = end - skipend;
                std::memmove(skipstart, skipend, taillen);
                end -= skiplen;
                skipstart = skipend = 0;
            }
        } // while (p != begin)
        
        // skip any left range
        if (skipstart)
        {
            RBXASSERT(skipend);
            const size_t skiplen = skipend - skipstart;
            const size_t taillen = end - skipend;
            std::memmove(skipstart, skipend, taillen);
            end -= skiplen;
        }
        
        if (end > begin)
            path_.resize(end - begin);
        else
            path_ = "/";

    } // if path is not empty
} // Url::normalize()

std::string Url::asString() const
{
    std::string result;
    result.reserve(
        scheme_.length() + 3 +
        host_.length() +
        path_.length() + 1 +
        query_.length() + 1 +
        fragment_.length() + 1);
    
    if (!scheme_.empty())
    {
        result += scheme_;
        result += "://";
    }
    
    result += host_;
    
    if (path_.empty())
    {
        if (!host_.empty())
        {
            result += '/';
        }
    }
    else
    {
        if (path_[0] != '/')
            result += '/';
        result += path_;
    }
    
    if (!query_.empty())
    {
        result += '?';
        result += query_;
    }
    
    if (!fragment_.empty())
    {
        result += '#';
        result += fragment_;
    }

    return result;
} // Url::asString()

bool Url::isValid() const
{
    return hasValidScheme() && hasValidHost() && hasValidPath() && hasValidQuery() && hasValidFragment();
}

bool Url::hasValidScheme() const
{
    if (scheme_.empty() || !::isalpha(scheme_[0]))
        return false;
    
    return RFC3986::checkStringIsOfClass(scheme_, RFC3986::CharClassBit_Scheme);
}

bool Url::hasHttpScheme() const
{
    return scheme_ == "http" || scheme_ == "https";
}

bool Url::hasValidHost() const
{
    // NOTE: only reg-name host part is expected
    // URL containing userinfo@ or :port parts will be invalid
    return RFC3986::checkStringIsOfClass(host_, RFC3986::CharClassBit_RegName);
}

bool Url::hasValidPath() const
{
    return RFC3986::checkStringIsOfClass(path_, RFC3986::CharClassBit_Path);
}

bool Url::hasValidQuery() const
{
    return query_.empty() || RFC3986::checkStringIsOfClass(path_, RFC3986::CharClassBit_QueryOrFragment);
}
    
bool Url::hasValidFragment() const
{
    return fragment_.empty() || RFC3986::checkStringIsOfClass(path_, RFC3986::CharClassBit_QueryOrFragment);
}

bool Url::isSubdomainOf(const char* domain) const
{
    auto index = boost::ifind_last(host_, domain);

    if (index.begin() == host_.end())
        return false;
    
    if (index.end() != host_.end())
        return false;
    
    if (index.begin() > host_.begin())
        return host_[index.begin() - host_.begin() - 1] == '.';

    return true;
}

bool Url::pathEquals(const char* path) const
{
    // Empty paths are equivalent
    const bool argIsEmpty = !path || !path[0] || (path[0] == '/' && !path[1]);
    const bool ownPathIsEmpty = pathIsEmpty();

    if (argIsEmpty || ownPathIsEmpty)
        return argIsEmpty && ownPathIsEmpty;
    
    if (path[0] == '/')
        ++path;
    
    return boost::algorithm::equals(path_.c_str() + 1, path);
}

bool Url::pathEqualsCaseInsensitive(const char* path) const
{
    // Empty paths are equivalent
    const bool argIsEmpty = !path || !path[0] || (path[0] == '/' && !path[1]);
    const bool ownPathIsEmpty = pathIsEmpty();

    if (argIsEmpty || ownPathIsEmpty)
        return argIsEmpty && ownPathIsEmpty;
    
    if (path[0] == '/')
        ++path;
    
    return boost::algorithm::iequals(path_.c_str() + 1, path);
}



} // namespace util