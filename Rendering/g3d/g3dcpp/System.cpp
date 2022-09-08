/** 
  @file System.cpp
 
  @maintainer Morgan McGuire, http://graphics.cs.williams.edu

  Note: every routine must call init() first.

  There are two kinds of detection used in this file.  At compile
  time, the _MSC_VER #define is used to determine whether x86 assembly
  can be used at all.  At runtime, processor detection is used to
  determine if we can safely call the routines that use that assembly.

  @created 2003-01-25
  @edited  2010-01-03
 */

#include "G3D/platform.h"
#include "G3D/System.h"
#include "G3D/debug.h"
#include "G3D/fileutils.h"
#include "G3D/G3DGameUnits.h"
#include "G3D/Crypto.h"
#include "G3D/stringutils.h"
#include "G3D/Table.h"
#include "G3D/units.h"
#include <time.h>

#include <cstring>
#include <cstdio>

// Uncomment the following line to turn off G3D::System memory
// allocation and use the operating system's malloc.
// Turned off the G3D Bufferpool - let the system allocator do the work - our big issue is not speed of allocation,
// rather, it is locality of reference
//
// ROBLOX - DB 7/14/08
//
#define NO_BUFFERPOOL

#if defined(__i386__) || defined(__x86_64__) || defined(G3D_WIN32)
#    define G3D_NOT_OSX_PPC
#endif

#include <cstdlib>

#ifdef G3D_WIN32

#   include <sys/timeb.h>

#elif defined(G3D_LINUX) || defined(G3D_ANDROID) // ROBLOX

#   include <stdlib.h>
#   include <stdio.h>
#   include <errno.h>
#   include <sys/types.h>
#   include <sys/select.h>
#   include <termios.h>
#   include <unistd.h>
#   include <sys/ioctl.h>
#   include <sys/time.h>
#   include <pthread.h>

#elif defined(G3D_OSX) || defined(G3D_IOS) // ROBLOX

    #include <stdlib.h>
    #include <stdio.h>
    #include <errno.h>
    #include <sys/types.h>
    #include <sys/sysctl.h>
    #include <sys/select.h>
    #include <sys/time.h>
    #include <termios.h>
    #include <unistd.h>
    #include <pthread.h>
    #include <mach-o/arch.h>

    #include <sstream>
//    #include <CoreServices/CoreServices.h> // ROBLOX
#endif

// SIMM include
#if !defined(G3D_IOS) && !defined(G3D_ANDROID)// ROBLOX
#include <xmmintrin.h>
#endif


