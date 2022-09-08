#include "stdafx.h"
#include "Crypt.h"
#include "Util/MD5Hasher.h"
#include "shlwapi.h"
#pragma comment(lib, "crypt32.lib")

#pragma comment (lib, "wintrust")

bool VerifyEmbeddedSignature(LPCWSTR pwszSourceFile)
{
    LONG lStatus;
    DWORD dwLastError;

    // Initialize the WINTRUST_FILE_INFO structure.

    WINTRUST_FILE_INFO FileData;
    memset(&FileData, 0, sizeof(FileData));
    FileData.cbStruct = sizeof(WINTRUST_FILE_INFO);
    FileData.pcwszFilePath = pwszSourceFile;
    FileData.hFile = NULL;
    FileData.pgKnownSubject = NULL;

    /*
    WVTPolicyGUID specifies the policy to apply on the file
    WINTRUST_ACTION_GENERIC_VERIFY_V2 policy checks:
    
    1) The certificate used to sign the file chains up to a root 
    certificate located in the trusted root certificate store. This 
    implies that the identity of the publisher has been verified by 
    a certification authority.
    
    2) In cases where user interface is displayed (which this example
    does not do), WinVerifyTrust will check for whether the  
    end entity certificate is stored in the trusted publisher store,  
    implying that the user trusts content from this publisher.
    
    3) The end entity certificate has sufficient permission to sign 
    code, as indicated by the presence of a code signing EKU or no 
    EKU.
    */

    GUID WVTPolicyGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    WINTRUST_DATA WinTrustData;

    // Initialize the WinVerifyTrust input data structure.

    // Default all fields to 0.
    memset(&WinTrustData, 0, sizeof(WinTrustData));

    WinTrustData.cbStruct = sizeof(WinTrustData);
    
    // Use default code signing EKU.
    WinTrustData.pPolicyCallbackData = NULL;

    // No data to pass to SIP.
    WinTrustData.pSIPClientData = NULL;

    // Disable WVT UI.
    WinTrustData.dwUIChoice = WTD_UI_NONE;

    // No revocation checking.
    WinTrustData.fdwRevocationChecks = WTD_REVOKE_NONE; 

    // Verify an embedded signature on a file.
    WinTrustData.dwUnionChoice = WTD_CHOICE_FILE;

    // Default verification.
    WinTrustData.dwStateAction = 0;

    // Not applicable for default verification of embedded signature.
    WinTrustData.hWVTStateData = NULL;

    // Not used.
    WinTrustData.pwszURLReference = NULL;

    // Default.
    WinTrustData.dwProvFlags = WTD_SAFER_FLAG;

    // This is not applicable if there is no UI because it changes 
    // the UI to accommodate running applications instead of 
    // installing applications.
    WinTrustData.dwUIContext = 0;

    // Set pFile.
    WinTrustData.pFile = &FileData;

    // WinVerifyTrust verifies signatures as specified by the GUID 
    // and Wintrust_Data.
    lStatus = WinVerifyTrust(
        NULL,
        &WVTPolicyGUID,
        &WinTrustData);

    switch (lStatus) 
    {
        case ERROR_SUCCESS:
            /*
            Signed file:
                - Hash that represents the subject is trusted.

                - Trusted publisher without any verification errors.

                - UI was disabled in dwUIChoice. No publisher or 
                    time stamp chain errors.

                - UI was enabled in dwUIChoice and the user clicked 
                    "Yes" when asked to install and run the signed 
                    subject.
            */
            //wprintf_s(L"The file \"%s\" is signed and the signature "
            //    L"was verified.\n",
            //    pwszSourceFile);
            return true;
        
        case TRUST_E_NOSIGNATURE:
            // The file was not signed or had a signature 
            // that was not valid.

            // Get the reason for no signature.
            dwLastError = GetLastError();
            if (TRUST_E_NOSIGNATURE == dwLastError ||
                    TRUST_E_SUBJECT_FORM_UNKNOWN == dwLastError ||
                    TRUST_E_PROVIDER_UNKNOWN == dwLastError) 
            {
                // The file was not signed.
                //wprintf_s(L"The file \"%s\" is not signed.\n",
                //    pwszSourceFile);
				return false;
            } 
            else 
            {
                // The signature was not valid or there was an error 
                // opening the file.
                //wprintf_s(L"An unknown error occurred trying to "
                //    L"verify the signature of the \"%s\" file.\n",
                //    pwszSourceFile);
				return false;
            }

            break;

        case TRUST_E_EXPLICIT_DISTRUST:
            // The hash that represents the subject or the publisher 
            // is not allowed by the admin or user.
            //wprintf_s(L"The signature is present, but specifically "
            //    L"disallowed.\n");
			return true;

        case TRUST_E_SUBJECT_NOT_TRUSTED:
            // The user clicked "No" when asked to install and run.
            //wprintf_s(L"The signature is present, but not "
            //    L"trusted.\n");
			return true;

        case CRYPT_E_SECURITY_SETTINGS:
            /*
            The hash that represents the subject or the publisher 
            was not explicitly trusted by the admin and the 
            admin policy has disabled user trust. No signature, 
            publisher or time stamp errors.
            */
            //wprintf_s(L"CRYPT_E_SECURITY_SETTINGS - The hash "
            //    L"representing the subject or the publisher wasn't "
            //    L"explicitly trusted by the admin and admin policy "
            //    L"has disabled user trust. No signature, publisher "
            //    L"or timestamp errors.\n");
            return true;

        default:
            // The UI was disabled in dwUIChoice or the admin policy 
            // has disabled user trust. lStatus contains the 
            // publisher or time stamp chain error.
            //wprintf_s(L"Error is: 0x%x.\n",
            //    lStatus);
			return true;
    }

    return true;
}

