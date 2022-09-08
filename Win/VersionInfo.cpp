/*
Module : VersionInfo.CPP
Purpose: Implementation for a MFC class encapsulation of Version Infos
Created: PJN / 10-04-2000
History: None


Copyright (c) 2000 by PJ Naughter.
All rights reserved.

*/

//////////////// Includes ////////////////////////////////////////////
#include "stdafx.h"
#include "VersionInfo.h"

#include "boost/tokenizer.hpp"
#include "format_string.h"
#include "StringConv.h"
//#include "FastLog.h"      Disable to get compiler working - RWM

//LOGGROUP(CrashReporterInit)

//////////////// Implementation //////////////////////////////////////

using RBX::SysPathString;
using RBX::utf8_decode;
using RBX::utf8_encode;

CVersionInfo::CVersionInfo()
{
    m_pVerData = NULL;
    m_pffi = NULL;
    m_wLangID = 0;
    m_wCharset = 1252; //Use the ANSI code page as a default
    m_pTranslations = NULL;
    m_nTranslations = 0;
}

CVersionInfo::~CVersionInfo()
{
    Unload();
}

void CVersionInfo::Unload()
{
    m_pffi = NULL;
    if (m_pVerData)
    {
        delete[] m_pVerData;
        m_pVerData = NULL;
    }
    m_wLangID = 0;
    m_wCharset = 1252; //Use the ANSI code page as a default
    m_pTranslations = NULL;
    m_nTranslations = 0;
}

BOOL CVersionInfo::Load(HMODULE module)
{
    WCHAR name[500];
    ::GetModuleFileNameW(module, name, 500);

    WCHAR path[_MAX_PATH];
    ::GetShortPathNameW(name, path, _MAX_PATH);

    return Load(SysPathString(path));
}

BOOL CVersionInfo::Load(const std::wstring& fileName)
{
    //Free up any previous memory lying around
    Unload();

    BOOL bSuccess = FALSE;
    DWORD dwHandle = 0;
    DWORD dwSize = GetFileVersionInfoSizeW(fileName.c_str(), &dwHandle);
    if (dwSize)
    {
        //  FASTLOGS(FLog::CrashReporterInit, "Loading module name: %s", fileName);
        // 	FASTLOG2(FLog::CrashReporterInit, "Handle: %p, Size: %u", dwHandle, dwSize);	  
        m_pVerData = new BYTE[dwSize];
        if (GetFileVersionInfoW(fileName.c_str(), dwHandle, dwSize, m_pVerData))
        {
            //Get the fixed size version info data
            UINT nLen = 0;
            if (VerQueryValue(m_pVerData, _T("\\"), (LPVOID*)&m_pffi, &nLen))
            {
                //Retrieve the Lang ID and Character set ID
                if (VerQueryValue(m_pVerData, _T("\\VarFileInfo\\Translation"), (LPVOID*)&m_pTranslations, &nLen) && nLen >= sizeof(TRANSLATION))
                {
                    m_nTranslations = nLen / sizeof(TRANSLATION);
                    m_wLangID = m_pTranslations[0].m_wLangID;
                    m_wCharset = m_pTranslations[0].m_wCodePage;
                }
                bSuccess = TRUE;
            }
        }
    }

    //Free up the memory we used
    if (!bSuccess)
    {
        if (m_pVerData)
        {
            delete[] m_pVerData;
            m_pVerData = NULL;
        }
    }

    return bSuccess;
}

VS_FIXEDFILEINFO* CVersionInfo::GetFixedFileInfo()
{
    return m_pffi;
}

DWORD CVersionInfo::GetFileFlagsMask()
{
    return m_pffi->dwFileFlagsMask;
}

DWORD CVersionInfo::GetFileFlags()
{
    return m_pffi->dwFileFlags;
}

DWORD CVersionInfo::GetOS()
{
    return m_pffi->dwFileOS;
}

DWORD CVersionInfo::GetFileType()
{
    return m_pffi->dwFileType;
}

DWORD CVersionInfo::GetFileSubType()
{
    return m_pffi->dwFileSubtype;
}

FILETIME CVersionInfo::GetCreationTime()
{
    FILETIME CreationTime;
    CreationTime.dwHighDateTime = m_pffi->dwFileDateMS;
    CreationTime.dwLowDateTime = m_pffi->dwFileDateLS;
    return CreationTime;
}


std::string CVersionInfo::GetValue(const std::string& sKey)
{

    //For the string to query with
    std::string sVal;
    std::string sQueryValue = format_string("\\StringFileInfo\\%04x%04x\\%s",
        m_wLangID, m_wCharset, sKey.c_str());

    //Do the query
    LPCTSTR pVal = NULL;
    UINT nLen = 0;
    if (VerQueryValue(m_pVerData, CVTS2W(sQueryValue).c_str(), (LPVOID*)&pVal, &nLen))
        sVal = CVTW2S(pVal);

    return sVal;
}

std::string CVersionInfo::GetCompanyName()
{
    return GetValue(CVTW2S(_T("CompanyName")));
}

std::string CVersionInfo::GetFileDescription()
{
    return GetValue(CVTW2S(_T("FileDescription")));
}

std::string CVersionInfo::GetFileVersionAsDotString()
{
    std::string v = GetFileVersionAsString();
    boost::tokenizer<boost::char_separator<char> > tokens(v, boost::char_separator<char>(" ,."));
    std::string dotVersion;
    for (boost::tokenizer<boost::char_separator<char> >::iterator tok_iter = tokens.begin();
        tok_iter != tokens.end(); ++tok_iter)
    {
        if (tok_iter != tokens.begin())
            dotVersion += ".";
        dotVersion += *tok_iter;
    }
    return dotVersion;
}

std::string CVersionInfo::GetFileVersionAsString()
{
    return GetValue(CVTW2S(_T("FileVersion")));
}

std::string CVersionInfo::GetInternalName()
{
    return GetValue(CVTW2S(_T("InternalName")));
}

std::string CVersionInfo::GetLegalCopyright()
{
    return GetValue(CVTW2S(_T("LegalCopyright")));
}

std::string CVersionInfo::GetOriginalFilename()
{
    return GetValue(CVTW2S(_T("OriginalFilename")));
}

std::string CVersionInfo::GetProductName()
{
    return GetValue(CVTW2S(_T("Productname")));
}

std::string CVersionInfo::GetProductVersionAsString()
{
    return GetValue(CVTW2S(_T("ProductVersion")));
}

int CVersionInfo::GetNumberOfTranslations()
{
    return m_nTranslations;
}

std::string CVersionInfo::GetComments()
{
    return GetValue(CVTW2S(_T("Comments")));
}

std::string CVersionInfo::GetLegalTrademarks()
{
    return GetValue(CVTW2S(_T("LegalTrademarks")));
}

std::string CVersionInfo::GetPrivateBuild()
{
    return GetValue(CVTW2S(_T("PrivateBuild")));
}

std::string CVersionInfo::GetSpecialBuild()
{
    return GetValue(CVTW2S(_T("SpecialBuild")));
}

CVersionInfo::TRANSLATION* CVersionInfo::GetTranslation(int nIndex)
{
    return &m_pTranslations[nIndex];
}

void CVersionInfo::SetTranslation(int nIndex)
{
    TRANSLATION* pTranslation = GetTranslation(nIndex);
    m_wLangID = pTranslation->m_wLangID;
    m_wCharset = pTranslation->m_wCodePage;
}
