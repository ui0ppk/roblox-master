#include "StdAfx.h"
#include "SharedHelpers.h"
#include "format_string.h"
#include "atlutil.h"
#include <strstream>
#include <vector>
#include <shlguid.h>
#include <shobjidl.h>
#include "FileSystem.h"

void throwHRESULT(HRESULT hr, const char* message)
{
	if (FAILED(hr))
		throw std::runtime_error(format_string("%s: %S", message, (LPCTSTR)AtlGetErrorDescription(hr)));
}

void throwHRESULT(HRESULT hr, const std::string& message)
{
	throwHRESULT(hr, message.c_str());
}

void throwLastError(BOOL result, const std::string& message)
{
	if (!result)
		throwHRESULT(HRESULT_FROM_WIN32(GetLastError()), message.c_str());
}

CString GetLastErrorDescription()
{
	HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
	CString s = AtlGetErrorDescription(hr);
	if (s.Find(_T("HRESULT"))!=0)
	{
		CString str;
		str.Format(_T(" (HRESULT 0x%8.8x)"), hr);
		s += str;
	}
	return s;
}

std::string QueryStringValue(CRegKey& key, const TCHAR* name)
{
	TCHAR value[64];
	ULONG length = 64;
	if (ERROR_SUCCESS == key.QueryStringValue(name, value, &length))
		return convert_w2s(value);
	else
		return "";
}

CRegKey CreateKey(HKEY parent, const TCHAR* name, const TCHAR* defaultValue, bool is64bits)
{
	//log << "CreateKey " << name << '\n'; log.flush();
	CRegKey key;
	REGSAM regSam = KEY_READ | KEY_WRITE;
	if (is64bits)
		regSam |= KEY_WOW64_64KEY;

	throwHRESULT(key.Create(parent, name, REG_NONE, REG_OPTION_NON_VOLATILE, regSam), format_string("Failed to create key %S", name));
	if (defaultValue)
	{
		//log << format_string("set '%s' default value to '%s'\n", name, defaultValue); log.flush();
		throwHRESULT(key.SetStringValue(NULL, defaultValue), format_string("Failed to set default value for key %S to %S", name, defaultValue));
	}
	return key;
}

void DeleteKey(simple_logger<wchar_t> &logger, HKEY parent, const TCHAR* name)
{
	if (!name)
	{
		throw std::runtime_error("Attempt to delete a null key");
	}

	if (std::wstring(name).empty())
	{
		throw std::runtime_error("Attempt to delete an empty key");
	}

	if (FAILED(::SHDeleteKey(parent, name)))
	{
		LLOG_ENTRY2(logger, "Failed to delete key %S %S", name, (LPCTSTR)GetLastErrorDescription());
	}
}

void DeleteSubKey(simple_logger<wchar_t> &logger, HKEY parent, const TCHAR* child, const TCHAR* key)
{
	std::wstring s = key;
	if (s.empty())
	{
		throw std::runtime_error("DeleteSubkey empty");
	}

	CRegKey k;
	if (!FAILED(k.Open(parent, child)))
	{
		LLOG_ENTRY2(logger, "Delete %S\\%S", child, key);
		DeleteKey(logger, k, key);
	}
}

std::string exceptionToString(const _com_error& e, int stage)
{
	CString desc = CString(e.Description().GetBSTR());
	CString errorMessage = CString(e.ErrorMessage());
	CString source = CString(e.Source().GetBSTR());
	std::ostrstream message;
	message << "_com_error stage=" << stage << " error=" << e.Error() 
		<< " message='" << errorMessage.GetString() << "' desc='" << desc.GetString() << "'"
		<< " source='" << source.GetString() << "'";
	return message.str();
}

std::string exceptionToString(const std::exception& e)
{
	return e.what();
}