#define ENCODING (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING)

typedef struct {
    LPWSTR lpszProgramName;
    LPWSTR lpszPublisherLink;
    LPWSTR lpszMoreInfoLink;
} SPROG_PUBLISHERINFO, *PSPROG_PUBLISHERINFO;

BOOL GetProgAndPublisherInfo(PCMSG_SIGNER_INFO pSignerInfo,
                             PSPROG_PUBLISHERINFO Info);
BOOL GetDateOfTimeStamp(PCMSG_SIGNER_INFO pSignerInfo, SYSTEMTIME *st);
bool VerifyCertificateInfo(PCCERT_CONTEXT pCertContext);
BOOL GetTimeStampSignerInfo(PCMSG_SIGNER_INFO pSignerInfo,
                            PCMSG_SIGNER_INFO *pCounterSignerInfo);

bool VerifyCryptSignature(const std::wstring& fileName)
{
	WCHAR szFileName[MAX_PATH]; 
    HCERTSTORE hStore = NULL;
    HCRYPTMSG hMsg = NULL; 
    PCCERT_CONTEXT pCertContext = NULL;
    BOOL fResult;   
    DWORD dwEncoding, dwContentType, dwFormatType;
    PCMSG_SIGNER_INFO pSignerInfo = NULL;
    PCMSG_SIGNER_INFO pCounterSignerInfo = NULL;
    DWORD dwSignerInfo;
    CERT_INFO CertInfo;     
    SPROG_PUBLISHERINFO ProgPubInfo;
    //SYSTEMTIME st;

	bool result = false;

    ZeroMemory(&ProgPubInfo, sizeof(ProgPubInfo));
    __try
    {
        lstrcpynW(szFileName, fileName.c_str(), MAX_PATH);
		if(!VerifyEmbeddedSignature(szFileName)){
			//The signature is not valid
			result = false;
			__leave;
		}

        // Get message handle and store handle from the signed file.
        fResult = CryptQueryObject(CERT_QUERY_OBJECT_FILE,
                                   szFileName,
                                   CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
                                   CERT_QUERY_FORMAT_FLAG_BINARY,
                                   0,
                                   &dwEncoding,
                                   &dwContentType,
                                   &dwFormatType,
                                   &hStore,
                                   &hMsg,
                                   NULL);
        if (!fResult)
        {
            //_tprintf(_T("CryptQueryObject failed with %x\n"), GetLastError());
            result = false;
			__leave;
        }
		 // Get signer information size.
        fResult = CryptMsgGetParam(hMsg, 
                                   CMSG_SIGNER_INFO_PARAM, 
                                   0, 
                                   NULL, 
                                   &dwSignerInfo);
        if (!fResult)
        {
            //_tprintf(_T("CryptMsgGetParam failed with %x\n"), GetLastError());
			result = false;
            __leave;
        }

        // Allocate memory for signer information.
        pSignerInfo = (PCMSG_SIGNER_INFO)LocalAlloc(LPTR, dwSignerInfo);
        if (!pSignerInfo)
        {
            //_tprintf(_T("Unable to allocate memory for Signer Info.\n"));
			result = false;
            __leave;
        }

        // Get Signer Information.
        fResult = CryptMsgGetParam(hMsg, 
                                   CMSG_SIGNER_INFO_PARAM, 
                                   0, 
                                   (PVOID)pSignerInfo, 
                                   &dwSignerInfo);
        if (!fResult)
        {
            //_tprintf(_T("CryptMsgGetParam failed with %x\n"), GetLastError());
			result = false;
            __leave;
        }
        
        // Get program name and publisher information from 
        // signer info structure.
        if (GetProgAndPublisherInfo(pSignerInfo, &ProgPubInfo))
        {
			if (StrCmpW(ProgPubInfo.lpszProgramName, L"Roblox Application") != 0)
            {
				result = false;
				__leave;
                //wprintf(L"Program Name : %s\n", ProgPubInfo.lpszProgramName);
            }

            //if (ProgPubInfo.lpszPublisherLink != NULL)
            //{
            //    wprintf(L"Publisher Link : %s\n", ProgPubInfo.lpszPublisherLink);
            //}

			if (StrCmpW(ProgPubInfo.lpszMoreInfoLink, L"http://www.roblox.com ") != 0)
            {
				result = false;
				__leave;
                //wprintf(L"MoreInfo Link : %s\n", ProgPubInfo.lpszMoreInfoLink);
            }
        }

        // Search for the signer certificate in the temporary 
        // certificate store.
        CertInfo.Issuer = pSignerInfo->Issuer;
        CertInfo.SerialNumber = pSignerInfo->SerialNumber;

        pCertContext = CertFindCertificateInStore(hStore,
                                                  ENCODING,
                                                  0,
                                                  CERT_FIND_SUBJECT_CERT,
                                                  (PVOID)&CertInfo,
                                                  NULL);
        if (!pCertContext)
        {
            //_tprintf(_T("CertFindCertificateInStore failed with %x\n"), GetLastError());
			result = false;
            __leave;
        }

        // Print Signer certificate information.
        //_tprintf(_T("Signer Certificate:\n\n"));        
		if(!VerifyCertificateInfo(pCertContext)){
			result = false;
			__leave;
		}
        //_tprintf(_T("\n"));
        result = true;
        // Get the timestamp certificate signerinfo structure.
        /*if (GetTimeStampSignerInfo(pSignerInfo, &pCounterSignerInfo))
        {
            // Search for Timestamp certificate in the temporary
            // certificate store.
            CertInfo.Issuer = pCounterSignerInfo->Issuer;
            CertInfo.SerialNumber = pCounterSignerInfo->SerialNumber;

            pCertContext = CertFindCertificateInStore(hStore,
                                                ENCODING,
                                                0,
                                                CERT_FIND_SUBJECT_CERT,
                                                (PVOID)&CertInfo,
                                                NULL);
            if (!pCertContext)
            {
                _tprintf(_T("CertFindCertificateInStore failed with %x\n"),
                    GetLastError());
                __leave;
            }

            // Print timestamp certificate information.
            _tprintf(_T("TimeStamp Certificate:\n\n"));
            PrintCertificateInfo(pCertContext);
            _tprintf(_T("\n"));

            // Find Date of timestamp.
            if (GetDateOfTimeStamp(pCounterSignerInfo, &st))
            {
                _tprintf(_T("Date of TimeStamp : %02d/%02d/%04d %02d:%02d\n"),
                                            st.wMonth,
                                            st.wDay,
                                            st.wYear,
                                            st.wHour,
                                            st.wMinute);
            }
            _tprintf(_T("\n"));
        }*/
    }
    __finally
    {               
        // Clean up.
        if (ProgPubInfo.lpszProgramName != NULL)
            LocalFree(ProgPubInfo.lpszProgramName);
        if (ProgPubInfo.lpszPublisherLink != NULL)
            LocalFree(ProgPubInfo.lpszPublisherLink);
        if (ProgPubInfo.lpszMoreInfoLink != NULL)
            LocalFree(ProgPubInfo.lpszMoreInfoLink);

        if (pSignerInfo != NULL) LocalFree(pSignerInfo);
        if (pCounterSignerInfo != NULL) LocalFree(pCounterSignerInfo);
        if (pCertContext != NULL) CertFreeCertificateContext(pCertContext);
        if (hStore != NULL) CertCloseStore(hStore, 0);
        if (hMsg != NULL) CryptMsgClose(hMsg);
    }
	return result;
}

