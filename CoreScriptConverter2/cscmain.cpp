#define _CRT_SECURE_NO_WARNINGS 1

#include <stdio.h>
#include <stdarg.h>
#include <boost/filesystem.hpp>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>

#include "script/LuaVM.h"
#include "util/Utilities.h" // for rot13

#ifdef _MSC_VER
#define ASSERT(X) ((void) ( (X) || ( fprintf(stderr, "%s(%d): ASSERTION FAILED: %s\n", __FILE__, __LINE__, #X), __debugbreak(), 1 ) )  )
#else
#define ASSERT(X)
#endif

using namespace std;
using namespace boost::filesystem;

struct filedesc
{
    string name;
    path   file;
    bool   module;
};


//3456                                                                         |<--80
const char* usage = 
"Roblox Script Compiler - compiles and encrypts local core scripts."
""
"Usage:\n"
"\n"
"rsc [option]\n"
"\n"
"Options:\n"
"  --help     - this help message\n"
"  --helpcfg  - config file help\n"
"  --verbose  - print .lua files and name mappings to stdout\n"
"  --rscloc   - changes the location to look for rsc.config file\n"
"\n"
"When run, rsc requires a config file named rsc.config to be present in the\n"
"current directory. The config file specifies paths and additional options for\n"
"the script compilation process.\n"
;

const char* cfghelp =
"rsc.config file reference\n"
"\n"
"The syntax is key=value. Preceding and trailing whitespace chars are ignored.\n"
"All relative paths assume the location of rsc.config as the base path.\n"
"Comment lines must start with '#'.\n"
"Required settings:\n"
"\n"
"source   - path to look for Lua scripts.\n"
"output   - output path/name (e.g. ../App/script/LuaGenCS.inl)\n"
"\n"
"Optional settings:\n"
"source1, ... source9 - additional paths to look for Lua scripts\n"
;


map<string,string> config;
int verbose = 0;

//////////////////////////////////////////////////////////////////////////
static int fatal(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "FATAL: ");
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(1);
    return 1;
}

template< class Map >
static inline typename Map::mapped_type* mapget(const Map& m, const typename Map::key_type& k)
{
    typename Map::const_iterator it = m.find(k);
    if( it == m.end() ) return 0;
    return const_cast<typename Map::mapped_type*>(&it->second);
}

static void rdconfig(const char* cfgfile, char** argv)
{
    config.clear();

    FILE* vf = fopen(cfgfile, "rt");
    vf || fatal( "could not open '%s'. Run 'rsc --help' for help.\n", cfgfile, argv[0]);

    char str[2000];
    char* k, *v, *q;
    int  line = 1;

    // parses "key=value", trims the whitespace, so that "   key    =    value   " is still ok
    while (true)
    {
        if (!fgets(str, sizeof(str), vf))
            break;

        for (k = str; *k && strchr(" \n\t", *k); ++k) {} // ignore the whitespace

        if (!*k || *k == '#') { continue; } // empty line, whitespace or comment

        for (q = k; isalnum(*q) || *q == '_'; ++q ) {} // find where key ends

        while( *q && *q != '=' ) { *q++ = 0; } // find the '='

        *q == '=' || fatal("%s(%d): '=' expected.\n", cfgfile, line);
        *q = 0; // kill the '=' sign, because who cares

        strlen(k) || fatal("%s(%d): option name expected before '='.\n", cfgfile, line);

        for (v = q + 1; *v && strchr(" \t", *v); ++v) {} // find where the value starts

        q = v + strlen(v) - 1; // end of the line
        while ( q > v && strchr(" \n\t", *q ) ) { *q-- = 0; } // trim from the end

        // at this point, k points to key, v points to value

        config.insert( map<string, string>::value_type(k,v) ).second  || fatal("%s(%d): duplicate entry for '%s'", cfgfile, line, k);

        line++;
    }

    fclose(vf);
}

void rdfile(string* result, const path& filepath)
{
    ASSERT(result);

#if _MSC_VER
    FILE* vf = _wfopen(filepath.native().c_str(), L"rb");
#else
    FILE* vf = fopen(filepath.native().c_str(), "rb");
#endif

    vf || fatal("could not open '%s'\n", filepath.native().c_str() );
    
    // no better portable way to get file size
    fseek(vf, 0, SEEK_END);
    long size = ftell(vf);
    fseek(vf, 0, SEEK_SET);
    
    result->resize(size);
    fread( (char*)result->data(), 1, size, vf);
    fclose(vf);
}


