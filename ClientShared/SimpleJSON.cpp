#include "SimpleJSON.h"
#include "rapidjson/document.h"
#include <sstream>

// used for the old parser
#ifndef RBX_BOOTSTRAPPER_MAC
#include "boost/algorithm/string.hpp"
#endif

void SimpleJSON::ReadFromStream(const char *stream)
{
    rapidjson::Document root;
    root.Parse<0>(stream);
    if (root.GetParseError())
    {
        // RapidJSON actually crashes if you attempt to iterate over an invalid document.
        size_t errOffset = root.GetErrorOffset();
        const char* errMessage = root.GetParseError();
        std::stringstream msg("SimpleJson, @");
        msg << errOffset << ": " << errMessage;
        _error = true;
        _errorString = std::string(msg.str());
        return;
    }
    for(rapidjson::Value::MemberIterator it = root.MemberBegin(); it != root.MemberEnd(); ++it)
    {
        std::string valueName(it->name.GetString());
        std::stringstream valueData;

        // (only IsString was originally supported by the original SimpleJSON)
        if (it->value.IsString())
        {
            valueData << it->value.GetString();
        }
        else if (it->value.IsInt())
        {
            valueData << it->value.GetInt();
        }
        else if (it->value.IsBool())
        {
            valueData << (it->value.GetBool() ? "True" : "False");
        }
        else
        {
            // error
            continue;
        }
		parser dataParser = _propValues[valueName];
		if (dataParser != NULL)
		{
			dataParser(valueData.str().c_str());
		}
		else 
		{
			DefaultHandler(valueName, valueData.str());
		}
    }
}

bool SimpleJSON::ParseBool(const char* value)
{
	return (strcmp(value, "true") == 0 || strcmp(value, "True") == 0) ? true : false;
}


// Below is the old version that will be kept around for a while.
// 

static const char *ValueEnclosure = "\"";
static const char LineBreak = 10;

static bool needTrimSymbol(char c)
{
	return (c == ValueEnclosure[0]) || c==LineBreak || c==' ' || c == '\t' || c=='\n' || c=='\r';
}

#ifdef RBX_BOOTSTRAPPER_MAC
static void trim_str_left(std::string &s)
{
    std::string::iterator iter = s.begin();
    for (;iter!=s.end();++iter)
    {
        if (!needTrimSymbol(*iter))
            break;
    }
    s.erase(s.begin(), iter);
}
static void trim_str_right(std::string &s)
{
    std::string::iterator iter = s.end();
    for (;iter!=s.begin();)
    {
      if (!needTrimSymbol(*(--iter)))
      {
          ++iter;
          break;
      }
    }
    s.erase(iter, s.end());
}
static void trim_str(std::string &s)
{
    try
    {
        trim_str_left(s);
        trim_str_right(s);
    }
    catch (...)
    {
        
    }
}
#endif
