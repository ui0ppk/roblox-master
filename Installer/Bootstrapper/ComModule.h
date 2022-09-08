#pragma once

#include <string>
#include <vector>
#include "format_string.h"

struct ComModule
{
private:
	typedef HRESULT (STDAPICALLTYPE *PFNREGISTERTYPELIB)(ITypeLib *, LPCOLESTR /* const szFullPath */, LPCOLESTR /* const szHelpDir */);
	static PFNREGISTERTYPELIB registerTypeLibFunc;
	static bool regPerUser;

public:
	ComModule();
	static bool Init(bool perUser);

	struct Interface
	{
		Interface(const char* name, const GUID& guid, bool isConnectionPoint);
		Interface(const char* name, const char* guid, bool isConnectionPoint):name(name),guid(guid),isConnectionPoint(isConnectionPoint) {}
		bool isConnectionPoint;
		std::string name;
		std::string guid;
	};
	typedef std::vector<Interface> Interfaces;
	std::string appName;
	std::wstring fileName;
	std::wstring clsid;
	Interfaces interfaces;
	std::string typeLib; 
	std::string typeLibVersion; 
	std::wstring appID;
	std::string progID;
	std::string versionIndependentProgID;
	std::wstring name;
	std::wstring moduleName;
	bool isDll;
	bool is64Bits;

	void registerModule(CRegKey &classesKey, std::wstring versionDir, simple_logger<wchar_t> &logger);
	void unregisterModule(HKEY ckey, bool isPerUser, simple_logger<wchar_t> &logger);
};
