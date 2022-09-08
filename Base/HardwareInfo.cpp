#include "boost/lexical_cast.hpp"
#include "geekinfo.h"

namespace RBX {



unsigned char CPUCount(unsigned int *TotAvailLogical,
					   unsigned int *TotAvailCore,
					   unsigned int *PhysicalNum)
{
	// this is using GeekInfo library. EL.
	unsigned char StatusFlag(0);
	std::string val = systemMetric(kSystemMetricCPULogicalCount);
	try { *TotAvailLogical = boost::lexical_cast<unsigned int>(val.c_str());
	} catch (boost::bad_lexical_cast&)  { StatusFlag = 1; }
    val 	= systemMetric(kSystemMetricCPUCoreCount);
    try { *TotAvailCore 	= boost::lexical_cast<unsigned int>(val.c_str());
	} catch (boost::bad_lexical_cast&)  { StatusFlag = 1; }
    val 	= systemMetric(kSystemMetricCPUPhysicalCount);
    try { *PhysicalNum 	= boost::lexical_cast<unsigned int>(val.c_str());
	} catch (boost::bad_lexical_cast&)  { StatusFlag = 1; }

	return StatusFlag; // 0 == success
}

} // namespace

#if 0
#include <iostream>

// UNIT TEST (by eric l.)!

// to test this, use this line in the console (with gcc).
//
// g++ -IGeekInfo/src/geekinfo-2.1.4/include -I../boost_1_38_0/include HardwareInfo.cpp -o runHardareInfo -Wl,-syslibroot,/Developer/SDKs/MacOSX10.5.sdk -mmacosx-version-min=10.5 -arch i386 GeekInfo/src/geekinfo-2.1.4/build.x86_32/libgeekinfo.a -LGeekInfo/src/geekinfo-2.1.4/build.x86_32/lib -framework Carbon -framework IOKit -framework Cocoa -framework WebKit -framework AGL -framework OpenGL
//
//

int main() {
   std::cout << systemMetricName(kSystemMetricCPU) << ": " << systemMetric(kSystemMetricCPU) << std::endl;
   unsigned int l, c, p;
   NewCPUCount( &l, &c, &p );
   fprintf(stderr,"logical: %d Core: %d Physical: %d\n", l, c, p);
   return 0;
}

#endif

