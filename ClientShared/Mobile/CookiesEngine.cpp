#include "../CookiesEngine.h"
#include "../format_string.h"

bool CookiesEngine::reportValue(CookiesEngine &engine, std::string key, std::string value)
{
    return false;
}


std::wstring CookiesEngine::getCookiesFilePath()
{
    std::string path("");
    return convert_s2w(path);
}

void CookiesEngine::setCookiesFilePath(std::wstring &path)
{
}


CookiesEngine::CookiesEngine(std::wstring path)
{
}

void CookiesEngine::ParseFileContent(std::fstream &f)
{
}

void CookiesEngine::FlushContent(std::fstream &f)
{
}

int CookiesEngine::SetValue(std::string key, std::string value)
{
    return -1;
}

std::string CookiesEngine::GetValue(std::string key, int *errorCode, bool *exists)
{
    return "";
}

int CookiesEngine::DeleteValue(std::string key)
{
    return -1;
}

