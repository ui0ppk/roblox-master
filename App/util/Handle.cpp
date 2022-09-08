#include "stdafx.h"

#include "Util/Handle.h"
#include "Util/Utilities.h"
#include "RbxAssert.h"
#include <limits>
#include "boost/lexical_cast.hpp"
#include "reflection/Object.h"

namespace RBX {

	InstanceHandle::InstanceHandle(Reflection::DescribedBase* target)
		:target(shared_from(target))
	{
	}

	bool InstanceHandle::operatorLess(const InstanceHandle& other) const
	{
		return target < other.target;
	}
	bool InstanceHandle::empty() const {
		return !target;
	}

	void InstanceHandle::linkTo(shared_ptr<Reflection::DescribedBase> target) {
		this->target = target;
	}



}	// end namespace RBX