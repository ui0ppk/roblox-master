#pragma once

#include "FastLog.h"

#include <string>
#include <vector>
#include <stdexcept>

// NOTE: After removal of this fast flag, remove the HTW3C.h file and all libwww references everywhere
DYNAMIC_FASTFLAG(UseNewUrlClass);

namespace RBX
{

/* A class for parsing and assembling URLs with focus mostly on HTTP-specific subset of RFC3986.
 *
 * Works with strings having the following form:
 * https://www.roblox.com/very/long/path?query&arg=value#fragment
 * \___/   \____________/\_____________/ \_____________/ \______/
 *   |           |             |                |           |
 * scheme       host          path            query      fragment
 *                                           optional    optional
 *
 * NOTES:
 * - scheme() does not include the "://" delimiter
 * - host() is expected to be a domain name.
 * - path() always begins with '/' character (see normalization)
 * - query() and fragment() do not include their '?' and '#' delimiters
 * - URL strings with missing or empty scheme or host parts will produce invalid Url instances.
 *   (An invalid Url is that with isValid() == false)
 * - There are no guarantees for invalid Urls.
 * - Userinfo (user:password@) and :port optional parts are deliberately not handled.
 *   Trying to parse such URLs may result in invalid Url.
 * - Url instances are currently immutable.
 * - %-(un)escaping is not performed and is deliberately out of scope of this class.
 * - Automatic normalization of components is performed:
 *   - scheme and host are lowercased
 *   - path sequences like //, /. and abc/.. are collapsed
 *   - path is forced to begin with '/' character
 * - URL is invalid if:
 *   - scheme or host are missing/empty
 *   - any component contains invalid characters (see RFC3986) of malformed %-sequences
 */

class Url
{
public:
    // Parse URL string and perform normalization
    static Url fromString(const std::string& str)
    {
        return fromString(str.c_str());
    }
    
    static Url fromString(const char* str);
    
    // Build a new URL of provided components and perform normalization
    static Url fromComponents(
        const std::string& scheme,
        const std::string& host,
        const std::string& path = "/",
        const std::string& query = "",
        const std::string& fragment = ""
    );
    
    const std::string& scheme() const { return scheme_; }
    const std::string& host() const { return host_; }
    const std::string& path() const { return path_; }
    const std::string& query() const { return query_; }
    const std::string& fragment() const { return fragment_; }
    
    bool isValid() const;
    
    // Construct a string representation of the URL
    // NOTE: may produce malformed URLs if isValid() is false for non-trivial reasons (e.g. reserved unescaped characters in wrong places)
    std::string asString() const;
    
    bool hasValidScheme() const;
    bool hasHttpScheme() const;
    bool hasValidHost() const;
    bool hasValidPath() const;
    bool hasValidQuery() const;
    bool hasValidFragment() const;
    
    bool pathIsEmpty() const
    {
        return path_.length() < 2;
    }

    // Check whether the current host() is a subdomain of provided domain name.
    // - "www.roblox.com" is a subdomain of "roblox.com"
    // - "roblox.com" is a subdomain of "roblox.com"
    // - "notroblox.com" is not a subdomain of "roblox.com"
    // Domain names are case insensitive
    bool isSubdomainOf(const char* domain) const;
    
    // Check that path() is equal to path.
    // Done using simple string comparison, with the exception that path argument might not begin with '/'.
    bool pathEquals(const char* path) const;
    
    // Same as pathEquals() but performs case insensitive comparison
    bool pathEqualsCaseInsensitive(const char* path) const;

    bool isSubdomainOf(const std::string& domain) const
    {
        return isSubdomainOf(domain.c_str());
    }
    
    bool pathEquals(const std::string& path) const
    {
        return pathEquals(path.c_str());
    }
    
    bool pathEqualsCaseInsensitive(const std::string& path) const
    {
        return pathEqualsCaseInsensitive(path.c_str());
    }

private:
    void normalize();

    std::string scheme_;
    std::string host_;
    std::string path_;
    std::string query_;
    std::string fragment_;
}; // class Url
    
} // namespace rbx