void buildFileList( vector<filedesc>& files, path dir, path parent = path() )
{
    for( boost::filesystem::directory_iterator it(dir), e; it != e; ++it )
    {
        directory_entry& dirent  = *it;
        const path& filepath     = dirent.path();
        file_status st           = dirent.status();

        if (st.type() == regular_file)
            if (filepath.extension() == ".lua")
            {
                path name = parent / filepath.filename();

                filedesc d;
                d.name = name.replace_extension().string();
				std::replace( d.name.begin(),  d.name.end(), '\\', '/');
                d.file = filepath;
                d.module = d.name.find("Modules") == 0;
                if(d.module)
                    d.name = d.name.substr( 8 ); // remove the "Modules/" from the name, because that's how it's supposed to be

                files.push_back(d);

                if (verbose)
                {
                    printf( " %s: %s\n", d.name.c_str(), filepath.native().c_str() );
                }
            }

        if (dirent.status().type() == directory_file)
        {
            if( dirent.path().filename() != ".git" )
                buildFileList(files, dirent.path(), parent / dirent.path().filename() );
        }
    }
}

char* hexa(unsigned char ch)
{
    static const char sym[] = "0123456789abcdef";
    static char buf[8];
    buf[0] = '0';
    buf[1] = 'x';
    buf[2] = sym[ (ch>>4) & 0xf ];
    buf[3] = sym[ (ch   ) & 0xf ];
    buf[4] = 0;
    return buf;
}

void printBuffer( ostream& os, const string& name, const string& data)
{
    os << "static const unsigned char "<<name<<"[] = {";
    for( int j=0, e = data.size(); j<e; ++j )
    {
        if ( !(j%32) )
            os<<"\n    ";
        os<<hexa(data[j])<<", ";
    }
    os << "\n    };\n\n";
}


int main(int argc, char** argv)
{
	std::string rscloc = "rsc.config";

    for( int j=1; j<argc; ++j )
    {
        if (!strcmp(argv[j], "--help")) 
            return fprintf( stderr, "%s\n", usage), 0; 
        else if (!strcmp(argv[j], "--helpcfg"))
            return fprintf( stderr, "%s\n", cfghelp), 0;
        else if (!strcmp(argv[j], "--verbose"))
            verbose = 1;
		else if(!strcmp(argv[j], "--rscloc"))
		{
			if (j + 1 < argc)
			{
				rscloc = argv[j + 1];
				j++;
			}
		}
        else
            fatal("command-line: bad option '%s', use --help for info.\n", argv[j]);
    }

	rdconfig(rscloc.c_str(), argv);

    if (verbose)
    {
        for (auto a = config.begin(); a != config.end(); ++a)
        {
            printf("'%s'='%s'\n", a->first.c_str(), a->second.c_str());
        }
    }

    mapget(config, "source") || fatal("missing 'source' in rsc.config\n");
    mapget(config, "output") || fatal("missing 'output' in rsc.config\n");

    path rsclocPath(rscloc.c_str());
    vector<filedesc> files;
    path source = rsclocPath.parent_path() / config["source"];
    buildFileList(files, source);
    
    for(int j=1; j<9; ++j)
    {
        char keyname[44];
        sprintf( keyname, "source%d", j );

        if( string* dir = mapget(config, string(keyname)) )
        {
            path p = rsclocPath.parent_path() / *dir;
            buildFileList(files, p );
        }
    }

    // got all the files, go ahead and compile
    
    if(1)
    {
        stringstream arrays, scripts, modules; // 3 main sections

        scripts << "static const CoreScriptBytecode gCoreScripts[] = {\n";
        modules << "static const CoreScriptBytecode gCoreModuleScripts[] = {\n";
        
        for( unsigned j=0; j<files.size(); ++j)
        {
            string source;
            filedesc& fd = files[j];
            rdfile( &source, fd.file);

            std::string bytecode = LuaVM::compileCore(source);

            string encname = fd.name;
            string arrayName = RBX::format( "a%04u", j);
            encname = RBX::rot13(encname);

            printBuffer(arrays, arrayName, bytecode);

            (fd.module ? modules : scripts) << "    { \"" << encname << "\", " << arrayName << ", " << bytecode.size() << " },\n";

            printf(".");
        }
        printf("\n");

        scripts << "};\n\n";
        modules << "};\n\n";

        printf("\n");
        path outputFile = rsclocPath.parent_path() / config["output"];
        ofstream output(outputFile.c_str());
        output << arrays.str() << scripts.str() << modules.str();
        if( output.rdstate() & output.failbit )
            fatal( "could not write to '%s'", outputFile.c_str() );
    }

}
