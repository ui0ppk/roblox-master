/*
Module : VersionInfo.H
Purpose: Interface for an MFC class encapsulation of Version Infos
Created: PJN / 10-04-2000

Copyright (c) 2000 by PJ Naughter.  
All rights reserved.

*/


/////////////////////////////// Defines ///////////////////////////////////////
#ifndef __VERSIONINFO_H__
#define __VERSIONINFO_H__

// NOTE: This class was refactored to use std::string instead of CString
#include <string>

//Pull in the win32 version Library
#pragma comment(lib, "version.lib")
 

/////////////////////////////// Classes ///////////////////////////////////////


class CVersionInfo
{
public:
  struct TRANSLATION
  {
    WORD m_wLangID;   //e.g. 0x0409 LANG_ENGLISH, SUBLANG_ENGLISH_USA
    WORD m_wCodePage; //e.g. 1252 Codepage for Windows:Multilingual
  };

//Constructors / Destructors
  CVersionInfo();
  ~CVersionInfo();

//methods:
  BOOL                          Load(HMODULE module);
  BOOL                          Load(const std::wstring& sFileName);
  VS_FIXEDFILEINFO*             GetFixedFileInfo();
  DWORD                         GetFileFlagsMask();
  DWORD                         GetFileFlags();
  DWORD                         GetOS();
  DWORD                         GetFileType();
  DWORD                         GetFileSubType();
  FILETIME                      GetCreationTime();
  unsigned __int64              GetFileVersion();
  unsigned __int64              GetProductVersion();
  std::string                   GetValue(const std::string& sKeyName);
  std::string                   GetComments();
  std::string                   GetCompanyName();
  std::string                   GetFileDescription();
  std::string                   GetFileVersionAsString();
  std::string                   GetFileVersionAsDotString();
  std::string                   GetInternalName();
  std::string                   GetLegalCopyright();
  std::string                   GetLegalTrademarks();
  std::string                   GetOriginalFilename();
  std::string                   GetPrivateBuild();
  std::string                   GetProductName();
  std::string                   GetProductVersionAsString();
  std::string                   GetSpecialBuild();
  int                           GetNumberOfTranslations();
  TRANSLATION*                  GetTranslation(int nIndex);
  void                          SetTranslation(int nIndex);
  
protected:
//Methods
  void Unload();

//Data
  WORD              m_wLangID;       //The current language ID of the resource
  WORD              m_wCharset;      //The current Character set ID of the resource
  LPVOID            m_pVerData;      //Pointer to the Version info blob
  TRANSLATION*      m_pTranslations; //Pointer to the "\\VarFileInfo\\Translation" version info
  int               m_nTranslations; //The number of translated version infos in the resource
  VS_FIXEDFILEINFO* m_pffi;          //Pointer to the fixed size version info data
};


#endif //__VERSIONINFO_H__

