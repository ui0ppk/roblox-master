// Unzipper.cpp: implementation of the CUnzipper class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Unzipper.h"

#include "zlib\unzip.h"
#include "zlib\iowin32.h"
#include <string>
#include "format_string.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

static void throwUz(int result, const char* error)
{
	if (result!=UNZ_OK)
		throw std::runtime_error(format_string("Failed %s: UZ=%d", error, result));
}

extern void throwLastError(BOOL result, const std::string& message);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

const UINT BUFFERSIZE = 20480;

CUnzipper::CUnzipper(LPCTSTR szFileName) : m_uzFile(0)
{
	m_szOutputFolder[0] = 0;

	if (szFileName)
	{
		// Because of users with non-ascii usernames, and
		// because unzOpen uses fopen(2), it is easiest
		// to get a short path and try to encode it.
		// The full path, even when encoded, doesn't work.
		LPTSTR szShortPath = (LPTSTR)malloc(MAX_PATH);
		GetShortPathName(szFileName, szShortPath, MAX_PATH);
		m_uzFile = unzOpen(convert_w2s(szShortPath).c_str());
		free(szShortPath);

		if (!m_uzFile)
			throw std::runtime_error(format_string("unzOpen failed for %S", szFileName));

		// set the default output folder
		TCHAR* szPath = StrDup(szFileName);

		// strip off extension
		TCHAR* p = StrRChr(szPath, NULL, _T('.'));

		if (p)
		{
			//////////
			// BUGFIX
			*p = '\\';
			++p;
			//////////
			*p = 0;
		}

		lstrcpy(m_szOutputFolder, szPath);

		LocalFree(szPath);
	}
}

CUnzipper::~CUnzipper()
{
	CloseZip();
}

BOOL CUnzipper::CloseZip()
{
	unzCloseCurrentFile(m_uzFile);

	int nRet = unzClose(m_uzFile);
	m_uzFile = NULL;

	return (nRet == UNZ_OK);
}

// simple interface
void CUnzipper::Unzip(BOOL bIgnoreFilePath)
{
	if (!m_uzFile)
		throw std::runtime_error("No file to unzip");

	UnzipTo(m_szOutputFolder, bIgnoreFilePath);
}

void CUnzipper::UnzipTo(LPCTSTR szFolder, BOOL bIgnoreFilePath)
{
	if (!m_uzFile)
		throw std::runtime_error("No file to unzip");

	if (!szFolder)
		throw std::runtime_error("No destination folder");

	if (!CreateFolder(szFolder))
		throw std::runtime_error("Failed to create destination folder");

	if (GetFileCount() == 0)
		throw std::runtime_error("file count is 0");

	if (!GotoFirstFile())
		throw std::runtime_error("failed to go to first file");

	// else
	do
	{
		UnzipFile(szFolder, bIgnoreFilePath);
	}
	while (GotoNextFile());
}

void CUnzipper::Unzip(LPCTSTR szFileName, LPCTSTR szFolder, BOOL bIgnoreFilePath)
{
	CloseZip();

	OpenZip(szFileName);

	UnzipTo(szFolder, bIgnoreFilePath);
}

// extended interface
void CUnzipper::OpenZip(LPCTSTR szFileName)
{
	CloseZip();

	if (szFileName)
	{
		m_uzFile = unzOpen(convert_w2s(szFileName).c_str());
		if (!m_uzFile)
			throw std::runtime_error("unzOpen failed");

		// set the default output folder
		TCHAR* szPath = StrDup(szFileName);

		// strip off extension
		TCHAR* p = StrRChr(szPath, NULL, _T('.'));

		if (p)
			*p = 0;

		lstrcpy(m_szOutputFolder, szPath);
		LocalFree(szPath);
	}
}


int CUnzipper::GetFileCount()
{
	if (!m_uzFile)
		return 0;

	unz_global_info info;

	if (unzGetGlobalInfo(m_uzFile, &info) == UNZ_OK)
	{
		return (int)info.number_entry;
	}

	return 0;
}

BOOL CUnzipper::GetFileInfo(int nFile, UZ_FileInfo& info)
{
	if (!m_uzFile)
		return FALSE;

	if (!GotoFile(nFile))
		return FALSE;

	return GetFileInfo(info);
}

void CUnzipper::UnzipFile(int nFile, LPCTSTR szFolder, BOOL bIgnoreFilePath)
{
	if (!m_uzFile)
		throw std::runtime_error("No file to unzip");

	if (!szFolder)
		szFolder = m_szOutputFolder;

	if (!GotoFile(nFile))
		throw std::runtime_error("GotoFile failed");

	UnzipFile(szFolder, bIgnoreFilePath);
}

