#include "stdafx.h"

#include <boost/function.hpp>
#include <boost/bind.hpp>

#include "FileDeployer.h"
#include "FileSystem.h"
#include "SharedHelpers.h"

#include "AtlPath.h"
#include "AtlFile.h"
#include "atlutil.h"
#include "MD5Hasher.h"
#include "HttpTools.h"
#include "Unzipper.h"

FileDeployer::FileDeployer(IInstallerSite *site, bool perUser)
: _site(site), _perUser(perUser), _bytesDownloaded(0)
{
}

// Downloads a file from the install site. Uses a cached file when possible
std::wstring FileDeployer::downloadVersionedFile(const TCHAR* name, Progress& progress, bool usePrimaryCdn)
{
	std::wstring downloadsDirectory = FileSystem::getSpecialFolder(_perUser ? FileSystem::RobloxUserApplicationData : FileSystem::RobloxCommonApplicationData, true, "Downloads");
	if (downloadsDirectory.empty())
		throw std::runtime_error("Failed to create Downloads folder"); 

	CRegKey key = CreateKey(_perUser ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, _T("Software\\RobloxReg\\ETags"));

	std::string etag = QueryStringValue(key, name);

	std::wstring oldFileName;
	if (!etag.empty())
	{
		// Make sure the cached file hasn't been deleted
		// TODO: Lock the file so it can't be deleted!
		oldFileName = downloadsDirectory + convert_s2w(etag);
		if (!ATLPath::FileExists(oldFileName.c_str()))
		{
			etag.clear();
		}
		else
		{
			try
			{
				MD5Hasher hasher;
				hasher.addData(std::ifstream(oldFileName.c_str(), std::ios::binary));
				std::string hash = hasher.toString();
				if (hash!=etag)
				{
					LLOG_ENTRY3(_site->Logger(), "WARNING: etag and hash don't match for %S: etag=%s hash=%s", name, etag.c_str(), hash.c_str());
					etag.clear();
					::DeleteFile(oldFileName.c_str());
				}
			}
			catch (std::exception& e)
			{
				LLOG_ENTRY2(_site->Logger(), "WARNING: Failed to get MD5 hash for %S: %s", oldFileName.c_str(), exceptionToString(e).c_str());
				etag.clear();
			}
		}
	}

	if (etag.empty())
		oldFileName.clear();

	std::wstring tempFileName = simple_logger<wchar_t>::get_temp_filename(_T("tmp"));
	int status_code;
	{
		std::ofstream result(tempFileName.c_str(), std::ios::binary);
		std::string path;
		if (_site->InstallVersion().empty())
		{
			path = format_string("/%S", name);
		}
		else
		{
			path = format_string("/%s-%S", _site->InstallVersion().c_str(), name);
		}

		if(usePrimaryCdn)
		{
			status_code = HttpTools::httpGetCdn(_site, _site->InstallHost(), path, etag, result, false, boost::bind(&Progress::update, &progress, _site, _1, _2));
		}
		else
		{
			status_code = HttpTools::httpGet(_site, _site->InstallHost(), path, etag, result, false, boost::bind(&Progress::update, &progress, _site, _1, _2));
		}
	}

	_site->CheckCancel();

	std::wstring fileName = downloadsDirectory + convert_s2w(etag);

	// WinInet converts a 304 code to a 200 code, so ignore status_code and check for change in etag
	if (oldFileName != fileName)
	{
		// New file. Cache it please
		if (!oldFileName.empty())
		{
			if (!::DeleteFile(oldFileName.c_str()))
			{
				if (::GetLastError() != ERROR_FILE_NOT_FOUND)
				{
					LLOG_ENTRY2(_site->Logger(), "Failed to delete old %S: %S", oldFileName.c_str(), (LPCTSTR)GetLastErrorDescription());
				}
			}
		}

		if (!::DeleteFile(fileName.c_str()))
		{
			if (::GetLastError() != ERROR_FILE_NOT_FOUND)
			{
				LLOG_ENTRY2(_site->Logger(), "Failed to delete %S: %S", fileName.c_str(), (LPCTSTR)GetLastErrorDescription());
				return tempFileName;
			}
		}

		// try to read downloaded size for logging
		WIN32_FILE_ATTRIBUTE_DATA fileAttributeData;
		if (GetFileAttributesEx(tempFileName.c_str(), GetFileExInfoStandard, &fileAttributeData)) {
			_bytesDownloaded += fileAttributeData.nFileSizeLow;
		}

		copyFile(_site->Logger(), tempFileName.c_str(), fileName.c_str());
		key.SetStringValue(name, convert_s2w(etag).c_str());
	}

	if (!::DeleteFile(tempFileName.c_str()))
	{
		LLOG_ENTRY2(_site->Logger(), "Failed to delete %S: %S", tempFileName.c_str(), (LPCTSTR)GetLastErrorDescription());
	}

	return fileName;
}

