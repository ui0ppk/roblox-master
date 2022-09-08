#include "rbx/SystemUtil.h"

#include <sstream>

#include <sys/sysctl.h>
#include <mach/mach.h>
#include <mach/machine.h>

#if RBX_PLATFORM_IOS
std::string getFriendlyDeviceName();
std::string getDeviceOSVersion();
#else
#include <ApplicationServices/ApplicationServices.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/kext/KextManager.h>

// This is a private API in CoreFoundation (from CFPriv.h)
// Required for compilation to succeed, as is used below in osVer() call, at link it will find the defination in Core Foundation.
extern "C" CFStringRef CFCopySystemVersionString();
#endif

//Helpers
namespace
{
    static bool sysctlbynameuint64(const char * name, uint64_t * value)
    {
        size_t  size = sizeof(uint64_t);
        int     result;

        result = sysctlbyname(name, value, &size, NULL, 0);

        if (result != -1) {    
            switch(size) {
            case sizeof(uint64_t):
                break;
            case sizeof(uint32_t):
                *value = *(uint32_t *)value;
                break;
            case sizeof(uint16_t):
                *value = *(uint16_t *)value;
                break;
            case sizeof(uint8_t):
                *value = *(uint8_t *)value;
                break;
            }
            return true;
        } else {
            return false;
        }
    }

    static uint64_t sysctlbynameuint64(const char* name)
    {   
        uint64_t    result;

        if (sysctlbynameuint64(name, &result)) {
            return result;
        } else {
            return 0;
        }
    }

    static std::string sysctlbynamestring(const char * name)
    {
        std::string result;
        size_t      vsize = 512;
        char        vstr[ 512 ];

        sysctlbyname(name, vstr, &vsize, NULL, 0);
        result = vstr;
        return result;
    }
} // namespace

namespace RBX
{
namespace SystemUtil
{
	std::string getCPUMake()
	{
		return sysctlbynamestring("machdep.cpu.brand_string");  
	}

	uint64_t getCPUSpeed() 
	{
		return ( sysctlbynameuint64("hw.cpufrequency") / 1048576 );
	}

	uint64_t getCPULogicalCount()
	{
		uint64_t  result;

		if (!sysctlbynameuint64("hw.logicalcpu", &result)) 
		{
			if (!sysctlbynameuint64("hw.ncpu", &result)) 
			{
				result = 1;
			}
		}
		return result;
	}

	uint64_t getCPUCoreCount()
	{
		uint64_t result = 0;
		if (!sysctlbynameuint64("hw.physicalcpu", &result)) 
		{
			result = getCPULogicalCount();
		}
		return result;
	}

	uint64_t getCPUPhysicalCount()
	{
	  uint64_t result = 0;

	  if (!sysctlbynameuint64("hw.packages", &result)) 
	  {
		uint64_t ncpu = 0;
		uint64_t nlpp = 0;

		if (sysctlbynameuint64("hw.ncpu", &ncpu) && sysctlbynameuint64("machdep.cpu.logical_per_package", &nlpp)) 
		{
		  result = ncpu / nlpp;
		} 
		else 
		{
		  if (!sysctlbynameuint64("hw.physicalcpu", &result)) 
		  {
			if (!sysctlbynameuint64("hw.ncpu", &result)) 
			{
			  result = 1;
			}
		  }
		}    
	  }
	  return result;
	}	

    bool isCPU64Bit()
    {
        return sysctlbynameuint64("hw.cpu64bit_capable") || sysctlbynameuint64("hw.optional.x86_64");
    }

	uint64_t getMBSysRAM()
	{
		return (sysctlbynameuint64("hw.memsize") / 1048576 );
	}

	uint64_t getMBSysAvailableRAM()
	{
		uint64_t result;
		vm_statistics_data_t vm_stat;
		int count = HOST_VM_INFO_COUNT;

		host_statistics(mach_host_self(), HOST_VM_INFO, (integer_t*)&vm_stat, (mach_msg_type_number_t*)&count);
		result = (uint64_t)(vm_stat.free_count + vm_stat.inactive_count) * sysctlbynameuint64("hw.pagesize");

		return ( result / 1048576 );
	}			
	
	uint64_t getVideoMemory()
	{	
        static uint64_t vramSize = 0;

        if (vramSize)
            return vramSize;

		// Just in case if the call fails
		// Initialized to 64 MB default, that's the lowest for any intel Mac.		
		vramSize = 67108864;
		
#if !RBX_PLATFORM_IOS
		io_service_t        	dspPort;
		short					MAXDISPLAYS = 8;
		CGDirectDisplayID		displays[MAXDISPLAYS];
		CGDisplayCount			dspCount = 0;
		CFNumberRef				vramStorage = NULL;
		CFMutableDictionaryRef  vramProp = NULL;

		// How many active displays do we have?
		CGGetOnlineDisplayList(MAXDISPLAYS, displays, &dspCount);						
		
		// Get the service port for the 1st display
		dspPort = CGDisplayIOServicePort(displays[0]);

		if (kIOReturnSuccess == IORegistryEntryCreateCFProperties(dspPort, &vramProp, kCFAllocatorDefault, kNilOptions)) 
		{
			if (CFDictionaryGetValueIfPresent(vramProp, CFSTR("IOFBMemorySize"), (const void **)&vramStorage)) 
			{
				CFNumberGetValue(vramStorage, kCFNumberSInt64Type, (void *)&vramSize);
			}
			CFRelease(vramProp);
		}
#endif
        
		return (vramSize);
	}

    std::string osPlatform()
    {
#if defined(__arm__) || defined(__aarch64__)
        return "iOS";
#elif __i386__ || __x86_64__
        return "OSX";
#else
#error "Unsupported Apple Platform"
#endif
    }

