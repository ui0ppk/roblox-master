#include "../CookiesEngine.h"
#include "../format_string.h"
#include <vector>
#include "AtlBase.h"
#include "AtlSync.h"

const TCHAR rbxRegPath[] = _T("Software\\ROBLOX Corporation\\Roblox");
const TCHAR rbxRegName[] = _T("CPath");
const TCHAR CookieFileMutext[] = _T("RobloxCookieEngineMutex");

namespace
{
	bool IsFileExists(const wchar_t* name)
	{
#ifdef UNICODE
		std::wstring fileName = name;
#else
		std::string fileName = convert_w2s(std::wstring(name));
#endif
		HANDLE file = CreateFile(fileName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		bool exists = false;
		if (file != INVALID_HANDLE_VALUE)
		{
			exists = true;
			CloseHandle(file);
		}
        
		return exists;
	}
}

bool CookiesEngine::reportValue(CookiesEngine &engine, std::string key, std::string value)
{
	int rCounter = 10;
	
	while (rCounter >= 0)
	{
		int result = engine.SetValue(key, value);
		if (result == 0)
		{
			break;
		}
		::Sleep(50);
		rCounter --;
	}
	
	return rCounter >= 0 ? true : false;
}


std::wstring CookiesEngine::getCookiesFilePath()
{
	CRegKey pathKey;
	TCHAR path[MAX_PATH];
	path[0] = 0;
    
	pathKey.Open(HKEY_CURRENT_USER, rbxRegPath);
	if (pathKey.m_hKey != NULL)
	{
        
		ULONG size = MAX_PATH;
		pathKey.QueryStringValue(rbxRegName, path, &size);
        
		pathKey.Close();
	}
#ifdef UNICODE
	return path;
#else
	return convert_s2w(path);
#endif
}

void CookiesEngine::setCookiesFilePath(std::wstring &path)
{
	CRegKey pathKey;
	pathKey.Create(HKEY_CURRENT_USER, rbxRegPath);
	if (pathKey.m_hKey != NULL)
	{
#ifdef UNICODE
		pathKey.SetStringValue(rbxRegName, path.c_str());
#else
		pathKey.SetStringValue(rbxRegName, convert_w2s(path).c_str());
#endif
        
		pathKey.Close();
	}
}


#pragma warning(disable:4996)

static char chars[] = {'&', '='};

CookiesEngine::CookiesEngine(std::wstring path)
{
	fileName = path;
	FILE *f = fopen(convert_w2s(path).c_str(), "a");
	if (f)
	{
		fclose(f);
	}
}

void CookiesEngine::ParseFileContent(std::fstream &f)
{
	f.seekg(0, std::ios::end);
	int len = (int)f.tellg();
	std::vector<char> buf(len + 1);
	f.seekg(0, std::ios::beg);
	f.read(&buf[0], len);
	buf[len] = 0;
	
	std::string data(&buf[0]);
    
	int s = 0;
	while(true)
	{
		int e = data.find(chars[0], s);
		if (e == -1)
		{
			break;
		}
        
		std::string pair = data.substr(s, e - s);
		if (!pair.empty())
		{
			int m = pair.find(chars[1]);
            
			std::string key = pair.substr(0, m);
			std::string value = pair.substr(m + 1, pair.length());
            
			values.insert(std::pair<std::string, std::string>(key, value));
		}
		s = e + 1;
	}
}

void CookiesEngine::FlushContent(std::fstream &f)
{
	std::map<std::string, std::string>::iterator i;
    
	f.seekg(0, std::ios::end);
	int len = (int)f.tellg();
	f.seekg(0, std::ios::beg);
	for (int k = 0; k < len;k ++)
	{
		f << " ";
	}
	f.flush();
    
	f.seekg(0, std::ios::beg);
	for (i = values.begin(); i != values.end(); i++)
	{
		f << (*i).first << chars[1] << (*i).second << chars[0];
	}
	f.flush();
}

int CookiesEngine::SetValue(std::string key, std::string value)
{
    
	if (fileName.empty())
	{
		return -1;
	}

	if (!IsFileExists(fileName.c_str()))
	{
		return -1;
	}
    
	CMutex m(NULL, FALSE, CookieFileMutext);
	CMutexLock lock(m);
    
	std::fstream f(fileName.c_str());

	if (!f.is_open())
	{
		return -1;
	}
    
	values.clear();
	ParseFileContent(f);
    
	values[key] = value;
	FlushContent(f);
    
	f.close();
    
	return 0;
}

std::string CookiesEngine::GetValue(std::string key, int *errorCode, bool *exists)
{
	if (fileName.empty())
	{
		*errorCode = -1;
		return std::string("");
	}
    
	*exists = false;
	*errorCode = 0;
    
	if (!IsFileExists(fileName.c_str()))
	{
		*errorCode = -1;
		return std::string("");
	}
    
	CMutex m(NULL, FALSE, CookieFileMutext);
	DWORD result = ::WaitForSingleObject(m, 1);
	if (result == WAIT_TIMEOUT)
	{
		return std::string("");
	}
	//WARNING, you can not return after this point without releasing mutex!
	//be careful with exceptions as well! ATL has no LockWrapper with TryLock in it
	
	std::fstream f(fileName.c_str());

	std::string valueResult;
    
	if (!f.is_open())
	{
		*errorCode = -1;
	}
	else
	{
		values.clear();
		ParseFileContent(f);
        
		std::map<std::string, std::string>::iterator i;
		for (i = values.begin(); i != values.end(); i++)
		{
			if ((*i).first == key)
			{
				*exists = true;
				valueResult = (*i).second;
				break;
			}
		}
        
		f.close();
	}
    
	m.Release();

	return valueResult;
}

int CookiesEngine::DeleteValue(std::string key)
{
    
	if (fileName.empty())
	{
		return -1;
	}
    
	if (!IsFileExists(fileName.c_str()))
	{
		return -1;
	}
    
	CMutex m(NULL, FALSE, CookieFileMutext);
	CMutexLock lock(m);
    
	std::fstream f(fileName.c_str());

	if (!f.is_open())
	{
		return -1;
	}
    
	values.clear();
	ParseFileContent(f);
    
	std::map<std::string, std::string>::iterator i;
	for (i = values.begin(); i != values.end(); i++)
	{
		if ((*i).first == key)
		{
			values.erase(i);
			FlushContent(f);
			break;
		}
	}
    
	f.close();
	return 0;
}

#pragma warning(default:4996)