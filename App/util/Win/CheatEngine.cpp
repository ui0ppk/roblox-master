/*
This is John's counter-insurgency code, upgraded by Will.

When modifying this file, please be mindful of what suspicious
string literals and API calls will look like in a disassembler.
*/
#include "stdafx.h"

#include "boost/filesystem/operations.hpp"

#include <vector>
#include <list>

#include "util/CheatEngine.h"
#include "V8DataModel/HackDefines.h"
#include "util/RobloxGoogleAnalytics.h"
#include <psapi.h>
#include <TlHelp32.h>

#include "VMProtect/VMProtectSDK.h"
#include "FastLog.h"

#include <ctime>


FASTFLAGVARIABLE(US21969, false);
FASTINTVARIABLE(NumSmoothingPasses, 0)
FASTINTVARIABLE(RegLambda, -1000000000)

namespace RBX
{
    bool ceDetected = false;
    bool ceHwndChecks = false;
    HeapValue<uintptr_t> vehHookLocationHv(0);
    HeapValue<uintptr_t> vehStubLocationHv(0);
    void* vehHookContinue;
    __declspec(align(4096)) int writecopyTrap[4096];
}

namespace
{
	using boost::shared_ptr;

	typedef std::list<RGBQUAD> RGBImage;
	typedef std::vector<shared_ptr<RGBImage> > RGBImages;

	static void addImageFromBytes(RGBImages &images, const BYTE kRGBs[], const size_t kRGBLen)
	{
		RGBImages::value_type image(new RGBImage);
		images.push_back(image);

		RGBQUAD p;
		p.rgbReserved = 0;

		for (size_t i = 0; i <= kRGBLen - 3; i += 3)
		{
			p.rgbRed = kRGBs[i];
			p.rgbGreen = kRGBs[i+1];
			p.rgbBlue = kRGBs[i+2];
			image->push_back(p);
		}
	}

	// Add the large icon in the CE app itself.
	static void addImage1(RGBImages &images)
	{
		const BYTE kRGBs[] = {
			0,29,38,
			0,30,39,
			48,130,141,
			0,56,67,
			41,135,147,
			0,78,92,
			33,133,148,
			0,92,106
		};

		addImageFromBytes(images, kRGBs, sizeof(kRGBs) / sizeof(BYTE));
	}

	// Add the small icon in the upper-left of the CE window.
	static void addImage2(RGBImages &images)
	{
		const BYTE kRGBs[] = {
			1,86,130,
			1,56,86,
			3,29,43,
			3,19,28,
			0,13,21,
			0,23,36,
			0,43,67,
			1,76,115
		};

		addImageFromBytes(images, kRGBs, sizeof(kRGBs) / sizeof(BYTE));
	}

	// Add the icon that will appear in the task bar.
	static void addImage3(RGBImages &images)
	{
		const BYTE kRGBs[] = {
			1,40,61,
			1,45,69,
			1,66,101,
			1,91,137,
			1,100,149,
			1,89,134,
			1,63,96,
			1,44,68
		};

		addImageFromBytes(images, kRGBs, sizeof(kRGBs) / sizeof(BYTE));
	}

	// Add the small icon that will appear in the task bar (using small icons)
	static void addImage4(RGBImages &images)
	{
		const BYTE kRGBs[] = {
			3,30,46,
			6,33,49,
			1,72,108,
			1,96,144,
			2,49,75,
			8,35,51,
			8,35,51,
			7,40,58
		};

		addImageFromBytes(images, kRGBs, sizeof(kRGBs) / sizeof(BYTE));
	}

	static void setupImageArray(RGBImages &images)
	{
		addImage1(images);
		addImage2(images);

		if (FFlag::US21969)
		{
			addImage3(images);
			addImage4(images);
		}
	}
} // namespace

