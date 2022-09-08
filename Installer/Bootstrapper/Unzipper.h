// Unzipper.h: interface for the CUnzipper class (c) daniel godson 2002
//
// CUnzipper is a simple c++ wrapper for the 'unzip' file extraction
// api written by Gilles Vollant (c) 2002, which in turn makes use of 
// 'zlib' written by Jean-loup Gailly and Mark Adler (c) 1995-2002.
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely without restriction.
//
// Notwithstanding this, you are still bound by the conditions imposed
// by the original authors of 'unzip' and 'zlib'
//////////////////////////////////////////////////////////////////////

/*
	Modified by Roblox Corporation in 2008
	Primarily replaced BOOL return arguments with throwing std::exception
*/

#if !defined(AFX_UNZIPPER_H__EBC42716_31C7_4659_8EF3_9BF8D4409709__INCLUDED_)
#define AFX_UNZIPPER_H__EBC42716_31C7_4659_8EF3_9BF8D4409709__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

const UINT MAX_COMMENT = 255;

// create our own fileinfo struct to hide the underlying implementation
struct UZ_FileInfo
{
	TCHAR szFileName[MAX_PATH + 1];
	TCHAR szComment[MAX_COMMENT + 1];
	
	DWORD dwVersion;  
	DWORD dwVersionNeeded;
	DWORD dwFlags;	 
	DWORD dwCompressionMethod; 
	DWORD dwDosDate;	
	DWORD dwCRC;   
	DWORD dwCompressedSize; 
	DWORD dwUncompressedSize;
	DWORD dwInternalAttrib; 
	DWORD dwExternalAttrib; 
	bool bFolder;
};

class CUnzipper  
{
public:
	bool skipExistingFiles;

	CUnzipper(LPCTSTR szFileName = NULL);
	virtual ~CUnzipper();
	
	// simple interface
	void Unzip(LPCTSTR szFileName, LPCTSTR szFolder = NULL, BOOL bIgnoreFilePath = FALSE);
	
	// works with prior opened zip
	void Unzip(BOOL bIgnoreFilePath = FALSE); // unzips to output folder or sub folder with zip name 
	void UnzipTo(LPCTSTR szFolder, BOOL bIgnoreFilePath = FALSE); // unzips to specified folder

	// extended interface
	void OpenZip(LPCTSTR szFileName);
	BOOL CloseZip(); // for multiple reuse
	
	// unzip by file index
	int GetFileCount();
	BOOL GetFileInfo(int nFile, UZ_FileInfo& info);
	void UnzipFile(int nFile, LPCTSTR szFolder = NULL, BOOL bIgnoreFilePath = FALSE);
	
	// unzip current file
	BOOL GotoFirstFile(LPCTSTR szExt = NULL);
	BOOL GotoNextFile(LPCTSTR szExt = NULL);
	BOOL GetFileInfo(UZ_FileInfo& info);
	void UnzipFile(LPCTSTR szFolder = NULL, BOOL bIgnoreFilePath = FALSE);

	// helpers
	BOOL GotoFile(LPCTSTR szFileName, BOOL bIgnoreFilePath = TRUE);
	BOOL GotoFile(int nFile);
	
protected:
	void* m_uzFile;
	TCHAR m_szOutputFolder[MAX_PATH + 1];

protected:
	BOOL CreateFolder(LPCTSTR szFolder);
	BOOL CreateFilePath(LPCTSTR szFilePath); // truncates from the last '\'
	void SetFileModTime(LPCTSTR szFilePath, DWORD dwDosDate);

};

#endif // !defined(AFX_UNZIPPER_H__EBC42716_31C7_4659_8EF3_9BF8D4409709__INCLUDED_)