/*int _tmain(int argc, TCHAR *argv[])
{
    WCHAR szFileName[MAX_PATH]; 
    HCERTSTORE hStore = NULL;
    HCRYPTMSG hMsg = NULL; 
    PCCERT_CONTEXT pCertContext = NULL;
    BOOL fResult;   
    DWORD dwEncoding, dwContentType, dwFormatType;
    PCMSG_SIGNER_INFO pSignerInfo = NULL;
    PCMSG_SIGNER_INFO pCounterSignerInfo = NULL;
    DWORD dwSignerInfo;
    CERT_INFO CertInfo;     
    SPROG_PUBLISHERINFO ProgPubInfo;
    SYSTEMTIME st;

    ZeroMemory(&ProgPubInfo, sizeof(ProgPubInfo));
    __try
    {
        if (argc != 2)
        {
            _tprintf(_T("Usage: SignedFileInfo <filename>\n"));
            return 0;
        }

#ifdef UNICODE
        lstrcpynW(szFileName, argv[1], MAX_PATH);
#else
        if (mbstowcs(szFileName, argv[1], MAX_PATH) == -1)
        {
            printf("Unable to convert to unicode.\n");
            __leave;
        }
#endif

        // Get message handle and store handle from the signed file.
        fResult = CryptQueryObject(CERT_QUERY_OBJECT_FILE,
                                   szFileName,
                                   CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
                                   CERT_QUERY_FORMAT_FLAG_BINARY,
                                   0,
                                   &dwEncoding,
                                   &dwContentType,
                                   &dwFormatType,
                                   &hStore,
                                   &hMsg,
                                   NULL);
        if (!fResult)
        {
            _tprintf(_T("CryptQueryObject failed with %x\n"), GetLastError());
            __leave;
        }

        // Get signer information size.
        fResult = CryptMsgGetParam(hMsg, 
                                   CMSG_SIGNER_INFO_PARAM, 
                                   0, 
                                   NULL, 
                                   &dwSignerInfo);
        if (!fResult)
        {
            _tprintf(_T("CryptMsgGetParam failed with %x\n"), GetLastError());
            __leave;
        }

        // Allocate memory for signer information.
        pSignerInfo = (PCMSG_SIGNER_INFO)LocalAlloc(LPTR, dwSignerInfo);
        if (!pSignerInfo)
        {
            _tprintf(_T("Unable to allocate memory for Signer Info.\n"));
            __leave;
        }

        // Get Signer Information.
        fResult = CryptMsgGetParam(hMsg, 
                                   CMSG_SIGNER_INFO_PARAM, 
                                   0, 
                                   (PVOID)pSignerInfo, 
                                   &dwSignerInfo);
        if (!fResult)
        {
            _tprintf(_T("CryptMsgGetParam failed with %x\n"), GetLastError());
            __leave;
        }
        
        // Get program name and publisher information from 
        // signer info structure.
        if (GetProgAndPublisherInfo(pSignerInfo, &ProgPubInfo))
        {
            if (ProgPubInfo.lpszProgramName != NULL)
            {
                wprintf(L"Program Name : %s\n",
                    ProgPubInfo.lpszProgramName);
            }

            if (ProgPubInfo.lpszPublisherLink != NULL)
            {
                wprintf(L"Publisher Link : %s\n",
                    ProgPubInfo.lpszPublisherLink);
            }

            if (ProgPubInfo.lpszMoreInfoLink != NULL)
            {
                wprintf(L"MoreInfo Link : %s\n",
                    ProgPubInfo.lpszMoreInfoLink);
            }
        }

        _tprintf(_T("\n"));

        // Search for the signer certificate in the temporary 
        // certificate store.
        CertInfo.Issuer = pSignerInfo->Issuer;
        CertInfo.SerialNumber = pSignerInfo->SerialNumber;

        pCertContext = CertFindCertificateInStore(hStore,
                                                  ENCODING,
                                                  0,
                                                  CERT_FIND_SUBJECT_CERT,
                                                  (PVOID)&CertInfo,
                                                  NULL);
        if (!pCertContext)
        {
            _tprintf(_T("CertFindCertificateInStore failed with %x\n"),
                GetLastError());
            __leave;
        }

        // Print Signer certificate information.
        _tprintf(_T("Signer Certificate:\n\n"));        
        PrintCertificateInfo(pCertContext);
        _tprintf(_T("\n"));
        
        // Get the timestamp certificate signerinfo structure.
        if (GetTimeStampSignerInfo(pSignerInfo, &pCounterSignerInfo))
        {
            // Search for Timestamp certificate in the temporary
            // certificate store.
            CertInfo.Issuer = pCounterSignerInfo->Issuer;
            CertInfo.SerialNumber = pCounterSignerInfo->SerialNumber;

            pCertContext = CertFindCertificateInStore(hStore,
                                                ENCODING,
                                                0,
                                                CERT_FIND_SUBJECT_CERT,
                                                (PVOID)&CertInfo,
                                                NULL);
            if (!pCertContext)
            {
                _tprintf(_T("CertFindCertificateInStore failed with %x\n"),
                    GetLastError());
                __leave;
            }

            // Print timestamp certificate information.
            _tprintf(_T("TimeStamp Certificate:\n\n"));
            PrintCertificateInfo(pCertContext);
            _tprintf(_T("\n"));

            // Find Date of timestamp.
            if (GetDateOfTimeStamp(pCounterSignerInfo, &st))
            {
                _tprintf(_T("Date of TimeStamp : %02d/%02d/%04d %02d:%02d\n"),
                                            st.wMonth,
                                            st.wDay,
                                            st.wYear,
                                            st.wHour,
                                            st.wMinute);
            }
            _tprintf(_T("\n"));
        }
    }
    __finally
    {               
        // Clean up.
        if (ProgPubInfo.lpszProgramName != NULL)
            LocalFree(ProgPubInfo.lpszProgramName);
        if (ProgPubInfo.lpszPublisherLink != NULL)
            LocalFree(ProgPubInfo.lpszPublisherLink);
        if (ProgPubInfo.lpszMoreInfoLink != NULL)
            LocalFree(ProgPubInfo.lpszMoreInfoLink);

        if (pSignerInfo != NULL) LocalFree(pSignerInfo);
        if (pCounterSignerInfo != NULL) LocalFree(pCounterSignerInfo);
        if (pCertContext != NULL) CertFreeCertificateContext(pCertContext);
        if (hStore != NULL) CertCloseStore(hStore, 0);
        if (hMsg != NULL) CryptMsgClose(hMsg);
    }
    return 0;
}*/

