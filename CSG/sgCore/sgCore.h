#ifndef   __sgCore__
#define  __sgCore__

#include "sgDefs.h"

#if (SG_CURRENT_PLATFORM==SG_PLATFORM_MAC)
	#pragma GCC visibility push(default)
    #pragma pack(1)
#endif

#pragma pack(push, 1)

#include "sgErrors.h"
#include "sgMatrix.h"
#include "sgSpaceMath.h"
#include "sgObject.h"
#include "sg2D.h"
#include "sgGroup.h"
#include "sg3D.h"

#include "sgScene.h"
#include "sgAlgs.h"


#include "sgTD.h"
#include "sgIApp.h"
#include "sgFileManager.h"

sgCore_API    bool   sgInitKernel();
sgCore_API    void   sgFreeKernel(bool show_memleaks=true);
sgCore_API    void   sgGetVersion(int& major, int& minor, int& release, int& build);
/****************************************************************/
/*************************************************************************************/

#if (SG_CURRENT_PLATFORM==SG_PLATFORM_MAC)
	#pragma GCC visibility pop
#endif

#pragma pack( pop )

#endif    // __sgCore__