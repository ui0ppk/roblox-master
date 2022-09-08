#pragma once
#include <string>
#include <map>

class SimpleJSON;
typedef void (*parser)(const char *stream);

#define START_DATA_MAP(className)			\
	static className *_thisPtr;					\
	void Init();								\
	className() {_thisPtr = this; Init(); }	\
	virtual ~className() {}				\

#define END_DATA_MAP()

#define DATA_MAP_IMPL_START(className)			\
	className *className::_thisPtr = NULL;			\
	void className::Init() {						\

#define DATA_MAP_IMPL_END()	\
	}							\

#define DECLARE_DATA_INT(name)								\
	private: \
	int _prop##name;											\
	public: \
	int GetValue##name() const { return _prop##name; }				\
	static void ReadValue##name(const char *stream) {			\
		_thisPtr->_prop##name = atoi(stream);					\
	}															\

#define DECLARE_DATA_BOOL(name)								\
	private: \
	bool _prop##name;											\
	public: \
	bool GetValue##name() const { return _prop##name; }				\
	static void ReadValue##name(const char *stream) {			\
	_thisPtr->_prop##name = ParseBool(stream); \
	}															\

#define DECLARE_DATA_STRING(name)									\
	private: \
	std::string _prop##name;											\
	public: \
	const char * GetValue##name() const { return _prop##name.c_str(); }		\
	static void ReadValue##name(const char *stream) {					\
	_thisPtr->_prop##name = std::string(stream);						\
	}																	\


#define IMPL_DATA(name,def)									\
	_prop##name = def;											\
	_propValues[std::string(#name)] = ReadValue##name;			\

class SimpleJSON
{
protected:
	std::map<std::string, parser> _propValues;
    bool _error;
    std::string _errorString;
public:
	SimpleJSON() : _error(false) {};
	virtual ~SimpleJSON() {};

	static bool ParseBool(const char* value);

	void ReadFromStream(const char *stream);

    bool GetError() const {return _error;}
    void ClearError() {_error = false;}
    std::string GetErrorString() const {return _errorString;}

protected:
	virtual bool DefaultHandler(const std::string& valueName, const std::string& valueData)
	{ return false; };
};
