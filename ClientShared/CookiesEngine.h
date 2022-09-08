#pragma once

#include <string>
#include <map>
#include <fstream>

class CookiesEngine
{
	std::wstring fileName;
	void ParseFileContent(std::fstream &f);
	void FlushContent(std::fstream &f);
	std::map<std::string, std::string> values;
public:
	static std::wstring getCookiesFilePath();
	static void setCookiesFilePath(std::wstring &path);
	static bool reportValue(CookiesEngine &engine, std::string key, std::string value);
	

	CookiesEngine(std::wstring path);
	~CookiesEngine() {}

	int SetValue(std::string key, std::string value);
	std::string GetValue(std::string key, int *errorCode, bool *exists);
	int DeleteValue(std::string key);
};