// ScanForCheatEngine. Returns true if we find it.
bool RBX::vmProtectedDetectCheatEngineIcon()
{
	VMProtectBeginMutation("6");

	RGBImages cheatImageTemplates;
	setupImageArray(cheatImageTemplates);

	HWND hDesktopWnd=GetDesktopWindow();
	HDC hDesktopDC=GetDC(hDesktopWnd);
	int		nWidth=GetSystemMetrics(SM_CXSCREEN);
	int		nHeight=GetSystemMetrics(SM_CYSCREEN);
	HDC		hBmpFileDC=CreateCompatibleDC(hDesktopDC);
	HBITMAP	hBmpFileBitmap=CreateCompatibleBitmap(hDesktopDC,nWidth,nHeight);
	HBITMAP hOldBitmap = (HBITMAP) SelectObject(hBmpFileDC,hBmpFileBitmap);
	BitBlt(hBmpFileDC,0,0,nWidth,nHeight,hDesktopDC,0,0,SRCCOPY|CAPTUREBLT);
	SelectObject(hBmpFileDC,hOldBitmap);

	bool result = false;
	LPVOID				pBuf=NULL;
	BITMAPINFO			bmpInfo;
	//BITMAPFILEHEADER	bmpFileHeader;
	HDC hdc=GetDC(NULL);

	do
	{
		ZeroMemory(&bmpInfo,sizeof(BITMAPINFO));
		bmpInfo.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
		GetDIBits(hdc,hBmpFileBitmap,0,0,NULL,&bmpInfo,DIB_RGB_COLORS);

		if(bmpInfo.bmiHeader.biSizeImage<=0)
			bmpInfo.bmiHeader.biSizeImage=bmpInfo.bmiHeader.biWidth*abs(bmpInfo.bmiHeader.biHeight)*(bmpInfo.bmiHeader.biBitCount+7)/8;

		if((pBuf=malloc(bmpInfo.bmiHeader.biSizeImage))==NULL)
		{
			//MessageBox(NULL,_T("Unable to Allocate Bitmap Memory"),_T("Error"),MB_OK|MB_ICONERROR);
			break;
		}

		bmpInfo.bmiHeader.biCompression=BI_RGB;
		int scanlines = GetDIBits(hdc,hBmpFileBitmap,0,bmpInfo.bmiHeader.biHeight,pBuf,&bmpInfo,DIB_RGB_COLORS);	

		if (bmpInfo.bmiHeader.biBitCount != 32) break;

		// assume 32 bit color depth
		const int *p = (const int *)pBuf;
		for (RGBImages::const_iterator imagesIt = cheatImageTemplates.begin(); cheatImageTemplates.end() != imagesIt; ++imagesIt)
		{
			if (result)
			{
				break;
			}

			shared_ptr<RGBImage> image = *imagesIt;
			for (unsigned int i = 0 ; i < (bmpInfo.bmiHeader.biSizeImage / 4) - (image->size() * 4); i++) // step - number of pixels (in template) * 4 bytes per pixel
			{
				if (result)
				{
					break;
				}

				result = true;
				size_t offs = 0;
				for (RGBImage::const_iterator it = image->begin(); image->end() != it; ++it, ++offs)
				{
					const RGBQUAD &quad = *it;
					const int px = *reinterpret_cast<const int *>(&quad);
					if ((p[i + offs] & 0xFFFFFF /*ignore reserved bits, assume little-endian*/) != px)
					{
						result = false;
						break;
					}
				}
			}
		}
	} while (false);

	if(hdc)
		ReleaseDC(NULL,hdc);

	if(pBuf)
		free(pBuf);

	if (hBmpFileDC)
		DeleteDC(hBmpFileDC);

	if (hBmpFileBitmap)
		DeleteObject(hBmpFileBitmap);

    VMProtectEnd();
	return result;
	
}

namespace RBX {

    namespace CryptStrings{
        // Code generation.  This is equivalent to strCmp(x, "Window") == 0
        bool cmpWindow(const char* inString)
        {
            const unsigned char cmpString[7] = {9, 214, 144, 249, 109, 4, 186};
            if (!inString) return false;
            for (int i = 0; i < 7; ++i)
            {
                if ((unsigned char)((inString[i]+i)*159) != cmpString[i]) return false;
                if (!inString[i]) return i == 6;
            };
            return false;
        }

