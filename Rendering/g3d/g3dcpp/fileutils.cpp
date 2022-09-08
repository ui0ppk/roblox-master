/**
 @file fileutils.cpp
 
 @author Morgan McGuire, graphics3d.com
 
 @author  2002-06-06
 @edited  2010-02-05
 */

#include <cstring>
#include <cstdio>
#include "G3D/platform.h"
#include "G3D/fileutils.h"

#include "G3D/stringutils.h"

namespace G3D {
    
std::string pathConcat(const std::string& dirname, const std::string& file) {
    // Ensure that the directory ends in a slash
    if ((dirname.size() != 0) && 
        (dirname[dirname.size() - 1] != '/') &&
        (dirname[dirname.size() - 1] != '\\') &&
        (dirname[dirname.size() - 1] != ':')) {
        return dirname + '/' + file;
    } else {
        return dirname + file;
    }
}

//////////////////////////////////////////////////////////////////////////////

void parseFilename(
    const std::string&  filename,
    std::string&        root,
    Array<std::string>& path,
    std::string&        base,
    std::string&        ext) {

    std::string f = filename;

    root = "";
    path.clear();
    base = "";
    ext = "";

    if (f == "") {
        // Empty filename
        return;
    }

    // See if there is a root/drive spec.
    if ((f.size() >= 2) && (f[1] == ':')) {
        
        if ((f.size() > 2) && isSlash(f[2])) {
        
            // e.g.  c:\foo
            root = f.substr(0, 3);
            f = f.substr(3, f.size() - 3);
        
        } else {
        
            // e.g.  c:foo
            root = f.substr(2);
            f = f.substr(2, f.size() - 2);

        }

    } else if ((f.size() >= 2) & isSlash(f[0]) && isSlash(f[1])) {
        
        // e.g. //foo
        root = f.substr(0, 2);
        f = f.substr(2, f.size() - 2);

    } else if (isSlash(f[0])) {
        
        root = f.substr(0, 1);
        f = f.substr(1, f.size() - 1);

    }

    // Pull the extension off
    {
        // Find the period
        size_t i = f.rfind('.');

        if (i != std::string::npos) {
            // Make sure it is after the last slash!
            size_t j = iMax(f.rfind('/'), f.rfind('\\'));
            if ((j == std::string::npos) || (i > j)) {
                ext = f.substr(i + 1, f.size() - i - 1);
                f = f.substr(0, i);
            }
        }
    }

    // Pull the basename off
    {
        // Find the last slash
        size_t i = iMax(f.rfind('/'), f.rfind('\\'));
        
        if (i == std::string::npos) {
            
            // There is no slash; the basename is the whole thing
            base = f;
            f    = "";

        } else if ((i != std::string::npos) && (i < f.size() - 1)) {
            
            base = f.substr(i + 1, f.size() - i - 1);
            f    = f.substr(0, i);

        }
    }

    // Parse what remains into path.
    size_t prev, cur = 0;

    while (cur < f.size()) {
        prev = cur;
        
        // Allow either slash
        size_t i = f.find('/', prev + 1);
        size_t j = f.find('\\', prev + 1);
        if (i == std::string::npos) {
            i = f.size();
        }

        if (j == std::string::npos) {
            j = f.size();
        }

        cur = iMin(i, j);

        if (cur == std::string::npos) {
            cur = f.size();
        }

        path.append(f.substr(prev, cur - prev));
        ++cur;
    }
}

std::string filenameBaseExt(const std::string& filename) {
    int i = filename.rfind("/");
    int j = filename.rfind("\\");

    if ((j > i) && (j >= 0)) {
        i = j;
    }

#   ifdef G3D_WIN32
        j = filename.rfind(":");
        if ((i == -1) && (j >= 0)) {
            i = j;
        }
#   endif

    if (i == -1) {
        return filename;
    } else {
        return filename.substr(i + 1, filename.length() - i);
    }
}


std::string filenameBase(const std::string& s) {
    std::string drive;
    std::string base;
    std::string ext;
    Array<std::string> path;

    parseFilename(s, drive, path, base, ext);
    return base;
}


std::string filenameExt(const std::string& filename) {
    int i = filename.rfind(".");
    if (i >= 0) {
        return filename.substr(i + 1, filename.length() - i);
    } else {
        return "";
    }
}


std::string filenamePath(const std::string& filename) {
    int i = filename.rfind("/");
    int j = filename.rfind("\\");

    if ((j > i) && (j >= 0)) {
        i = j;
    }

#   ifdef G3D_WIN32
        j = filename.rfind(":");
        if ((i == -1) && (j >= 0)) {
            i = j;
        }
#   endif

    if (i == -1) {
        return "";
    } else {
        return filename.substr(0, i+1);
    }
}


bool filenameContainsWildcards(const std::string& filename) {
    return (filename.find('*') != std::string::npos) || (filename.find('?') != std::string::npos);
}

}

