

#ifndef _E34E3E6DF0724eb493E138F10DF08D03
#define _E34E3E6DF0724eb493E138F10DF08D03

#include <string>
#include <map>
#include "boost/utility.hpp"

#include "rbx/Debug.h"
#include "rbx/boost.hpp"
#include "rbx/atomic.h"  
#include <boost/unordered_map.hpp>
#include "rbx/threadsafe.h"

#include "security/ApiSecurity.h"

namespace RBX {

	class Name : public boost::noncopyable
	{
		static RBX::mutex& mutex();
		class NameMap;
		static NameMap& map();

		template<const char* const& sName>
		static const Name& doDeclare()
		{
			static const Name& n = declare(sName);
			return n;
		}
		template<const char* const& sName>
		static void callDoDeclare()
		{
			doDeclare<sName>();
		}

		// sortIndex is atomic to avoid any chance of stale
		// data when calling setOrderIndex
		rbx::atomic<int> sortIndex;

	public:
		std::string const str;	// the string that is the text name

		static size_t size();
		static size_t approximateMemoryUsage();

		// Declaration and Query
		static const Name& getNullName();

		// Fast and thread-safe
        NOINLINE static const Name& declare(const char* const& sName);
        FORCEINLINE static const Name& declare(const std::string& sName)
        {
            return declare(sName.c_str());
        }
		template<const char* const& sName>
		static const Name& declare()
		{
			if(sName == NULL)
				return getNullName();

			static boost::once_flag flag = BOOST_ONCE_INIT;
			boost::call_once(&callDoDeclare<sName>, flag);
			return doDeclare<sName>();
		}
		
        NOINLINE static const Name& lookup(const char* const& sName);
        FORCEINLINE static const Name& lookup(const std::string& sName)
        {
            return lookup(sName.c_str());
        }

		bool empty() const { return getNullName()==*this; }
		static bool empty(const Name* name) { return name==0 || *name==getNullName(); }

		// Convert to string
		const std::string& toString() const  { return str; }
		const char* c_str() const  { return str.c_str(); }
		
		// Comparison
#if 1
		// Optimization - avoids string comparisons
		static inline int compare(const Name& a, const Name& b) {
			return a.sortIndex - b.sortIndex;
		}
		inline int compare(const Name& other) const {
			return sortIndex - other.sortIndex;
		}
		inline bool operator < (const Name& other) const {
			return sortIndex < other.sortIndex;
		}
		inline bool operator > (const Name& other) const {
			return sortIndex > other.sortIndex;
		}
#else
		static inline int compare(const Name& a, const Name& b) {
			return a.str.compare(b.str);
		}
		inline int compare(const Name& other) const {
			return str.compare(other.str);
		}
		inline bool operator < (const Name& other) const {
			return str < other.str;
		}
		inline bool operator > (const Name& other) const {
			return str > other.str;
		}
#endif
		inline bool operator == (const Name& other) const {
			return this==&other;
		}
		inline bool operator != (const Name& other) const {
			return this!=&other;
		}
		inline bool operator == (const std::string& sName) const {
			return this->str == sName;
		}
		inline bool operator != (const std::string& sName) const {
			return this->str != sName;
		}
		inline bool operator == (const char* const &sName) const {
			return this->str == sName;
		}
		inline bool operator != (const char* const &sName) const {
			return this->str != sName;
		}

	private:
		explicit Name(const char* const &sName);
		void setOrderIndex();
	};

	std::ostream& operator<<(std::ostream& os, const RBX::Name& name);

	// An object that has a Name
	class RBXInterface INamed {
	public:
		virtual const Name& getName() const = 0;
	};

	// A template implementation of INamed
	template <class BaseClass, const char* const& sName>
	class Named : public BaseClass {
	public:

		// Constructors with different numbers of arguments
		Named() : BaseClass() {}
		template<typename Arg0>
		Named(Arg0 arg0) : BaseClass(arg0) {}
		template<typename Arg0, typename Arg1>
		Named(Arg0 arg0, Arg1 arg1) : BaseClass(arg0, arg1) {}
		template<typename Arg0, typename Arg1, typename Arg2>
		Named(Arg0 arg0, Arg1 arg1, Arg2 arg2) : BaseClass(arg0, arg1, arg2) {}
		template<typename Arg0, typename Arg1, typename Arg2, typename Arg3>
		Named(Arg0 arg0, Arg1 arg1, Arg2 arg2, Arg3 arg3) : BaseClass(arg0, arg1, arg2, arg3) {}

		static const Name& name() {
			return Name::declare<sName>();
		}
		virtual const Name& getName() const {
			return name();
		}
	};

}

#endif
