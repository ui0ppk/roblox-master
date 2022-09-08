#pragma once

#include "V8World/Joint.h"
#include "V8Kernel/Connector.h"

namespace RBX {
	
	class KernelJoint 
		: public Joint
		, public Connector		// Implements "computeForce"
	{
	private:
		typedef Joint Super;
		// IPipelined
    protected:
		/*override*/ void putInKernel(Kernel* kernel);
		/*override*/ void removeFromKernel();
    private:
		// Joint
		/*override*/ JointType getJointType() const	{return Joint::KERNEL_JOINT;}

		// Connector
		/*override*/ Body* getBody(BodyIndex id) {
			RBXASSERT(inKernel());
			if (id == body0) {
				return getEngineBody();
			}
			else {
				return NULL;
			}
		}

	protected:
		/*implement*/ virtual Body* getEngineBody() = 0;
		/*override*/ KernelType getConnectorKernelType() const {return Connector::KERNEL_JOINT;}

	public:
		KernelJoint()	{}
		~KernelJoint()	{}
	};

} // namespace