        // Code generation.  This is equivalent to strCmp(x, "ComboLBox") == 0
        bool cmpComboLBox(const char* inString)
        {
            const unsigned char cmpString[10] = {229, 16, 89, 51, 53, 231, 120, 90, 128, 111};
            if (!inString) return false;
            for (int i = 0; i < 10; ++i)
            {
                if ((unsigned char)((inString[i]+i)*183) != cmpString[i]) return false;
                if (!inString[i]) return i == 9;
            };
            return false;
        }

        // Compares input to "Cheat Engine" but is not null terminated.
        // This has been modified to work with the obfuscated strings.
        bool cmpPrefixCheatEngine(const char* inString)
        {
            const unsigned char cmpString[12] = {71, 245, 155, 148, 24, 1, 175, 17, 3, 10, 24, 176};
            if (!inString) return false;
            for (int i = 0; i < 12; ++i)
            {
                if ((unsigned char)(((inString[i]^kCeCharKey)+i)*173) != cmpString[i]) return false;
                if (!(inString[i]^kCeCharKey)) return false;
            };
            return true;
        }

        // CE 5.6.1 loads a LOT of windows at startup.  The class names are revealing.
        bool cmpTfrmdissectWindow(const char* inString)
        {
            const unsigned char cmpString[18] = {244, 95, 20, 240, 168, 94, 129, 202, 21, 204, 238, 242, 93, 19, 130, 238, 127, 217};
            if (!inString) return false;
            for (int i = 0; i < 18; ++i)
            {
                if ((unsigned char)((inString[i]+i)*73) != cmpString[i]) return false;
                if (!inString[i]) return (i == 17);
            };
            return false;
        }

    };

