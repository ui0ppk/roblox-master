#include "CookiesEngine.h"
#include "../format_string.h"
#include <vector>

#define RBX_PLIST_KEY "CPath"
#define RBX_PLAYER_BUNDLE_ID "com.roblox.RobloxPlayer"
#include <CoreFoundation/CoreFoundation.h>

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
		::usleep(1000*50);
		rCounter --;
	}
	
	return rCounter >= 0 ? true : false;
}


std::wstring CookiesEngine::getCookiesFilePath()
{
	std::string path("");
	
	CFStringRef cookiePathKey = CFSTR(RBX_PLIST_KEY);
	CFStringRef playerBundleId = CFSTR(RBX_PLAYER_BUNDLE_ID);
	
	CFStringRef cookiePathValue = (CFStringRef)CFPreferencesCopyAppValue(cookiePathKey, playerBundleId);
	if (cookiePathValue != NULL)
	{
		CFIndex size = CFStringGetMaximumSizeForEncoding(CFStringGetLength(cookiePathValue), kCFStringEncodingUTF8);
		if (size > 0)
		{
			char *cookiePathString = (char *)malloc(sizeof(char) * (size + 1));
			if (CFStringGetCString(cookiePathValue, cookiePathString, size, kCFStringEncodingUTF8))
			{
				path = cookiePathString;
				
				//check if the directory exists
				std::string directoryPath = path.substr(0, path.rfind("/"));
				if (access(directoryPath.c_str(), F_OK) == -1)
				{
					char cmdBuffer[1024];
					snprintf(cmdBuffer, 1024, "mkdir -p %s", directoryPath.c_str());
					system(cmdBuffer);
				}
			}
            
			free(cookiePathString);
		}
        
		CFRelease(cookiePathValue);
	}
	
	return convert_s2w(path);
}

void CookiesEngine::setCookiesFilePath(std::wstring &path)
{
	// Cannot be done here for Mac. It's already too late to set the plist preferences if we're running the app
	// Instead, this is handled within the bootstrapper...
	//  at the time of writing this, that's within
	//  .../Client/Installer/BootstrapperMac/Roblox/controller.m::ModifyClientInfoList
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

	std::fstream f(convert_w2s(fileName).c_str());

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
    

	std::fstream f(convert_w2s(fileName).c_str());
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
    
	return valueResult;
}

int CookiesEngine::DeleteValue(std::string key)
{

	if (fileName.empty())
	{
		return -1;
	}
    

	std::fstream f(convert_w2s(fileName).c_str());
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