void copyFile(simple_logger<wchar_t> &logger, const TCHAR* srcFile, const TCHAR* dstFile)
{
	LOG_ENTRY2("Bootstrapper::copyFile src = %S, dst = %S", srcFile, dstFile);
	//log << "copyFile(\"" << srcFile << "\", \"" << dstFile << "\")\n"; log.flush();
	std::ifstream src(srcFile, std::ios::binary);
	std::ofstream dst(dstFile, std::ios::binary);

	if(src.fail()){
		throw std::exception(format_string("Error opening srcFile='%S' for copy",srcFile).c_str());
	}
	if(dst.fail()){
		throw std::exception(format_string("Error opening dstFile='%S' for copy",dstFile).c_str());
	}


	src.seekg(0, std::ifstream::end);
	if((src.rdstate() & (std::ifstream::failbit | std::ifstream::badbit)) != 0){
		throw std::exception(format_string("Error with seekg to end on srcFile='%S' during copy",srcFile).c_str());
	}


	size_t size = src.tellg();
	if(size == -1){
		throw std::exception(format_string("Error with tellg on srcFile='%S' during copy",srcFile).c_str());
	}

	src.seekg(0);
	if((src.rdstate() & (std::ifstream::failbit | std::ifstream::badbit)) != 0){
		throw std::exception(format_string("Error with seekg to beginning on srcFile='%S' during copy",srcFile).c_str());
	}


	//log << "new char [" << size << "]\n"; log.flush();
	// allocate memory for file content
	std::vector<char> buffer(size);

	//log << "src.read\n"; log.flush();
	// read content of infile
	src.read(&buffer[0], size);

	if(src.fail()){
		throw std::exception(format_string("Error reading srcFile='%S' during copy",srcFile).c_str());
	}

	//log << "dst.write\n"; log.flush();
	// write to outfile
	dst.write(&buffer[0], size);

	if(dst.fail()){
		throw std::exception(format_string("Error writing dstFile='%S' during copy",dstFile).c_str());
	}

	src.close();
	if(src.fail()){
		throw std::exception(format_string("Error closing srcFile='%S' during copy",srcFile).c_str());
	}

	dst.close();
	if(dst.fail()){
		throw std::exception(format_string("Error closing dstFile='%S' during copy",dstFile).c_str());
	}
}

static std::wstring buildVersionKey(const TCHAR* componentCode)
{
	return std::wstring(_T("cur")) + componentCode + std::wstring(_T("Ver"));
}

static std::wstring buildUrlKey(const TCHAR* componentCode)
{
	return std::wstring(_T("cur")) + componentCode + std::wstring(_T("Url"));
}

void deleteCurVersionKeys(simple_logger<wchar_t> &logger, bool isPerUser, const TCHAR *component)
{
	CRegKey key;
	LOG_ENTRY("deleteCurVersionKeys");
	if(!FAILED(key.Open(isPerUser ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, _T("Software\\ROBLOX Corporation\\Roblox"), KEY_WRITE)))
	{
		LOG_ENTRY("deleteCurVersionKeys - key Opened");
		key.DeleteValue(buildVersionKey(component).c_str());
		key.DeleteValue(buildUrlKey(component).c_str());
	}
}

void setCurrentVersion(simple_logger<wchar_t> &logger, bool isPerUser, const TCHAR* componentCode, const TCHAR* version, const TCHAR* baseUrl)
{
	CRegKey key;
	LOG_ENTRY3("setCurrentVersion - opening write registry key component=%S, version=%S, url=%S", componentCode, version, baseUrl);
	if (ERROR_SUCCESS == key.Create(isPerUser ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, _T("Software\\ROBLOX Corporation\\Roblox")))
	{
		std::wstring vKey = buildVersionKey(componentCode);
		std::wstring uKey = buildUrlKey(componentCode);

		LOG_ENTRY3("Writing current version to registry for %S, version = %S, key = %S", componentCode, version, vKey.c_str());
		LONG result = key.SetStringValue(vKey.c_str(), version);
		LOG_ENTRY1("Succcess = %d", result == ERROR_SUCCESS);

		LOG_ENTRY3("Writing current url to registry for %S, url = %S, key = %S", componentCode, baseUrl, uKey.c_str());
		result = key.SetStringValue(uKey.c_str(), baseUrl);
		LOG_ENTRY1("Succcess = %d", result == ERROR_SUCCESS);
	}
}

void getCurrentVersion(simple_logger<wchar_t> &logger, bool isPerUser, const TCHAR* componentCode, TCHAR* version, int vBufSize, TCHAR* baseUrl, int uBaseSize)
{
	CRegKey key;
	LOG_ENTRY1("getCurrentVersion - opening read registry key component=%S", componentCode);
	version[0] = 0;
	baseUrl[0] = 0;
	if (ERROR_SUCCESS == key.Open(isPerUser ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, _T("Software\\ROBLOX Corporation\\Roblox"), KEY_READ))
	{
		std::wstring vKey = buildVersionKey(componentCode);
		std::wstring uKey = buildUrlKey(componentCode);

		LOG_ENTRY2("Reading current version from registry for %S, key = %S", componentCode, vKey.c_str());
		ULONG size = vBufSize;
		LONG result = key.QueryStringValue(vKey.c_str(), version, &size);
		LOG_ENTRY2("Success = %d, version = %S", result == ERROR_SUCCESS, version);

		LOG_ENTRY2("Reading current url from registry for %S, key = %S", componentCode, uKey.c_str());
		size = uBaseSize;
		result = key.QueryStringValue(uKey.c_str(), baseUrl, &size );
		LOG_ENTRY2("Success = %d, url = %S", result == ERROR_SUCCESS, baseUrl);
	}
}

