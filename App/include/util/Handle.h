
#ifndef _28C82C86EA754d62AF934FA46C3698ED
#define _28C82C86EA754d62AF934FA46C3698ED

#include <functional>
#include <algorithm>
#include <string>

#include "rbx/Debug.h"
#include "Util/Object.h"
#include "Util/Memory.h"

namespace RBX {

	namespace Reflection
	{
		class DescribedBase;
	}

	// Used to reference RBX::Reflection::DescribedBase in the XmlElement class
	class InstanceHandle {
		shared_ptr<Reflection::DescribedBase> target;
	public:
		InstanceHandle() {}
		InstanceHandle(Reflection::DescribedBase* target);
		InstanceHandle(shared_ptr<Reflection::DescribedBase> target):target(target) {}
		InstanceHandle(const InstanceHandle& other):target(other.target) {}

		InstanceHandle& operator=(const InstanceHandle& value) {
			target = value.target;
			return *this;
		}
		InstanceHandle& operator=(shared_ptr<Reflection::DescribedBase> value) {
			target = value;
			return *this;
		}
		
		bool empty() const;

		shared_ptr<Reflection::DescribedBase> getTarget() const { return target; }

		void linkTo(shared_ptr<Reflection::DescribedBase> target);

		bool operator==(const InstanceHandle& other) const { return operatorEqual(other); }
		bool operator!=(const InstanceHandle& other) const { return !operatorEqual(other); }
		bool operator<(const InstanceHandle& other) const{ return operatorLess(other); }
		bool operator>(const InstanceHandle& other) const{ return operatorGreater(other); }

	protected:
		bool operatorEqual(const InstanceHandle& other) const;
		bool operatorLess(const InstanceHandle& other) const;
		bool operatorGreater(const InstanceHandle& other) const;
	};



}

#endif