// Downloads a file from the install site. Uses a cached file when possible
void FileDeployer::deployVersionedFile(const TCHAR* name, const TCHAR* subfolder, Progress& progress, bool commitData)
{
	std::wstring path;
	std::wstring fileName;
	try
	{
		deployVersionedFileInternal(name, subfolder, progress, true /*usePrimaryCdn*/, path, fileName, commitData);
	}
	catch( CAtlException e )
	{
		_site->Error(std::runtime_error(format_string("AtlException 0x%x", e.m_hr)));
	}
	catch(silent_exception&)
	{
	}
	catch (std::exception& e)
	{
		LLOG_ENTRY3(_site->Logger(), "Failure when attempting to deploy %S from %S to %S", name, fileName.c_str(), path.c_str());
		cleanupVersionedFile(name,path,fileName);

		try
		{
			//Try to download/unzip one more time, this time do not use the CDN
			deployVersionedFileInternal(name, subfolder, progress, false /*usePrimaryCdn*/, path, fileName, commitData);
		}
		catch(std::exception&)
		{
			cleanupVersionedFile(name,path,fileName);

			//It failed again, so report the first error we got
			_site->Error(std::runtime_error(format_string("deployVersionedFile %S to \"%S\" failed: %s", name, path.c_str(), exceptionToString(e).c_str())));
		}
	}
	progress.done = true;
}

void FileDeployer::deployVersionedFileInternal(const TCHAR* name, const TCHAR* subfolder, Progress& progress, bool usePrimaryCdn,
											   /*output*/ std::wstring& path, /*output*/ std::wstring& fileName, bool commitData)
{
	path = _site->ProgramDirectory();
	if (subfolder)
		path += subfolder;

	LLOG_ENTRY1(_site->Logger(), "deployVersionedFile %S", name);

	fileName = downloadVersionedFile(name, progress, usePrimaryCdn);

	if (!ATLPath::FileExists(fileName.c_str()))
		throw std::runtime_error(format_string("File doesn't exist: %s", fileName.c_str()));

	{
		CAtlFile file;
		throwHRESULT(file.Create(fileName.c_str(), GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING), format_string("Failed to open %S", fileName.c_str()));
		LARGE_INTEGER len;
		if (::GetFileSizeEx(file, &len)) 
		{
			// I don't believe that we will ever have file bigger that 32 bits, so lets skip hi part of the size
			LLOG_ENTRY3(_site->Logger(), "File size is %d for %S from %S", len.LowPart, fileName.c_str(), name);
		} 
		else 
		{
			LLOG_ENTRY3(_site->Logger(), "Failed to get file size for %S: %S from %S", fileName.c_str(), (LPCTSTR)::GetLastErrorDescription(), name);
		}
	}

	_site->CheckCancel();

	if (commitData) 
	{
		CUnzipper unzipper(fileName.c_str());
		unzipper.skipExistingFiles = true;
		unzipper.UnzipTo(path.c_str());
	}
}

void FileDeployer::cleanupVersionedFile(const std::wstring& name, const std::wstring& path, const std::wstring& fileName)
{
	try
	{
		if(ATLPath::FileExists(fileName.c_str()))
		{
			::DeleteFile(fileName.c_str());
			if (::GetLastError() != ERROR_FILE_NOT_FOUND) 
			{
				LLOG_ENTRY2(_site->Logger(), "Failed to delete fileName(%S): %S", fileName.c_str(), (LPCTSTR)GetLastErrorDescription());
			}
		}

		if(ATLPath::FileExists(path.c_str()))
		{
			::DeleteFile(path.c_str());
			if (::GetLastError() != ERROR_FILE_NOT_FOUND) 
			{
				LLOG_ENTRY2(_site->Logger(), "Failed to delete path(%S): %S", path.c_str(), (LPCTSTR)GetLastErrorDescription());
			}
		}

		//Clear out the etag registry cache, just in case our file delete did not work
		CRegKey key = CreateKey(_perUser ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, _T("Software\\RobloxReg\\ETags"));
		key.SetStringValue(name.c_str(), _T(""));
	}
	catch(std::exception&)
	{
		LLOG_ENTRY3(_site->Logger(), "Error when trying to clean up the deploy of %S from %S to %S", name.c_str(), fileName.c_str(), path.c_str());
	}
}