bool VerifyCertificateInfo(PCCERT_CONTEXT pCertContext)
{
    bool result = true;
    LPTSTR szName = NULL;
    DWORD dwData;

	static std::string issuerNameToCheck; // Symantec Class 3 SHA256 Code Signing CA
	issuerNameToCheck = "";
	issuerNameToCheck = "Sym";
	issuerNameToCheck += "ant";
	issuerNameToCheck += "ec ";
	issuerNameToCheck += "Cla";
	issuerNameToCheck += "ss ";
	issuerNameToCheck += "3 S";
	issuerNameToCheck += "HA2";
	issuerNameToCheck += "56 ";
	issuerNameToCheck += "Cod";
	issuerNameToCheck += "e S";
	issuerNameToCheck += "ign";
	issuerNameToCheck += "ing";
	issuerNameToCheck += " CA";

	static std::string nameToCheck;
	nameToCheck += "R";
	nameToCheck += "O";
	nameToCheck += "B";
	nameToCheck += "L";
	nameToCheck += "O";
	nameToCheck += "X";
	nameToCheck += " C";
	nameToCheck += "or";
	nameToCheck += "por";
	nameToCheck += "at";
	nameToCheck += "io";
	nameToCheck += "n";

    // Print Serial Number.
    //_tprintf(_T("Serial Number: "));
    dwData = pCertContext->pCertInfo->SerialNumber.cbData;
	if(dwData != 16){
		return false;
	}

	// signature is taken from current version of RobloxApp file
    // (the value displayed in windows is shown here.  In memory it is arranged like this)
	if(	pCertContext->pCertInfo->SerialNumber.pbData[15] != 0x1B || 
		pCertContext->pCertInfo->SerialNumber.pbData[14] != 0x81 ||
		pCertContext->pCertInfo->SerialNumber.pbData[13] != 0x59 ||
		pCertContext->pCertInfo->SerialNumber.pbData[12] != 0xFA ||
		pCertContext->pCertInfo->SerialNumber.pbData[11] != 0xF8 ||
		pCertContext->pCertInfo->SerialNumber.pbData[10] != 0x22 ||
		pCertContext->pCertInfo->SerialNumber.pbData[ 9] != 0x8B ||
		pCertContext->pCertInfo->SerialNumber.pbData[ 8] != 0x39 ||
		pCertContext->pCertInfo->SerialNumber.pbData[ 7] != 0xAB ||
		pCertContext->pCertInfo->SerialNumber.pbData[ 6] != 0xC0 ||
		pCertContext->pCertInfo->SerialNumber.pbData[ 5] != 0x0E ||
		pCertContext->pCertInfo->SerialNumber.pbData[ 4] != 0x31 ||
		pCertContext->pCertInfo->SerialNumber.pbData[ 3] != 0xBB ||
		pCertContext->pCertInfo->SerialNumber.pbData[ 2] != 0xAD ||
		pCertContext->pCertInfo->SerialNumber.pbData[ 1] != 0x43 ||
		pCertContext->pCertInfo->SerialNumber.pbData[ 0] != 0x09){
		return false;
	}

    //for (DWORD n = 0; n < dwData; n++)
    //{
    //    _tprintf(_T("%02x "), pCertContext->pCertInfo->SerialNumber.pbData[dwData - (n + 1)]);
    //}
    //_tprintf(_T("\n"));

    // Get Issuer name size.
    if (!(dwData = CertGetNameString(pCertContext, 
                                     CERT_NAME_SIMPLE_DISPLAY_TYPE,
                                     CERT_NAME_ISSUER_FLAG,
                                     NULL,
                                     NULL,
                                     0)))
    {
        //_tprintf(_T("CertGetNameString failed.\n"));
		return false;
	}

    // Allocate memory for Issuer name.
    szName = (LPTSTR)LocalAlloc(LPTR, dwData * sizeof(TCHAR));
    if (!szName)
    {
        //_tprintf(_T("Unable to allocate memory for issuer name.\n"));
	    return false;
    }

    // Get Issuer name.
    if (!(CertGetNameString(pCertContext, 
                            CERT_NAME_SIMPLE_DISPLAY_TYPE,
                            CERT_NAME_ISSUER_FLAG,
                            NULL,
                            szName,
                            dwData)))
    {
        //_tprintf(_T("CertGetNameString failed.\n"));
		if (szName != NULL) LocalFree(szName);
	    return false;
    }

    // print Issuer name.


	if(szName != issuerNameToCheck){
		if (szName != NULL) LocalFree(szName);
	    return false;
	}
    //_tprintf(_T("Issuer Name: %s\n"), szName);
    LocalFree(szName);
    szName = NULL;

    // Get Subject name size.
    if (!(dwData = CertGetNameString(pCertContext, 
                                     CERT_NAME_SIMPLE_DISPLAY_TYPE,
                                     0,
                                     NULL,
                                     NULL,
                                     0)))
    {
        //_tprintf(_T("CertGetNameString failed.\n"));
	    return false;
    }

    // Allocate memory for subject name.
    szName = (LPTSTR)LocalAlloc(LPTR, dwData * sizeof(TCHAR));
    if (!szName)
    {
        //_tprintf(_T("Unable to allocate memory for subject name.\n"));
	    return false;
    }

    // Get subject name.
    if (!(CertGetNameString(pCertContext, 
                            CERT_NAME_SIMPLE_DISPLAY_TYPE,
                            0,
                            NULL,
                            szName,
                            dwData)))
    {
        //_tprintf(_T("CertGetNameString failed.\n"));
		if (szName != NULL) LocalFree(szName);
	    return false;
    }
	

    // Print Subject Name.
	if(szName != nameToCheck){
		if (szName != NULL) LocalFree(szName);
	    return false;
	}
    //_tprintf(_T("Subject Name: %s\n"), szName); //"ROBLOX Corporation"

    return true;
}