BOOL CUnzipper::GotoFirstFile(LPCTSTR szExt)
{
	if (!m_uzFile)
		return FALSE;

	if (!szExt || !lstrlen(szExt))
		return (unzGoToFirstFile(m_uzFile) == UNZ_OK);

	// else
	if (unzGoToFirstFile(m_uzFile) == UNZ_OK)
	{
		UZ_FileInfo info;

		if (!GetFileInfo(info))
			return FALSE;

		// test extension
		TCHAR* pExt = StrRChr(info.szFileName, NULL, _T('.'));

		if (pExt)
		{
			pExt++;

			if (lstrcmpi(szExt, pExt) == 0)
				return TRUE;
		}

		return GotoNextFile(szExt);
	}

	return FALSE;
}

BOOL CUnzipper::GotoNextFile(LPCTSTR szExt)
{
	if (!m_uzFile)
		return FALSE;

	if (!szExt || !lstrlen(szExt))
		return (unzGoToNextFile(m_uzFile) == UNZ_OK);

	// else
	UZ_FileInfo info;

	while (unzGoToNextFile(m_uzFile) == UNZ_OK)
	{
		if (!GetFileInfo(info))
			return FALSE;

		// test extension
		TCHAR* pExt = StrRChr(info.szFileName, NULL, _T('.'));

		if (pExt)
		{
			pExt++;

			if (lstrcmpi(szExt, pExt) == 0)
				return TRUE;
		}
	}

	return FALSE;

}

BOOL CUnzipper::GetFileInfo(UZ_FileInfo& info)
{
	if (!m_uzFile)
		return FALSE;

	unz_file_info uzfi;

	ZeroMemory(&info, sizeof(info));
	ZeroMemory(&uzfi, sizeof(uzfi));

	char strName[MAX_PATH + 1] = {0};
	std::string inputString = convert_w2s(info.szFileName);
	if (!inputString.empty())
	{
		strcpy_s(strName, MAX_PATH, inputString.c_str());
	}

	if (UNZ_OK != unzGetCurrentFileInfo(m_uzFile, &uzfi, strName, MAX_PATH, NULL, 0, const_cast<char*>(convert_w2s(info.szComment).c_str()), MAX_COMMENT))
		return FALSE;

	if (strlen(strName) > 0)
	{
		std::wstring outString(convert_s2w(strName));
		StrCpy(info.szFileName, outString.c_str());
	}

	// copy across
	info.dwVersion = uzfi.version;	
	info.dwVersionNeeded = uzfi.version_needed;
	info.dwFlags = uzfi.flag;	
	info.dwCompressionMethod = uzfi.compression_method; 
	info.dwDosDate = uzfi.dosDate;  
	info.dwCRC = uzfi.crc;	 
	info.dwCompressedSize = uzfi.compressed_size; 
	info.dwUncompressedSize = uzfi.uncompressed_size;
	info.dwInternalAttrib = uzfi.internal_fa; 
	info.dwExternalAttrib = uzfi.external_fa; 

	// replace filename forward slashes with backslashes
	int nLen = lstrlen(info.szFileName);

	while (nLen--)
	{
		if (info.szFileName[nLen] == '/')
			info.szFileName[nLen] = '\\';
	}

	// is it a folder?
	info.bFolder = (info.szFileName[lstrlen(info.szFileName) - 1] == '\\');

	return TRUE;
}

void CUnzipper::UnzipFile(LPCTSTR szFolder, BOOL bIgnoreFilePath)
{
	if (!m_uzFile)
		throw std::runtime_error("No file to unzip");

	if (!szFolder)
		szFolder = m_szOutputFolder;

	if (!CreateFolder(szFolder))
		throw std::runtime_error("failed CreateFolder");

	UZ_FileInfo info;
	GetFileInfo(info);

	// if the item is a folder then simply return 
	if (info.szFileName[lstrlen(info.szFileName) - 1] == '\\')
		return;

	// build the output filename
	TCHAR szFilePath[MAX_PATH];
	lstrcpy(szFilePath, szFolder);

	// append backslash
	if (szFilePath[lstrlen(szFilePath) - 1] != '\\')
		lstrcat(szFilePath, _T("\\"));

	if (bIgnoreFilePath)
	{
		TCHAR* p = StrRChr(info.szFileName, NULL, _T('\\'));

		if (p)
			lstrcpy(info.szFileName, p + 1);
	}

	lstrcat(szFilePath, info.szFileName);

	// open the input and output files
	if (!CreateFilePath(szFilePath))
		throw std::runtime_error("failed CreateFilePath");

	if (skipExistingFiles)
		if (!::DeleteFile(szFilePath))
			if (::GetLastError()!=ERROR_FILE_NOT_FOUND)
				return;

	{
		CHandle hOutputFile(::CreateFile(szFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));

		throwLastError(INVALID_HANDLE_VALUE != hOutputFile, "failed CreateFile");

		throwUz(unzOpenCurrentFile(m_uzFile), "unzOpenCurrentFile");
		try
		{
			// read the file and output
			int nRet = UNZ_OK;
			char pBuffer[BUFFERSIZE];

			do
			{
				nRet = unzReadCurrentFile(m_uzFile, pBuffer, BUFFERSIZE);

				if (nRet > 0)
				{
					// output
					DWORD dwBytesWritten = 0;
					throwLastError(::WriteFile(hOutputFile, pBuffer, nRet, &dwBytesWritten, NULL), "WriteFile error");
					if (dwBytesWritten != (DWORD)nRet)
						throw std::runtime_error("dwBytesWritten != nRet");
				}
			}
			while (nRet > 0);
		}
		catch (...)
		{
			unzCloseCurrentFile(m_uzFile);
			throw;
		}
		unzCloseCurrentFile(m_uzFile);
	}

	try
	{
	SetFileModTime(szFilePath, info.dwDosDate);
	}
	catch (...)
	{
	}
}