std::wstring getPlayerInstallKey()
{
	return std::wstring(_T("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{373B1718-8CC5-4567-8EE2-9033AD08A680}"));
}

std::wstring getStudioInstallKey()
{
	return std::wstring(_T("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{B805FF17-92FE-4757-8142-F0A2850DFE03}"));
}

std::wstring getQTStudioInstallKey()
{
	return std::wstring(_T("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{2922D6F1-2865-4EFA-97A9-94EEAB3AFA14}"));
}

std::wstring getPlayerCode()
{
	return std::wstring(_T("Player"));
}

std::wstring getStudioCode()
{
	return std::wstring(_T("Studio"));
}

std::wstring getQTStudioCode()
{
	return std::wstring(_T("QTStudio"));
}

void appendEnvironmentToProtocolScheme(std::wstring& scheme, const std::string baseUrl)
{
	std::vector<std::string> baseHostUrlParts = splitOn(baseUrl, '.');
	if (baseHostUrlParts[1] != "roblox")
	{
		scheme += convert_s2w("-" + baseHostUrlParts[1]);
	}
}

std::wstring getPlayerProtocolScheme(const std::string& baseUrl)
{
	std::wstring scheme = _T("roblox-player");

	appendEnvironmentToProtocolScheme(scheme, baseUrl);

	return scheme;
}

std::wstring getQTStudioProtocolScheme(const std::string& baseUrl)
{
	std::wstring scheme = _T("roblox-studio");

	appendEnvironmentToProtocolScheme(scheme, baseUrl);

	return scheme;
}

std::wstring getStudioRegistrySubPath()
{
	return _T("StudioRobloxReg");
}

std::wstring getStudioRegistryPath()
{
	return _T("SOFTWARE\\") + getStudioRegistrySubPath();
}

std::wstring getQTStudioRegistrySubPath()
{
	return _T("StudioQTRobloxReg");
}

std::wstring getQTStudioRegistryPath()
{
	return _T("SOFTWARE\\") + getQTStudioRegistrySubPath();
}

static void createShortcut(const TCHAR *linkFileName, const TCHAR *exePath, const TCHAR *args)
{
	CComPtr<IShellLink> psl;
	throwHRESULT(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl), "CLSID_ShellLink failed");

	// Set the path to the shortcut target
	throwHRESULT(psl->SetPath(exePath), "psl->SetPath failed");

	if (args)
	{
		throwHRESULT(psl->SetArguments(args), "psl->SetArguments failed");
	}

	::DeleteFile(linkFileName);

	// Query IShellLink for the IPersistFile interface for saving 
	//the shortcut in persistent storage. 
	CComPtr<IPersistFile> ppf; 
	throwHRESULT(psl->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&ppf)), "IID_IPersistFile failed");

	// Save the link by calling IPersistFile::Save. 
	throwHRESULT(ppf->Save(linkFileName, TRUE), "IPersistFile Save failed");
}

void createRobloxShortcut(simple_logger<wchar_t> &logger, bool isPerUser, const TCHAR *linkFileName, const TCHAR *exePath, const TCHAR *args, bool desktop, bool forceCreate)
{
	std::wstring shortcutsDirectory;
	if (!desktop)
	{
		shortcutsDirectory = FileSystem::getSpecialFolder(isPerUser ? FileSystem::RobloxUserPrograms : FileSystem::RobloxCommonPrograms, true);
	}
	else
	{
		shortcutsDirectory = FileSystem::getSpecialFolder(FileSystem::Desktop, false, NULL, NULL);
	}

	if (shortcutsDirectory.empty())
	{
		throw std::runtime_error("Failed to create Shortcuts folder"); 
	}

	CString szFullLinkName;
	szFullLinkName.Format(_T("%s%s.lnk"), shortcutsDirectory.c_str(), linkFileName);

	BOOL exists = FileSystem::IsFileExists(szFullLinkName);
    LOG_ENTRY2("shortcut status: '%S', exists = %d", (LPCTSTR)szFullLinkName, exists);

	// if !exists mean that it is first install or user deleted shortcut, we want to update desktop shortcut only if it was there before us
	// or on first install
	if (forceCreate || exists)
	{
		LOG_ENTRY1("Creating shortcut: %S", (LPCTSTR)szFullLinkName);
		createShortcut(szFullLinkName, exePath, args);
	}
}

