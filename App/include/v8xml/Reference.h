
#ifndef _568E368F53F1431aB7D4923F6D45021A
#define _568E368F53F1431aB7D4923F6D45021A

#include "rbx/Debug.h"

#include "Util/Object.h"
#include "Util/Handle.h"

#include <list>
#include <map>
#include <set>
#include <string>

class XmlNameValuePair;

namespace RBX {

	namespace Reflection
	{
		class DescribedBase;
	}
	
	// Used for streaming
	class RBXInterface IIDREF
	{
	private:
		friend class IReferenceBinder;
		virtual void assignIDREF(Reflection::DescribedBase* propertyOwner, const InstanceHandle& handle) const = 0;
	};

	// Used for streaming
	class RBXBaseClass IReferenceBinder
	{
	public:
		virtual void announceID(const XmlNameValuePair* valueID, Reflection::DescribedBase* source) = 0;
		virtual void announceIDREF(const XmlNameValuePair* valueIDREF, Reflection::DescribedBase* propertyOwner, const IIDREF* idref) = 0;
		virtual bool resolveRefs() = 0;
		virtual ~IReferenceBinder(){}
	protected:
		void assign(const IIDREF* idref, Reflection::DescribedBase* propertyOwner, const InstanceHandle& handle) {
			idref->assignIDREF(propertyOwner, handle);
		}
	};

}

#endif
