 /* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/KernelJoint.h"
#include "V8World/Primitive.h"
#include "V8Kernel/Kernel.h"
#include "V8Kernel/Constants.h"
#include "V8Kernel/Connector.h"
#include "V8Kernel/Body.h"

namespace RBX {


void KernelJoint::putInKernel(Kernel* _kernel)
{
	Super::putInKernel(_kernel);

	_kernel->insertConnector(this);
}


void KernelJoint::removeFromKernel()
{
	getKernel()->removeConnector(this);

	Super::removeFromKernel();
}




} // namespace


// Randomized Locations for hackflags
namespace RBX 
{ 
    namespace Security
    {
        unsigned int hackFlag8 = 0;
    };
};