LPWSTR AllocateAndCopyWideString(LPCWSTR inputString)
{
    LPWSTR outputString = NULL;

    outputString = (LPWSTR)LocalAlloc(LPTR,
        (wcslen(inputString) + 1) * sizeof(WCHAR));
    if (outputString != NULL)
    {
        lstrcpyW(outputString, inputString);
    }
    return outputString;
}

BOOL GetProgAndPublisherInfo(PCMSG_SIGNER_INFO pSignerInfo,
                             PSPROG_PUBLISHERINFO Info)
{
    BOOL fReturn = FALSE;
    PSPC_SP_OPUS_INFO OpusInfo = NULL;  
    DWORD dwData;
    BOOL fResult;
    
    __try
    {
        // Loop through authenticated attributes and find
        // SPC_SP_OPUS_INFO_OBJID OID.
        for (DWORD n = 0; n < pSignerInfo->AuthAttrs.cAttr; n++)
        {           
            if (lstrcmpA(SPC_SP_OPUS_INFO_OBJID, 
                        pSignerInfo->AuthAttrs.rgAttr[n].pszObjId) == 0)
            {
                // Get Size of SPC_SP_OPUS_INFO structure.
                fResult = CryptDecodeObject(ENCODING,
                            SPC_SP_OPUS_INFO_OBJID,
                            pSignerInfo->AuthAttrs.rgAttr[n].rgValue[0].pbData,
                            pSignerInfo->AuthAttrs.rgAttr[n].rgValue[0].cbData,
                            0,
                            NULL,
                            &dwData);
                if (!fResult)
                {
                    _tprintf(_T("CryptDecodeObject failed with %x\n"),
                        GetLastError());
                    __leave;
                }

                // Allocate memory for SPC_SP_OPUS_INFO structure.
                OpusInfo = (PSPC_SP_OPUS_INFO)LocalAlloc(LPTR, dwData);
                if (!OpusInfo)
                {
                    _tprintf(_T("Unable to allocate memory for Publisher Info.\n"));
                    __leave;
                }

                // Decode and get SPC_SP_OPUS_INFO structure.
                fResult = CryptDecodeObject(ENCODING,
                            SPC_SP_OPUS_INFO_OBJID,
                            pSignerInfo->AuthAttrs.rgAttr[n].rgValue[0].pbData,
                            pSignerInfo->AuthAttrs.rgAttr[n].rgValue[0].cbData,
                            0,
                            OpusInfo,
                            &dwData);
                if (!fResult)
                {
                    _tprintf(_T("CryptDecodeObject failed with %x\n"),
                        GetLastError());
                    __leave;
                }

                // Fill in Program Name if present.
                if (OpusInfo->pwszProgramName)
                {
                    Info->lpszProgramName =
                        AllocateAndCopyWideString(OpusInfo->pwszProgramName);
                }
                else
                    Info->lpszProgramName = NULL;

                // Fill in Publisher Information if present.
                if (OpusInfo->pPublisherInfo)
                {

                    switch (OpusInfo->pPublisherInfo->dwLinkChoice)
                    {
                        case SPC_URL_LINK_CHOICE:
                            Info->lpszPublisherLink =
                                AllocateAndCopyWideString(OpusInfo->pPublisherInfo->pwszUrl);
                            break;

                        case SPC_FILE_LINK_CHOICE:
                            Info->lpszPublisherLink =
                                AllocateAndCopyWideString(OpusInfo->pPublisherInfo->pwszFile);
                            break;

                        default:
                            Info->lpszPublisherLink = NULL;
                            break;
                    }
                }
                else
                {
                    Info->lpszPublisherLink = NULL;
                }

                // Fill in More Info if present.
                if (OpusInfo->pMoreInfo)
                {
                    switch (OpusInfo->pMoreInfo->dwLinkChoice)
                    {
                        case SPC_URL_LINK_CHOICE:
                            Info->lpszMoreInfoLink =
                                AllocateAndCopyWideString(OpusInfo->pMoreInfo->pwszUrl);
                            break;

                        case SPC_FILE_LINK_CHOICE:
                            Info->lpszMoreInfoLink =
                                AllocateAndCopyWideString(OpusInfo->pMoreInfo->pwszFile);
                            break;

                        default:
                            Info->lpszMoreInfoLink = NULL;
                            break;
                    }
                }               
                else
                {
                    Info->lpszMoreInfoLink = NULL;
                }

                fReturn = TRUE;

                break; // Break from for loop.
            } // lstrcmp SPC_SP_OPUS_INFO_OBJID                 
        } // for 
    }
    __finally
    {
        if (OpusInfo != NULL) LocalFree(OpusInfo);      
    }

    return fReturn;
}