BOOL CUnzipper::GotoFile(int nFile)
{
	if (!m_uzFile)
		return FALSE;

	if (nFile < 0 || nFile >= GetFileCount())
		return FALSE;

	GotoFirstFile();

	while (nFile--)
	{
		if (!GotoNextFile())
			return FALSE;
	}

	return TRUE;
}

BOOL CUnzipper::GotoFile(LPCTSTR szFileName, BOOL bIgnoreFilePath)
{
	if (!m_uzFile)
		return FALSE;

	// try the simple approach
	if (unzLocateFile(m_uzFile, convert_w2s(szFileName).c_str(), 2) == UNZ_OK)
		return TRUE;

	else if (bIgnoreFilePath)
	{ 
		// brute force way
		if (unzGoToFirstFile(m_uzFile) != UNZ_OK)
			return FALSE;

		UZ_FileInfo info;

		do
		{
			if (!GetFileInfo(info))
				return FALSE;

			// test name
			TCHAR* pName = StrRChr(info.szFileName, NULL, _T('\\'));

			if (pName)
			{
				pName++;

				if (lstrcmpi(szFileName, pName) == 0)
					return TRUE;
			}
		}
		while (unzGoToNextFile(m_uzFile) == UNZ_OK);
	}

	// else
	return FALSE;
}

BOOL CUnzipper::CreateFolder(LPCTSTR szFolder)
{
	if (!szFolder || !lstrlen(szFolder))
		return FALSE;

	DWORD dwAttrib = GetFileAttributes(szFolder);

	// already exists ?
	if (dwAttrib != 0xffffffff)
		return ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY);

	// recursively create from the top down
	TCHAR* szPath = StrDup(szFolder);
	TCHAR* p = StrRChr(szPath, NULL, _T('\\'));

	if (p) 
	{
		// The parent is a dir, not a drive
		*p = '\0';
			
		// if can't create parent
		if (!CreateFolder(szPath))
		{
			LocalFree(szPath);
			return FALSE;
		}
		LocalFree(szPath);

		if (!::CreateDirectory(szFolder, NULL)) 
			return FALSE;
	}
	
	return TRUE;
}

BOOL CUnzipper::CreateFilePath(LPCTSTR szFilePath)
{
	TCHAR* szPath = StrDup(szFilePath);
	TCHAR* p = StrRChr(szPath, NULL, _T('\\'));

	BOOL bRes = FALSE;

	if (p)
	{
		*p = '\0';

		bRes = CreateFolder(szPath);
	}

	LocalFree(szPath);

	return bRes;
}

void CUnzipper::SetFileModTime(LPCTSTR szFilePath, DWORD dwDosDate)
{
	CHandle hFile(CreateFile(szFilePath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL));

	throwLastError(hFile!=INVALID_HANDLE_VALUE, format_string("SetFileModTime couldn't open %S", szFilePath));
	
	FILETIME ftm, ftLocal, ftCreate, ftLastAcc, ftLastWrite;

	throwLastError(GetFileTime(hFile, &ftCreate, &ftLastAcc, &ftLastWrite), format_string("SetFileModTime failed GetFileTime: %S", szFilePath));

	throwLastError(DosDateTimeToFileTime((WORD)(dwDosDate >> 16), (WORD)dwDosDate, &ftLocal), format_string("SetFileModTime failed DosDateTimeToFileTime: %S", szFilePath));

	throwLastError(LocalFileTimeToFileTime(&ftLocal, &ftm), format_string("SetFileModTime failed LocalFileTimeToFileTime: %S", szFilePath));

	throwLastError(SetFileTime(hFile, &ftm, &ftLastAcc, &ftm), format_string("SetFileModTime failed SetFileTime: %S", szFilePath));
}