// isPerUser: whether to install per user or global for machine (unused now?)
// folder: where the shortcut files are located
// exeName: name of executable to which shortcut points
// exeFolderPath: path to exeName
// baseFolderPath: path to unversioned programs
void updateExistingRobloxShortcuts(
    simple_logger<wchar_t>& logger, 
    bool                    isPerUser, 
    const TCHAR*            folder, 
    const TCHAR*            exeName, 
    const TCHAR*            exeFolderPath, 
    const TCHAR*            baseFolderPath )
{
	LOG_ENTRY1("Searching for link files in : %S folder", folder);

	std::wstring searchPattern(folder);
	searchPattern += _T("*.lnk");

	WIN32_FIND_DATA findFileData;  
	HANDLE handle = INVALID_HANDLE_VALUE;
	handle = FindFirstFile(searchPattern.c_str(), &findFileData);
	if (handle == INVALID_HANDLE_VALUE)
		return; // unable to find this directory

	std::wstring newLink = std::wstring(exeFolderPath) + std::wstring(exeName);

	try
	{
		do
		{
			// skip folders
			if (findFileData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
				continue;

			CComPtr<IShellLink> psl;
			throwHRESULT(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl), "CLSID_ShellLink failed");

			CComPtr<IPersistFile> ppf; 
			throwHRESULT(psl->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&ppf)), "IID_IPersistFile failed");

			std::wstring fullName(folder);
			fullName += findFileData.cFileName;

			// Save the link by calling IPersistFile::Load. 
			throwHRESULT(ppf->Load(fullName.c_str(), STGM_READWRITE), "IPersistFile Load failed");
			LOG_ENTRY1("Link found: %S", fullName.c_str());

			TCHAR foundFilePath[MAX_PATH];
			throwHRESULT(psl->GetPath(foundFilePath, MAX_PATH, NULL, SLGP_RAWPATH), "psl->GetPath failed");

			if (StrStr(foundFilePath, exeName))
			{
				// the executable we're looking for
				throwHRESULT(psl->SetPath(newLink.c_str()), "psl->SetPath failed");
				LOG_ENTRY2("Updating link: %S with path: %S", fullName.c_str(), newLink.c_str());
				throwHRESULT(ppf->Save(fullName.c_str(), TRUE), "IPersistFile Save failed");
			}
			else
			{
				// even if we don't match, we might have to do some magic with old shortcuts that pointed to the player when
				// they wanted to launch studio (approx 04/2012 we only had one executable for player/studio)

				bool isMFCStudio = (StrCmp(exeName, _T(STUDIOBOOTSTAPPERNAME)) == 0); // true if we're updating for MFC studio
				LOG_ENTRY1("updateExistingRobloxShortcuts isStudio = %d", isMFCStudio);

				if (StrStr(foundFilePath, _T("Roblox.exe")))
				{
					// this shortcut points to the player
					TCHAR args[MAX_PATH];
					// detect if this shortcut is really a studio shortcut by looking at its arguments
					throwHRESULT(psl->GetArguments(args, MAX_PATH), "updateExistingRobloxShortcuts - GetArgument Failed");
					bool isIdeLink = (StrStr(args, _T("ide")) != 0);
					if (isIdeLink && isMFCStudio)
					{
						throwHRESULT(psl->SetPath(newLink.c_str()), "psl->SetPath failed");
						LOG_ENTRY2("Old studio link - updating link: %S with path: %S", fullName.c_str(), newLink.c_str());
						throwHRESULT(ppf->Save(fullName.c_str(), TRUE), "IPersistFile Save failed");
					}
					else if (!isIdeLink)
					{
						throwHRESULT(psl->SetPath(newLink.c_str()), "psl->SetPath failed");
						LOG_ENTRY2("Old player link - updating link: %S with path: %S", fullName.c_str(), newLink.c_str());
						throwHRESULT(ppf->Save(fullName.c_str(), TRUE), "IPersistFile Save failed");
					}
				} 
				else
				{
					// shortcut does not point to the player
					// rewrite studio shortcuts to point to the base folder instead of the versioned folder
					// in case the user pointed it directly to the exe instead of the bootstrapper
					std::vector<std::wstring> sbss;
					sbss.push_back(_T(STUDIOBOOTSTAPPERNAME));
					sbss.push_back(_T(STUDIOBOOTSTAPPERNAMEBETA));
					for (size_t i = 0;i < sbss.size();i ++)
					{
						if (StrStr(foundFilePath, sbss[i].c_str()))
						{
							LOG_ENTRY1("Studio bootstrapper found name = %S", sbss[i].c_str());
							std::wstring studioLink = std::wstring(baseFolderPath) + _T(STUDIOBOOTSTAPPERNAMEBETA); // All studios point to QT launcher
							throwHRESULT(psl->SetPath(studioLink.c_str()), "psl->SetPath failed");
							LOG_ENTRY2("Old studio link - updating link: %S with path: %S", fullName.c_str(), studioLink.c_str());
							throwHRESULT(ppf->Save(fullName.c_str(), TRUE), "IPersistFile Save failed");
							break;
						}
					}
				}
			}
		} while (FindNextFile(handle, &findFileData));

		FindClose(handle); 
	}
	catch (std::exception &)
	{
		FindClose(handle);
		throw;
	}
}