BOOL GetDateOfTimeStamp(PCMSG_SIGNER_INFO pSignerInfo, SYSTEMTIME *st)
{   
    BOOL fResult;
    FILETIME lft, ft;   
    DWORD dwData;
    BOOL fReturn = FALSE;
    
    // Loop through authenticated attributes and find
    // szOID_RSA_signingTime OID.
    for (DWORD n = 0; n < pSignerInfo->AuthAttrs.cAttr; n++)
    {           
        if (lstrcmpA(szOID_RSA_signingTime, 
                    pSignerInfo->AuthAttrs.rgAttr[n].pszObjId) == 0)
        {               
            // Decode and get FILETIME structure.
            dwData = sizeof(ft);
            fResult = CryptDecodeObject(ENCODING,
                        szOID_RSA_signingTime,
                        pSignerInfo->AuthAttrs.rgAttr[n].rgValue[0].pbData,
                        pSignerInfo->AuthAttrs.rgAttr[n].rgValue[0].cbData,
                        0,
                        (PVOID)&ft,
                        &dwData);
            if (!fResult)
            {
                _tprintf(_T("CryptDecodeObject failed with %x\n"),
                    GetLastError());
                break;
            }

            // Convert to local time.
            FileTimeToLocalFileTime(&ft, &lft);
            FileTimeToSystemTime(&lft, st);

            fReturn = TRUE;

            break; // Break from for loop.
                        
        } //lstrcmp szOID_RSA_signingTime
    } // for 

    return fReturn;
}