    int osPlatformId()
    {
        return 0;
    }
    
    std::string deviceName()
    {
#if RBX_PLATFORM_IOS
        return getFriendlyDeviceName();
#elif __i386__
        return "Mac";
#else
#error "Unsupported Apple Platform"
#endif
        
    }

    
    std::string osVer()
    {
#if RBX_PLATFORM_IOS
        return getDeviceOSVersion();
#elif __i386__
        std::ostringstream  s;
        CFStringRef         version = CFCopySystemVersionString();
        char                versionBuffer[ 256 ];
        int                 versionSize = 256;
        
        CFStringGetCString(version, versionBuffer, versionSize, kCFStringEncodingUTF8);
        s << "Mac OS X " << &(versionBuffer[8]);
        return s.str();
#else
#error "Unsupported Apple Platform"
#endif
        
    }
	
	std::string getGPUMake()
	{
        std::ostringstream s;
#if !RBX_PLATFORM_IOS
		io_service_t        	dspPort;

		short					MAXDISPLAYS = 8;
		CGDirectDisplayID		displays[MAXDISPLAYS];
		CGDisplayCount			dspCount = 0;
		
		
		// How many active displays do we have?
		CGGetOnlineDisplayList(MAXDISPLAYS, displays, &dspCount);						
		
		// Get the service port for the 1st display
		dspPort = CGDisplayIOServicePort(displays[0]);
		
		// get the model string from the hardware device
		CFDataRef model = (CFDataRef)IORegistryEntrySearchCFProperty(dspPort,kIOServicePlane,CFSTR("model"), kCFAllocatorDefault,kIORegistryIterateRecursively | kIORegistryIterateParents);
		std::string modelStr;
		if( !model ) {
			modelStr = std::string( "<model could not be found" );
		} else { 
			modelStr = std::string( (const char*)CFDataGetBytePtr(model), CFDataGetLength(model) );
			CFRelease( model );
		}
		s << modelStr << " drv=<driver version could not be found>";
		
		// Get the graphics controller dictionary
		io_registry_entry_t graphicsControl = IORegistryEntryFromPath( kIOMasterPortDefault, "IOService:/AppleACPIPlatformExpert/GMUX/AppleGraphicsControl" );
		if( !graphicsControl ) {
			return s.str();
		}
		
		CFMutableDictionaryRef graphicsControlDictionary;
		IORegistryEntryCreateCFProperties(graphicsControl,&graphicsControlDictionary,NULL,NULL);
		if( !graphicsControlDictionary ) {	
			return s.str();
		}
		
		// look up the GL Driver class in the Graphics Controller Dictionary
		CFStringRef configLookup = CFStringCreateWithCString(NULL, "Config1", kCFStringEncodingASCII);
		if( !configLookup ) {
			CFRelease( graphicsControlDictionary );
			return s.str();
		}
		
		CFMutableDictionaryRef config = (CFMutableDictionaryRef)CFDictionaryGetValue(graphicsControlDictionary,configLookup);
		if( !config ) {
			CFRelease( graphicsControlDictionary );
			CFRelease( configLookup );
			return s.str();
		}
		
		CFRelease( configLookup );
		
		CFStringRef glDriverLookup = CFStringCreateWithCString(NULL, "GLDriver1", kCFStringEncodingASCII);
		if( !glDriverLookup ) {
			CFRelease( graphicsControlDictionary );
			return s.str();
		}
		
		CFStringRef glDriver = (CFStringRef)CFDictionaryGetValue(config, glDriverLookup);
		if( !glDriver ) {
			CFRelease( glDriverLookup );
			CFRelease( graphicsControlDictionary );
			return s.str();
		}
		
		CFRelease( glDriverLookup );
		
		// get the bundle id from the GL Driver class	
		CFStringRef bundleName = IOObjectCopyBundleIdentifierForClass( glDriver );
		if( !bundleName ) {
			CFRelease( graphicsControlDictionary );
			return s.str();
		}
		
		CFRelease( graphicsControlDictionary );
		
		// get the bundle from the bundle id
		CFURLRef bundleURL = KextManagerCreateURLForBundleIdentifier( NULL, bundleName );
		if( !bundleURL ) {
			CFRelease( bundleName );
			return s.str();
		}
		
		CFRelease( bundleName );
		
		CFBundleRef bundle = CFBundleCreate( NULL, bundleURL );
		if( !bundle ) {
			CFRelease( bundleURL );
			return s.str();
		}
		
		CFRelease( bundleURL );
		
		// finally... get the version string from the bundle
		CFStringRef version = (CFStringRef)CFBundleGetValueForInfoDictionaryKey( bundle, CFSTR("CFBundleGetInfoString") );
		if( !version ) {
			CFRelease( bundle );
			return s.str();
		}
		
		char versionBuffer[ 256 ];
		CFIndex  versionSize = 256;
		CFStringGetCString(version, versionBuffer, versionSize, kCFStringEncodingMacRoman);
		
		CFRelease( bundle );
		
		s.str("");
		s << modelStr << " drv=" << versionBuffer;
		
#endif
		return s.str();
	}
	
	std::string getMaxRes()
	{
        std::ostringstream strstream;
#if !RBX_PLATFORM_IOS
		CGRect mainMonitor = CGDisplayBounds(CGMainDisplayID());
		CGFloat monitorHeight = CGRectGetHeight(mainMonitor);
		CGFloat monitorWidth = CGRectGetWidth(mainMonitor);
		
		strstream << monitorWidth << 'x' << monitorHeight;
#endif
		return strstream.str();
	}
} // SystemUtil
} // RBX