std::wstring getDektopShortcutPath(simple_logger<wchar_t> &logger, const TCHAR *shortcutName)
{
	std::wstring shortcutsDirectory = FileSystem::getSpecialFolder(FileSystem::Desktop, false, NULL, NULL);
	LOG_ENTRY1("hasDesktopShortcut - checking folder = %S", shortcutsDirectory.c_str());
	if (!shortcutsDirectory.empty())
	{
		CString szLinkName;
		szLinkName.Format(_T("%s%s.lnk"), shortcutsDirectory.c_str(), shortcutName);
		return std::wstring(szLinkName);
	}

	return std::wstring(_T(""));
}

bool hasDesktopShortcut(simple_logger<wchar_t> &logger, const TCHAR *shortcutName)
{
	LOG_ENTRY1("hasDesktopShortcut - %S", shortcutName);
	std::wstring shortcut = getDektopShortcutPath(logger, shortcutName);
	LOG_ENTRY1("hasDesktopShortcut - is empty path = %S", shortcut.c_str());
	if (!shortcut.empty())
	{
		LOG_ENTRY1("hasDesktopShortcut - checking file = %S", shortcut.c_str());
		return FileSystem::IsFileExists(shortcut.c_str());
	}
	return false;
}

std::wstring getRobloxProgramsFolder(simple_logger<wchar_t> &logger, bool isPerUser)
{
	return FileSystem::getSpecialFolder(isPerUser ? FileSystem::RobloxUserPrograms : FileSystem::RobloxCommonPrograms, false);
}

bool hasProgramShortcut(simple_logger<wchar_t> &logger, bool isPerUser, const TCHAR *shortcutName)
{
	LOG_ENTRY1("hasProgramShortcut - %S", shortcutName);

	std::wstring shortcutsDirectory = getRobloxProgramsFolder(logger, isPerUser);
	if (!shortcutsDirectory.empty())
	{
		CString szLinkName;
		szLinkName.Format(_T("%s%s.lnk"), shortcutsDirectory.c_str(), shortcutName);

		return FileSystem::IsFileExists(szLinkName);
	}

	return false;
}

bool deleteDesktopShortcut(simple_logger<wchar_t> &logger, const TCHAR *shortcutName)
{
	LOG_ENTRY1("deleteDesktopShortcut - %S", shortcutName);
	std::wstring shortcutsDirectory = FileSystem::getSpecialFolder(FileSystem::Desktop, false, NULL, NULL);
	if (!shortcutsDirectory.empty())
	{
		CString szLinkName;
		szLinkName.Format(_T("%s%s.lnk"), shortcutsDirectory.c_str(), shortcutName);

		BOOL exists = FileSystem::IsFileExists(szLinkName);
		if (exists)
		{
			LOG_ENTRY1("deleteDesktopShortcut - %S", (LPCTSTR)szLinkName);
			return ::DeleteFile(szLinkName) == TRUE;
		}
	}
	return false;
}

void deleteProgramsShortcut(simple_logger<wchar_t> &logger, bool isPerUser, const TCHAR *shortcutName)
{
	LOG_ENTRY1("deleteProgramsShortcut - %S", shortcutName);

	std::wstring shortcutsDirectory = FileSystem::getSpecialFolder(isPerUser ? FileSystem::RobloxUserPrograms : FileSystem::RobloxCommonPrograms, false);
	if (!shortcutsDirectory.empty())
	{
		CString szLinkName;
		szLinkName.Format(_T("%s%s.lnk"), shortcutsDirectory.c_str(), shortcutName);

		BOOL exists = FileSystem::IsFileExists(szLinkName);
		if (exists)
		{
			LOG_ENTRY1("deleteProgramsShortcut - %S", (LPCTSTR)szLinkName);
			::DeleteFile(szLinkName);
		}
	}
}