namespace G3D {


/** Checks if the CPUID command is available on the processor (called from init) */
static bool checkForCPUID();

/** Called from init */
static void getG3DVersion(std::string& s);

/** Called from init */
static G3DEndian checkEndian();


System& System::instance() {
    static System thesystem;
    return thesystem;
}


System::System() :
    m_initialized(false),
    m_cpuSpeed(0),
    m_hasCPUID(false),
    m_hasRDTSC(false),
    m_hasMMX(false),
    m_hasSSE(false),
    m_hasSSE2(false),
    m_hasSSE3(false),
    m_has3DNOW(false),
    m_has3DNOW2(false),
    m_hasAMDMMX(false),
    m_cpuVendor("Uninitialized"),
    m_numCores(1),
    m_machineEndian(G3D_LITTLE_ENDIAN),
    m_cpuArch("Uninitialized"),
    m_operatingSystem("Uninitialized"),
    m_version("Uninitialized"),
    m_outOfMemoryCallback(NULL),
    m_realWorldGetTickTime0(0),
    m_highestCPUIDFunction(0) {

    init();
}


void System::init() {
    // NOTE: Cannot use most G3D data structures or utility functions
    // in here because they are not initialized.

    if (m_initialized) {
        return;
    } else {
        m_initialized = true;
    }

    getG3DVersion(m_version);
    
    m_machineEndian = checkEndian();

    m_hasCPUID = checkForCPUID();
    // Process the CPUID information
    if (m_hasCPUID) {
        // We read the standard CPUID level 0x00000000 which should
        // be available on every x86 processor.  This fills out
        // a string with the processor vendor tag.
        unsigned int eaxreg = 0, ebxreg = 0, ecxreg = 0, edxreg = 0;

        cpuid(CPUID_VENDOR_ID, eaxreg, ebxreg, ecxreg, edxreg);

        {
            char c[100];
            // Then we connect the single register values to the vendor string
            *((unsigned int*) c)         = ebxreg;
            *((unsigned int*) (c + 4))   = edxreg;
            *((unsigned int*) (c + 8))   = ecxreg;
            c[12] = '\0';
            m_cpuVendor = c;
        }

        switch (ebxreg) {
        case 0x756E6547:        // GenuineIntel
            m_cpuArch = "Intel Processor";
            break;
            
        case 0x68747541:        // AuthenticAMD
            m_cpuArch = "AMD Processor";
            break;

        case 0x69727943:        // CyrixInstead
            m_cpuArch = "Cyrix Processor";
            break;

        default:
            m_cpuArch = "Unknown Processor Vendor";
            break;
        }


        unsigned int highestFunction = eaxreg;
        if (highestFunction >= CPUID_NUM_CORES) {
            cpuid(CPUID_NUM_CORES, eaxreg, ebxreg, ecxreg, edxreg);
            // Number of cores is in (eax>>26) + 1
            m_numCores = (eaxreg >> 26) + 1;
        }

        cpuid(CPUID_GET_HIGHEST_FUNCTION, m_highestCPUIDFunction, ebxreg, ecxreg, edxreg);
    }


    // Get the operating system name (also happens to read some other information)
#    ifdef RBX_PLATFORM_DURANGO

    m_hasSSE = true;
    m_hasSSE2 = true;
    m_hasSSE3 = true;
    m_numCores = 6;
    m_operatingSystem = "Durango";
    m_cpuSpeed = 999;
     
#    elif defined(G3D_WIN32)
		bool success = false;
        // Note that this overrides some of the values computed above
        m_cpuSpeed = 999;

        SYSTEM_INFO systemInfo;
        GetSystemInfo(&systemInfo);
        const char* arch = NULL;
        switch (systemInfo.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_INTEL:
            arch = "Intel";
            break;
    
        case PROCESSOR_ARCHITECTURE_MIPS:
            arch = "MIPS";
            break;

        case PROCESSOR_ARCHITECTURE_ALPHA:
            arch = "Alpha";
            break;

        case PROCESSOR_ARCHITECTURE_PPC:
            arch = "Power PC";
            break;

        default:
            arch = "Unknown";
        }

        m_numCores = systemInfo.dwNumberOfProcessors;
        uint32 maxAddr = (uint32)systemInfo.lpMaximumApplicationAddress;
        {
            char c[1024];
            sprintf(c, "%d x %d-bit %s processor",
                    systemInfo.dwNumberOfProcessors,
                    (int)(::log((double)maxAddr) / ::log(2.0) + 2.0),
                    arch);
            m_cpuArch = c;
        }

        OSVERSIONINFO osVersionInfo;
        osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        success = GetVersionEx(&osVersionInfo) != 0;

        if (success) {
            char c[1000];
            sprintf(c, "Windows %d.%d build %d Platform %d %s",
                osVersionInfo.dwMajorVersion,
                osVersionInfo.dwMinorVersion,
                osVersionInfo.dwBuildNumber,
                osVersionInfo.dwPlatformId,
                osVersionInfo.szCSDVersion);
            m_operatingSystem = c;
        } else {
            m_operatingSystem = "Windows";
        }
    
#    elif defined(G3D_LINUX) || defined(G3D_FREEBSD)

        {
            // Find the operating system using the 'uname' command
            FILE* f = popen("uname -a", "r");

            int len = 100;
            char* r = (char*)::malloc(len * sizeof(char));
            fgets(r, len, f);
            // Remove trailing newline
            if (r[strlen(r) - 1] == '\n') {
                r[strlen(r) - 1] = '\0';
            }
            fclose(f);

            m_operatingSystem = r;
            ::free(r);
        }

#   elif defined(G3D_OSX) || defined(G3D_IOS) // ROBLOX
    
#if defined(G3D_OSX) // ROBLOX

        // Operating System:
        SInt32 macVersion;
        Gestalt(gestaltSystemVersion, &macVersion);

        int major = (macVersion >> 8) & 0xFF;
        int minor = (macVersion >> 4) & 0xF;
        int revision = macVersion & 0xF;

        {
            char c[1000];
            sprintf(c, "OS X %x.%x.%x", major, minor, revision); 
            m_operatingSystem = c;
        }
                 
        // Clock Cycle Timing Information:
        Gestalt('pclk', &m_OSXCPUSpeed);
        m_cpuSpeed = iRound((double)m_OSXCPUSpeed / (1024 * 1024));
        m_secondsPerNS = 1.0 / 1.0e9;
#else // ROBLOX
    mach_timebase_info( &m_info ); // ROBLOX
#endif
    
        // System Architecture:
	const NXArchInfo* pInfo = NXGetLocalArchInfo();
		
	if (pInfo) {
	    m_cpuArch = pInfo->description;
			
	    switch (pInfo->cputype) {
	    case CPU_TYPE_POWERPC:
	        switch(pInfo->cpusubtype){
		case CPU_SUBTYPE_POWERPC_750:
		case CPU_SUBTYPE_POWERPC_7400:
		case CPU_SUBTYPE_POWERPC_7450:
		    m_cpuVendor = "Motorola";
		    break;
		case CPU_SUBTYPE_POWERPC_970:
		    m_cpuVendor = "IBM";
		    break;
		}
		break;
	    
            case CPU_TYPE_I386:
                m_cpuVendor = "Intel";
                break;
	    }
	}
#   endif

    initTime();

    getStandardProcessorExtensions();
}


void getG3DVersion(std::string& s) {
    char cstr[100];
    if ((G3D_VER % 100) != 0) {
        sprintf(cstr, "G3D %d.%02d beta %d",
                G3D_VER / 10000,
                (G3D_VER / 100) % 100,
                G3D_VER % 100);
    } else {
        sprintf(cstr, "G3D %d.%02d",
                G3D_VER / 10000,
                (G3D_VER / 100) % 100);
    }
    s = cstr;
}



const std::string& System::build() {
    const static std::string b =
#   ifdef _DEBUG
        "Debug";
#   else 
        "Release";
#   endif

    return b;
}


static G3DEndian checkEndian() {
    int32 a = 1;
    if (*(uint8*)&a == 1) {
        return G3D_LITTLE_ENDIAN;
    } else {
        return G3D_BIG_ENDIAN;
    }
}


static bool checkForCPUID() {
    // all known supported architectures have cpuid
    // add cases for incompatible architectures if they are added
    // e.g., if we ever support __powerpc__ being defined again

    return true;
}


void System::getStandardProcessorExtensions() {
#if ( ! defined(G3D_OSX) && !defined(G3D_IOS)) || defined(G3D_OSX_INTEL) // ROBLOX
    if (! m_hasCPUID) {
        return;
    }

    uint32 eaxreg = 0, ebxreg = 0, ecxreg = 0, features = 0;

    cpuid(CPUID_PROCESSOR_FEATURES, eaxreg, ebxreg, ecxreg, features);

#   define checkBit(var, bit)   ((var & (1 << bit)) ? true : false)

    m_hasRDTSC    = checkBit(features, 4);
    m_hasMMX      = checkBit(features, 23);
    m_hasSSE      = checkBit(features, 25);
    m_hasSSE2     = checkBit(features, 26);
    // Bit 28 is HTT; not checked by G3D

    m_hasSSE3     = checkBit(ecxreg, 0);

    if (m_highestCPUIDFunction >= CPUID_EXTENDED_FEATURES) {
        cpuid(CPUID_EXTENDED_FEATURES, eaxreg, ebxreg, ecxreg, features);
        m_hasAMDMMX = checkBit(features, 22);  // Only on AMD
        m_has3DNOW  = checkBit(features, 31);  // Only on AMD
        m_has3DNOW2 = checkBit(features, 30);  // Only on AMD
    } else {
        m_hasAMDMMX = false;
        m_has3DNOW  = false;
        m_has3DNOW2 = false;
    }

#   undef checkBit
#endif
}


void System::memcpy(void* dst, const void* src, size_t numBytes) {
    ::memcpy(dst, src, numBytes);
}



void System::memset(void* dst, uint8 value, size_t numBytes) {
    ::memset(dst, value, numBytes);
}


/** Removes the 'd' that icompile / Morgan's VC convention appends. */
static std::string computeAppName(const std::string& start) { return "Roblox"; }


std::string& System::appName() {
    static std::string n = "Roblox";
    return n;
}


std::string System::currentProgramFilename() {
    char filename[2048];

#   ifdef G3D_WIN32
    {
        GetModuleFileNameA(NULL, filename, sizeof(filename));
    } 
#   elif defined(G3D_OSX) || defined(G3D_IOS) // ROBLOX
    {
        // Run the 'ps' program to extract the program name
        // from the process ID.
        int pid;
        FILE* fd;
        char cmd[80];
        pid = getpid();
        sprintf(cmd, "ps -p %d -o comm=\"\"", pid);

        fd = popen(cmd, "r");
        int s = fread(filename, 1, sizeof(filename), fd);
        // filename will contain a newline.  Overwrite it:
        filename[s - 1] = '\0';
    }
#   else
    {
        int ret = readlink("/proc/self/exe", filename, sizeof(filename));
            
        // In case of an error, leave the handling up to the caller
        if (ret == -1) {
            return "";
        }
            
        debugAssert((int)sizeof(filename) > ret);
            
        // Ensure proper NULL termination
        filename[ret] = 0;      
    }
    #endif

    return filename;
}


void System::sleep(RealTime t) {

    // Overhead of calling this function, measured from a previous run.
    static const RealTime OVERHEAD = 0.00006f;

    RealTime now = time();
    RealTime wakeupTime = now + t - OVERHEAD;

    RealTime remainingTime = wakeupTime - now;
    RealTime sleepTime = 0;

    // On Windows, a "time slice" is measured in quanta of 3-5 ms (http://support.microsoft.com/kb/259025)
    // Sleep(0) yields the remainder of the time slice, which could be a long time.
    // A 1 ms minimum time experimentally kept the "Empty GApp" at nearly no CPU load at 100 fps,
    // yet nailed the frame timing perfectly.
    static RealTime minRealSleepTime = 3 * units::milliseconds();

    while (remainingTime > 0) {

        if (remainingTime > minRealSleepTime * 2.5) {
            // Safe to use Sleep with a time... sleep for half the remaining time
            sleepTime = max(remainingTime * 0.5, 0.0005);
        } else if (remainingTime > minRealSleepTime) {
            // Safe to use Sleep with a zero time;
            // causes the program to yield only
            // the current time slice, and then return.
            sleepTime = 0;
        } else {
            // Not safe to use Sleep; busy wait
            sleepTime = -1;
        }

        if (sleepTime >= 0) {
            #ifdef G3D_WIN32
                // Translate to milliseconds
                Sleep((int)(sleepTime * 1e3));
            #else
                // Translate to microseconds
                usleep((int)(sleepTime * 1e6));
            #endif
        }

        now = time();
        remainingTime = wakeupTime - now;
    }
}


void System::initTime() {
    #ifdef G3D_WIN32
        if (QueryPerformanceFrequency((LARGE_INTEGER*)&m_counterFrequency)) {
            QueryPerformanceCounter((LARGE_INTEGER*)&m_start);
        }

        struct _timeb t;
        _ftime(&t);
        
        m_realWorldGetTickTime0 = (RealTime)t.time - t.timezone * G3D::MINUTE + (t.dstflag ? G3D::HOUR : 0);

    #else
        gettimeofday(&m_start, NULL);
        // "sse" = "seconds since epoch".  The time
        // function returns the seconds since the epoch
        // GMT (perhaps more correctly called UTC). 
        time_t gmt = ::time(NULL);
        
        // No call to free or delete is needed, but subsequent
        // calls to asctime, ctime, mktime, etc. might overwrite
        // local_time_vals. 
        tm* localTimeVals = localtime(&gmt);
    
        time_t local = gmt;
        
        if (localTimeVals) {
            // tm_gmtoff is already corrected for daylight savings.
            local = local + localTimeVals->tm_gmtoff;
        }
        
        m_realWorldGetTickTime0 = local;
    #endif
}


RealTime System::time() { 
#   ifdef G3D_WIN32
        uint64_t now;
        QueryPerformanceCounter((LARGE_INTEGER*)&now);

        return ((RealTime)(now - instance().m_start) /
                instance().m_counterFrequency) + instance().m_realWorldGetTickTime0;
#   else
        // Linux resolution defaults to 100Hz.
        // There is no need to do a separate RDTSC call as gettimeofday
        // actually uses RDTSC when on systems that support it, otherwise
        // it uses the system clock.
        struct timeval now;
        gettimeofday(&now, NULL);

        return (now.tv_sec  - instance().m_start.tv_sec) +
            (now.tv_usec - instance().m_start.tv_usec) / 1e6
            + instance().m_realWorldGetTickTime0;
#   endif
}


////////////////////////////////////////////////////////////////

#define REALPTR_TO_USERPTR(x)   ((uint8*)(x) + sizeof (void *))
#define USERPTR_TO_REALPTR(x)   ((uint8*)(x) - sizeof (void *))
#define REALBLOCK_SIZE(x)       ((x) + sizeof (void *))


void* System::malloc(size_t bytes) 
{
    return ::malloc(bytes);
}

void* System::calloc(size_t n, size_t x) 
{
    return ::calloc(n, x);
}


void* System::realloc(void* block, size_t bytes) 
{
    return ::realloc(block, bytes);
}


void System::free(void* p) 
{
    return ::free(p);
}


std::string System::currentDateString() {
    time_t t1;
    ::time(&t1);
    tm* t = localtime(&t1);
    return format("%d-%02d-%02d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday); 
}

#if defined(_MSC_VER) && !defined(RBX_PLATFORM_DURANGO)


// VC on Intel
void System::cpuid(CPUIDFunction func, uint32& areg, uint32& breg, uint32& creg, uint32& dreg) {
    // Can't copy from assembler direct to a function argument (which is on the stack) in VC.
    uint32 a,b,c,d;

    // Intel assembler syntax
    __asm {
        mov	  eax, func      //  eax <- func
        mov   ecx, 0
        cpuid              
        mov   a, eax   
        mov   b, ebx   
        mov   c, ecx   
        mov   d, edx
    }
    areg = a;
    breg = b; 
    creg = c;
    dreg = d;
}

#elif (defined(RBX_PLATFORM_DURANGO) || defined(G3D_OSX) || defined(G3D_IOS) || defined(G3D_ANDROID)) && ! defined(G3D_OSX_INTEL)

// no CPUID
void System::cpuid(CPUIDFunction func, uint32& eax, uint32& ebx, uint32& ecx, uint32& edx) {
    eax = 0;
    ebx = 0;
    ecx = 0;
    edx = 0;
}

#elif !defined(RBX_PLATFORM_DURANGO)

// See http://sam.zoy.org/blog/2007-04-13-shlib-with-non-pic-code-have-inline-assembly-and-pic-mix-well
// for a discussion of why the second version saves ebx; it allows 32-bit code to compile with the -fPIC option.
// On 64-bit x86, PIC code has a dedicated rip register for PIC so there is no ebx conflict.
void System::cpuid(CPUIDFunction func, uint32& eax, uint32& ebx, uint32& ecx, uint32& edx) { eax = 0;
#if ! defined(__PIC__) || defined(__x86_64__)
    // AT&T assembler syntax
    asm volatile(
                 "movl $0, %%ecx   \n\n" /* Wipe ecx */
                 "cpuid            \n\t"
                 : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                 : "a"(func));
#else
    // AT&T assembler syntax
    asm volatile(
                 "pushl %%ebx      \n\t" /* save ebx */
                 "movl $0, %%ecx   \n\n" /* Wipe ecx */
                 "cpuid            \n\t"
                 "movl %%ebx, %1   \n\t" /* save what cpuid just put in %ebx */
                 "popl %%ebx       \n\t" /* restore the old ebx */
                 : "=a"(eax), "=r"(ebx), "=c"(ecx), "=d"(edx)
                 : "a"(func));
#endif
}

#endif

}  // namespace