BOOL GetTimeStampSignerInfo(PCMSG_SIGNER_INFO pSignerInfo, PCMSG_SIGNER_INFO *pCounterSignerInfo)
{   
    PCCERT_CONTEXT pCertContext = NULL;
    BOOL fReturn = FALSE;
    BOOL fResult;       
    DWORD dwSize;   

    __try
    {
        *pCounterSignerInfo = NULL;

        // Loop through unathenticated attributes for
        // szOID_RSA_counterSign OID.
        for (DWORD n = 0; n < pSignerInfo->UnauthAttrs.cAttr; n++)
        {
            if (lstrcmpA(pSignerInfo->UnauthAttrs.rgAttr[n].pszObjId, 
                         szOID_RSA_counterSign) == 0)
            {
                // Get size of CMSG_SIGNER_INFO structure.
                fResult = CryptDecodeObject(ENCODING,
                           PKCS7_SIGNER_INFO,
                           pSignerInfo->UnauthAttrs.rgAttr[n].rgValue[0].pbData,
                           pSignerInfo->UnauthAttrs.rgAttr[n].rgValue[0].cbData,
                           0,
                           NULL,
                           &dwSize);
                if (!fResult)
                {
                    _tprintf(_T("CryptDecodeObject failed with %x\n"),
                        GetLastError());
                    __leave;
                }

                // Allocate memory for CMSG_SIGNER_INFO.
                *pCounterSignerInfo = (PCMSG_SIGNER_INFO)LocalAlloc(LPTR, dwSize);
                if (!*pCounterSignerInfo)
                {
                    _tprintf(_T("Unable to allocate memory for timestamp info.\n"));
                    __leave;
                }

                // Decode and get CMSG_SIGNER_INFO structure
                // for timestamp certificate.
                fResult = CryptDecodeObject(ENCODING,
                           PKCS7_SIGNER_INFO,
                           pSignerInfo->UnauthAttrs.rgAttr[n].rgValue[0].pbData,
                           pSignerInfo->UnauthAttrs.rgAttr[n].rgValue[0].cbData,
                           0,
                           (PVOID)*pCounterSignerInfo,
                           &dwSize);
                if (!fResult)
                {
                    _tprintf(_T("CryptDecodeObject failed with %x\n"),
                        GetLastError());
                    __leave;
                }

                fReturn = TRUE;
                
                break; // Break from for loop.
            }           
        }
    }
    __finally
    {
        // Clean up.
        if (pCertContext != NULL) CertFreeCertificateContext(pCertContext);
    }

    return fReturn;
}