 size_t HwndScanner::fullWindowInfo::compareByPid ( fullWindowInfo lhs, fullWindowInfo rhs)
{
    return (lhs.pid < rhs.pid);
}

BOOL CALLBACK HwndScanner::makeHwndVector(HWND hwnd, LPARAM lParam)
{
    VMProtectBeginMutation("7");
    std::vector<fullWindowInfo>* listing = reinterpret_cast<std::vector<fullWindowInfo>* >(lParam);

    char className[32];
    GetClassName(hwnd,className, sizeof(className));

    fullWindowInfo thisWindow;
    if ( CryptStrings::cmpWindow(className) || CryptStrings::cmpComboLBox(className))
    {   
        // Some people get very reactive when a game scans for "personal data" in other
        // processes.  This attempts to limit the search to a handful of apps that use
        // the same names for UI elements.
        char title[32] = "NA";
        bool* returnValue = reinterpret_cast<bool*>(lParam);
        GetWindowText(hwnd,title,sizeof(title));
        thisWindow.title = title;

        // These will linger in RAM, so they should be obfuscated.
        for (int i = 0; i < sizeof(title); ++i)
        {
            if (!thisWindow.title[i])
            {
                break;
            }
            else
            {
                thisWindow.title[i] ^= kCeCharKey;
            }
        }

        // Window position and size doesn't seem like personal information.
        // and again, I don't want to make it easy to determine what these are.
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        thisWindow.pid = pid;

        WINDOWINFO windowInfo;
        windowInfo.cbSize = sizeof(windowInfo);
        GetWindowInfo(hwnd, &windowInfo);
        thisWindow.winWidth  = kCeStructKey * (windowInfo.rcClient.right  - windowInfo.rcClient.left);
        thisWindow.winHeight = kCeStructKey * (windowInfo.rcClient.bottom - windowInfo.rcClient.top);
        thisWindow.active = windowInfo.dwWindowStatus;
        thisWindow.kickEarly = false;
        listing->push_back(thisWindow);
    }
    else if (CryptStrings::cmpTfrmdissectWindow(className))
    {
        thisWindow.active = 0;
        thisWindow.winHeight = 0;
        thisWindow.winWidth = 0;
        thisWindow.title = "";
        thisWindow.pid = 0;
        thisWindow.kickEarly = true;
        listing->push_back(thisWindow);
    }

    VMProtectEnd();
    return TRUE;
}

HwndScanner::HwndScanner()
{
}

int HwndScanner::scan()
{
    hwndScanResults.clear();
    EnumWindows(makeHwndVector, (LPARAM) &hwndScanResults);
    std::sort (hwndScanResults.begin(), hwndScanResults.end(), &fullWindowInfo::compareByPid);
    return hwndScanResults.size();
}

bool HwndScanner::detectTitle() const
{
    for(std::vector<fullWindowInfo>::const_iterator it = hwndScanResults.begin(); it != hwndScanResults.end(); ++it )
    {
        if (CryptStrings::cmpPrefixCheatEngine(it->title.c_str()))
        {   
            return true;
        }
    }
    return false;
}

bool HwndScanner::detectEarlyKick() const
{
    for(std::vector<fullWindowInfo>::const_iterator it = hwndScanResults.begin(); it != hwndScanResults.end(); ++it )
    {
        if (it->kickEarly)
        {   
            return true;
        }
    }
    return false;
}

bool HwndScanner::detectFakeAttach() const
{
    if (!hwndScanResults.size())
    {
        return false;
    }
    DWORD currentPid = hwndScanResults[0].pid;
    DWORD hasParams = 0;
    for(std::vector<fullWindowInfo>::const_iterator it = hwndScanResults.begin(); it != hwndScanResults.end(); ++it )
    {
        if (it->pid != currentPid)
        {
            hasParams = 0;
            currentPid = it->pid;
        }
        // Drop down "compare type"
        if ((it->winWidth == kCeStructKey*168) && (it->winHeight == kCeStructKey*143))
        {
            hasParams |= 1;
        }
        // Drop down "data type"
        if ((it->winWidth == kCeStructKey*168) && (it->winHeight == kCeStructKey*65))
        {
            hasParams |= 2;
        }
        // This just exists.
        if ((it->winWidth == 0) && (it->winHeight == 0))
        {
            hasParams |= 4;
        }
        // This is the process list.
        if ((it->winWidth == kCeStructKey*245) && (it->winHeight == kCeStructKey*353) && it->active == 0)
        {
            hasParams |= 8;
        }
        // This is "Modify Register at" (destroyed when closed) (for win32)
        else if ((it->winWidth == kCeStructKey*274) && (it->winHeight == kCeStructKey*265))
        {
            hasParams |= 0x8;
        }
        // This is DBVM's "DBK32 Loaded" flashy graphic.  AWESOME!
        else if ((it->winWidth == kCeStructKey*420) && (it->winHeight == kCeStructKey*60))
        {
            hasParams |= 0x8;
        }
        if (hasParams == 0xF)
        {
            return true;
        }
    }
    return false;
}

FileScanner::FileScanner() : baseTime(std::time(NULL))
{
    char folderName[MAX_PATH+1];
    DWORD errCode = GetTempPath(sizeof(folderName), folderName);

    // Windows API returns 0 for error
    if (errCode)
    {
        tempFolder = folderName;
    }
    
}

bool FileScanner::detectLogUpdate() const
{
    // This here assumes the user has blindly changed "Cheat Engine" in the string table for CE.
    // It is also assumed the user hasn't changed the defaults.  For speed, this will attempt to
    // find {CE}/{GUID}/MEMORY.FIRST
    // This will only get CE users who run scans.
    boost::system::error_code fsErrorCode;
    boost::filesystem::exists(tempFolder, fsErrorCode);
    if (fsErrorCode)
    {
        return false;
    }
    
    boost::filesystem::directory_iterator ceIter(tempFolder, fsErrorCode);
    boost::filesystem::directory_iterator endIter;
    if (fsErrorCode)
    {
        return false;
    }

    // CE modifies the %TMP%/{CE} directory when run, but never after that.
    // A scan will update the mtime of  %TMP%/{CE}/{GUID}
    // Thus, scan for a recently modified subdirectory with MEMORY.FIRST, in any directory.
    for (; ceIter != endIter; ++ceIter)
    {
        if (boost::filesystem::is_directory(ceIter->status())) 
        {
            boost::filesystem::directory_iterator guidIter(*ceIter, fsErrorCode);
            for (; guidIter != endIter; ++guidIter)
            {
                if((boost::filesystem::is_directory(guidIter->status())) 
                    && (boost::filesystem::last_write_time(*guidIter) >= baseTime ))
                {
                    boost::filesystem::path finalPath = guidIter->path();
                    finalPath /= "MEMORY.FIRST";
                    bool doesExist = boost::filesystem::exists(finalPath, fsErrorCode);
                    if (doesExist && !fsErrorCode)
                    {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

static DWORD WINAPI watchCeLogDir(void* ceWatchHandle)
{
    VMProtectBeginMutation("8");
    DWORD waitStatus = WAIT_ABANDONED;
    waitStatus = WaitForSingleObject(ceWatchHandle, INFINITE);

    if (waitStatus == WAIT_OBJECT_0)
    {
        ceDetected = true;
    }
    VMProtectEnd();
    return 0;
}

namespace CryptStrings
{

    void decodeInto(unsigned char* to, const unsigned char* from, unsigned char key, int copySize)
    {
        for (int i = 0; i < copySize; ++i)
        {
            to[i] = (from[i]*key) - i;
        }
    }
}

// Returns a handle to a thread that waits for CE to change log files.
// Apparently this won't work because you can't access the registry becuase invalid path...
// I'm guessing it returns invalid path incorrectly because the path is valid.
HANDLE setupCeLogWatcher()
{
    VMProtectBeginMutation("9");
    // Cheat Engine
    static const unsigned char kCeName[13] = {221, 247, 57, 28, 136, 187, 213, 107, 49, 78, 136, 144, 116}; // 95 : 
    static const unsigned char kCeNameKey = 159;
    // Software\\Cheat Engine
    static const unsigned char kCeReg[22] = {5, 16, 216, 1, 29, 74, 72, 244, 188, 20, 158, 16, 59, 199, 194, 76, 242, 72, 29, 199, 143, 211}; // 71 : 
    static const unsigned char kCeRegKey = 119;
    // Scanfolder
    static const unsigned char kCeScanfolder[11] = {167, 84, 119, 141, 130, 36, 106, 95, 25, 47, 162}; // 221 : 
    static const unsigned char kCeScanfolderKey = 117;

    char folderName[MAX_PATH+2];
    DWORD passCode;
    HANDLE ceLogWatcher = INVALID_HANDLE_VALUE;
    boost::filesystem::path ceLogPath("");
    HKEY hKey;
    unsigned char tmpString[32];
    bool tryDefault = true;
    bool spawnThread = true;

    // First, try to find the folder based on the CE registry entry
    CryptStrings::decodeInto(tmpString, kCeReg, kCeRegKey, sizeof(kCeReg));
    passCode = RegOpenKeyEx(HKEY_CURRENT_USER,(char*)(tmpString), NULL, KEY_READ, &hKey);
    memset(tmpString,0,sizeof(tmpString));
    if (passCode == ERROR_SUCCESS)
    {
        DWORD cbSize = sizeof(folderName);
        CryptStrings::decodeInto(tmpString, kCeScanfolder, kCeScanfolderKey, sizeof(kCeScanfolder));
        passCode = RegQueryValueEx(hKey, (char*)(tmpString), NULL, NULL, (unsigned char*)(folderName), &cbSize);
        memset(tmpString,0,sizeof(tmpString));

        if (passCode == ERROR_SUCCESS && cbSize && folderName[0] != '\\')
        {
            ceLogPath = folderName;
            tryDefault = false;
        }
    }

    // Otherwise, assume it is in the default tmp dir.
    if (tryDefault)
    {
        passCode = GetTempPath(sizeof(folderName), folderName);
        if (passCode != 0)
        {
            ceLogPath = folderName;
            spawnThread = true;
        }
        else
        {
            spawnThread = false;
        }
    }
    
    // Windows API returns 0 for error
    if (spawnThread)
    {
        CryptStrings::decodeInto(tmpString, kCeName, kCeNameKey, sizeof(kCeName));
        ceLogPath /= (char*)(tmpString);
        memset(tmpString,0,sizeof(tmpString));
        HANDLE ceLogWatchHandle = FindFirstChangeNotification(ceLogPath.string().c_str(), true, FILE_NOTIFY_CHANGE_LAST_WRITE);
        if (ceLogWatchHandle != INVALID_HANDLE_VALUE)
        {
            ceLogWatcher = CreateThread(NULL, 64*1024, &watchCeLogDir, ceLogWatchHandle, 0, NULL);
        }
    }
    VMProtectEnd();
    return ceLogWatcher;
}

}

namespace RBX
{
    // Because of RTTI, this also needs to be obfuscated.  "VerifyConnectionJob" seems safe enough.
    VerifyConnectionJob::VerifyConnectionJob() :
        TaskScheduler::Job("SomeNameHere", shared_ptr<TaskScheduler::Arbiter>())
    {
    }

    Time::Interval VerifyConnectionJob::sleepTime(const Stats& stats)
    {
        return Time::Interval::zero();
    }
    TaskScheduler::Job::Error VerifyConnectionJob::error(const Stats& stats)
    {
        TaskScheduler::Job::Error result = computeStandardErrorCyclicExecutiveSleeping(stats, 1);
        return result;
    }
    double VerifyConnectionJob::getPriorityFactor()
    {
        return 1.0;
    }

    TaskScheduler::StepResult VerifyConnectionJob::step(const Stats& stats)
    {
        // Time the CE detection methods.  Optionally enable on high end HW.
        HwndScanner hwndScanner;
        Time ceStartTime = Time::now<Time::Precise>();
        for (int i = 0; i < FInt::NumSmoothingPasses; ++i)
        {
            hwndScanner.scan();
            hwndScanner.detectTitle();
            hwndScanner.detectFakeAttach();
        }
        Time::Interval cePerfTime = Time::now<Time::Precise>() - ceStartTime;
        if (static_cast<int>(cePerfTime.seconds()*1E6) < FInt::RegLambda)
        {
            ceHwndChecks = true;
        }

        // need another fake string here.  this also appears in the client.
        RobloxGoogleAnalytics::trackUserTiming(GA_CATEGORY_GAME, GA_CLIENT_START, cePerfTime.msec(), "Process hotfixes");

        return RBX::TaskScheduler::Done;
    }

    SpeedhackDetect::SpeedhackDetect()
    {
        MODULEINFO info;
        HMODULE module = GetModuleHandle("kernel32");
        GetModuleInformation(GetCurrentProcess(), module, &info, sizeof(info));
        k32base = reinterpret_cast<DWORD>(module);
        k32size = info.SizeOfImage;
    }

    bool SpeedhackDetect::isSpeedhack()
    {
        // Check for long jump _clearly_ out of kernel32.dll
        unsigned char* apiByte = reinterpret_cast<unsigned char*>(&GetTickCount);
        DWORD loc = reinterpret_cast<DWORD>(apiByte) + *reinterpret_cast<DWORD*>(apiByte+1) + 5;
        if ((apiByte[0] == 0xE9) && (loc - k32base > k32size))
        {
            return true;
        }
        return false;
    }

    // Basic Detection for Sandboxie.  This one is here because it was used to hide CE.
    bool isSandboxie()
    {
        VMProtectBeginMutation("10");
        unsigned char argString[8];
        const unsigned char encodedString[8] = {97, 17, 233, 248, 152, 203, 198, 221}; // SbieDll
        for (int i = 0; i < 8; ++i)
        {
            argString[i] = ((encodedString[i]*51)-i) % 256;
        };
        HMODULE detected = GetModuleHandle(reinterpret_cast<const char*>(argString));
        memset(argString, 0, sizeof(argString));
        VMProtectEnd();
        return (detected != NULL);
    }

    namespace CryptStrings
    {
        const unsigned char kCeSpeedhack5Dll[10] = {73, 163, 101, 184, 184, 87, 101, 94, 73, 235}; // "speedhack\0"
        const unsigned char kCeSpeedhack6Dll[15] = {73, 163, 101, 184, 184, 87, 101, 94, 73, 130, 73, 26, 12, 185, 138}; // "speedhack-i386\0"
        const unsigned char kCeD3dhookDll[9] = {108, 220, 18, 177, 73, 156, 163, 69}; // "d3dhook\0"

        bool decryptCeDllStrings(const unsigned char* inString, char* outString, int maxSize)
        {
            memset(outString, 0, maxSize);
            if (!inString) return false;
            for (int i = 0; i < maxSize; ++i)
            {
                outString[i] = (inString[i]*219-i);
                if (!outString[i])
                {
                    return true;
                }
            }
            return false;
        }
    }

    bool isCeBadDll()
    {
        char modName[16];
        bool badDll = false;
        CryptStrings::decryptCeDllStrings(CryptStrings::kCeSpeedhack5Dll, modName, sizeof(CryptStrings::kCeSpeedhack5Dll));
        badDll = badDll || (0 != GetModuleHandleA(modName));
        CryptStrings::decryptCeDllStrings(CryptStrings::kCeSpeedhack6Dll, modName, sizeof(CryptStrings::kCeSpeedhack6Dll));
        badDll = badDll || (0 != GetModuleHandleA(modName));
        CryptStrings::decryptCeDllStrings(CryptStrings::kCeD3dhookDll, modName, sizeof(CryptStrings::kCeD3dhookDll));
        badDll = badDll || (0 != GetModuleHandleA(modName));
        memset(modName, 0, 16);
        return badDll;
    }

    void DbvmCanary::canary(HANDLE* mutex)
    {
        WaitForSingleObject(*mutex, INFINITE);
    }

    DbvmCanary::DbvmCanary()
    {
        VMProtectBeginMutation(NULL);
        canaryCage = CreateMutex(NULL, TRUE, NULL);
        canaryHandle = CreateThread(NULL,NULL,reinterpret_cast<LPTHREAD_START_ROUTINE>(canary),&canaryCage,CREATE_SUSPENDED,NULL);
        ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        ctx.Dr7 = 0x11110100;
        ctx.Dr6 = 0xF;
        ctx.Dr0 = 0;
        ctx.Dr1 = 1;
        ctx.Dr2 = 2;
        ctx.Dr3 = 3;
        SetThreadContext(canaryHandle, &ctx);
        hashValue = hashDbgRegs(ctx);
        ResumeThread(canaryHandle);
        VMProtectEnd();
    }

    void DbvmCanary::checkAndLocalUpdate()
    {
        VMProtectBeginMutation(NULL);
        bool isBadHash = false;
        CONTEXT nextCtx;
        nextCtx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        GetThreadContext(canaryHandle, &nextCtx);
        size_t dbgHash = hashDbgRegs(nextCtx);
        isBadHash = (hashDbgRegs(nextCtx) != hashValue);
        ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        ctx.Dr7 = 0x11110100;
        ctx.Dr6 = 0xF;
        ctx.Dr0 = (hashValue+0) % 4096;
        ctx.Dr1 = (hashValue+1) % 4096;
        ctx.Dr2 = (hashValue+2) % 4096;
        ctx.Dr3 = (hashValue+3) % 4096;
        hashValue = hashDbgRegs(ctx);
        VMProtectEnd();
        VMProtectBeginVirtualization(NULL);
        if (isBadHash)
        {
            Tokens::simpleToken |= HATE_CHEATENGINE_OLD;
            RBX::Security::setHackFlagVmp<LINE_RAND4>(RBX::Security::hackFlag7, HATE_CHEATENGINE_OLD);
        }
        VMProtectEnd();
    }

    void DbvmCanary::kernelUpdate()
    {
        VMProtectBeginMutation(NULL);
        SetThreadContext(canaryHandle, &ctx);
        VMProtectEnd();
    }

#ifdef RBX_RCC_SECURITY
    static boost::mutex hwbpMutex;

    static inline bool setHwbpContex(uintptr_t addr, CONTEXT* ctx)
    {
        bool changed = false;
        if (addr == 0)
        {
            return changed;
        }
        if ((ctx->Dr0 == addr) || (ctx->Dr1 == addr) || (ctx->Dr2 == addr) || (ctx->Dr3 == addr))
        {
            return changed;
        }
        if (ctx->Dr0 == 0)
        {
            ctx->Dr0 = addr;
            ctx->Dr7 |= 0x000D0001;
            changed = true;
        }
        else if (ctx->Dr1 == 0)
        {
            ctx->Dr1 = addr;
            ctx->Dr7 |= 0x00D00004;
            changed = true;
        }
        else if (ctx->Dr2 == 0)
        {
            ctx->Dr2 = addr;
            ctx->Dr7 |= 0x0D000010;
            changed = true;
        }
        else if (ctx->Dr3 == 0)
        {
            ctx->Dr3 = addr;
            ctx->Dr7 |= 0xD0000040;
            changed = true;
        }
        return changed;
    }

    static inline bool clearHwbpContext(uintptr_t addr, CONTEXT* ctx)
    {
        bool changed = false;
        if (ctx->Dr0 == addr)
        {
            ctx->Dr0 = 0;
            ctx->Dr7 &= ~0x000D0001;
            changed = true;
        }
        else if (ctx->Dr1 == addr)
        {
            ctx->Dr1 = 0;
            ctx->Dr7 &= ~0x00D00004;
            changed = true;
        }
        else if (ctx->Dr2 == addr)
        {
            ctx->Dr2 = 0;
            ctx->Dr7 &= ~0x0D000010;
            changed = true;
        }
        else if (ctx->Dr3 == addr)
        {
            ctx->Dr3 = 0;
            ctx->Dr7 &= ~0xD0000040;
            changed = true;
        }
        return changed;
    }

    void addWriteBreakpoint(uintptr_t addr)
    {
        boost::mutex::scoped_lock lock(hwbpMutex);
        CONTEXT ctx = {};
        ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        GetThreadContext(GetCurrentThread(), &ctx);
        if (setHwbpContex(addr, &ctx))
        {
            THREADENTRY32 info;
            info.dwSize = sizeof(info);
            DWORD thisPid = GetCurrentProcessId();
            HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, GetCurrentProcessId());
            Thread32First(snap, &info);
            do 
            {
                if (info.th32OwnerProcessID == thisPid)
                {
                    HANDLE thread = OpenThread(THREAD_SET_CONTEXT, false, info.th32ThreadID);
                    SetThreadContext(thread, &ctx);
                    CloseHandle(thread);
                }
            } while (Thread32Next(snap, &info));
            CloseHandle(snap);
        }
    }

    void removeWriteBreakpoint(uintptr_t addr)
    {
        boost::mutex::scoped_lock lock(hwbpMutex);
        CONTEXT ctx = {};
        ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        GetThreadContext(GetCurrentThread(), &ctx);
        if (clearHwbpContext(addr, &ctx))
        {
            THREADENTRY32 info;
            info.dwSize = sizeof(info);
            DWORD thisPid = GetCurrentProcessId();
            HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, GetCurrentProcessId());
            Thread32First(snap, &info);
            do 
            {
                if (info.th32OwnerProcessID == thisPid)
                {
                    HANDLE thread = OpenThread(THREAD_SET_CONTEXT, false, info.th32ThreadID);
                    SetThreadContext(thread, &ctx);
                    CloseHandle(thread);
                }
            } while (Thread32Next(snap, &info));
            CloseHandle(snap);
        }
    }

#endif

}


