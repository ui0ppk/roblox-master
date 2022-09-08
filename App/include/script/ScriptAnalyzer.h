#pragma once

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include <string>
#include <vector>

struct lua_State;

namespace RBX
{
    class DataModel;
    class Instance;

    namespace ScriptAnalyzer
    {
        // Don't change codes for the existing warnings - they have a corresponding wiki anchor tag in http://wiki.roblox.com/index.php?title=Script_Analysis
        enum WarningCode
		{
            Warning_Unknown = 0,

            Warning_UnknownGlobal = 1,
            Warning_DeprecatedGlobal = 2,
            Warning_GlobalUsedAsLocal = 3,
            Warning_LocalShadow = 4,
            Warning_SameLineStatement = 5,
            Warning_MultiLineStatement = 6,
            Warning_UnknownType = 7,
            Warning_DotCall = 8,
            Warning_UnknownMember = 9,
            Warning_BuiltinGlobalWrite = 10,
            Warning_Placeholder = 11,
            
            Warning_Internal
		};
        
        struct Position
        {
            unsigned int line, column;

			Position(unsigned int line, unsigned int column)
				: line(line)
				, column(column)
			{
			}
        };
        
        struct Location
        {
            Position begin, end;

			Location()
				: begin(0, 0)
				, end(0, 0)
			{
			}

			Location(const Position& begin, const Position& end)
				: begin(begin)
				, end(end)
			{
			}

			Location(const Position& begin, unsigned int length)
				: begin(begin)
				, end(begin.line, begin.column + length)
			{
			}

			Location(const Location& begin, const Location& end)
				: begin(begin.begin)
				, end(end.end)
			{
			}
        };

        struct Error
        {
            Location location;
            std::string text;
        };

        struct Warning
        {
            WarningCode code;
            Location location;
            std::string text;

			Warning (WarningCode code, Location location, std::string text)
				: code(code)
				, location(location)
				, text(text)
			{}
        };

		struct IntellesenseResult
		{
			IntellesenseResult()
				: name("")
				, isLocal(false)
				, isFunction(false)
				, location(Position(0,0) , Position(0,0))
			{}
			
			std::string name;
			bool isLocal;
			bool isFunction;
			Location location;
			std::vector<IntellesenseResult> children;			
		};

        struct Result
        {
            boost::optional<Error> error;
            std::vector<Warning> warnings;
			std::vector<IntellesenseResult> intellesenseAnalysis;
        };

        Result analyze(DataModel* dm, shared_ptr<Instance> script, const std::string& code);
    };
}
