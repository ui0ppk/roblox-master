
#ifndef _5F442CC8279E4ab292B4F86DBDCF1B27
#define _5F442CC8279E4ab292B4F86DBDCF1B27

#include <functional>
#include <algorithm>
#include <string>
#include "rbx/boost.hpp"
#include "rbx/Debug.h"

namespace RBX
{
	// generic encryption functions
	std::string sha1(const std::string& source);
	std::string rot13(std::string source);

	bool isCamel(const char* name);

	template <typename Type>
	class StringConverter
	{
	public:
		static std::string convertToString(const Type& value);
		static bool convertToValue(const std::string& text, Type& value);
	};

	// Note: not thread-safe
	template<class Class>
	class copy_on_write_ptr : public boost::noncopyable
	{
		mutable boost::shared_ptr<Class> object;
	public:
		// Default constructor creates an empty pointer
		copy_on_write_ptr() 
		{}
		
		// Initializes with a copy of the provided data
		copy_on_write_ptr(const Class& object)
			:object(new Class(object))
		{}

        // NA: 8/3/2013 Added this to make boost 1.5+ compile correctly for iOS
		// "bool" operator to see if it is initialized
        #if !defined( BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS ) && !defined( BOOST_NO_CXX11_NULLPTR )
        explicit operator bool () const BOOST_NOEXCEPT
        {
            return static_cast<bool>(object);
        }
        #else
		operator typename boost::shared_ptr<Class>::unspecified_bool_type() const
		{
			return object;
		}
        #endif
        
		// For quick-accessing a value (temporary. Do not keep reference)
		const Class& operator*() const
		{
			RBXASSERT(object.get() != 0);
			return *object;
		}
		// For quick-accessing a value (temporary. Do not keep reference)
		const Class* operator->() const
		{
			RBXASSERT(object.get() != 0);
			return object.get();
		}
		// For accessing an imutable value (as during an algorithm)
		// May return an empty pointer
		boost::shared_ptr<const Class> read() const
		{
			return object;
		}

		// NOTE: Don't hold on to this too long. It might change on you
		boost::shared_ptr<Class>& write()
		{
			if (!object)
			{
				// create a new object to assign to
				object = boost::shared_ptr<Class>(new Class());
			}
			else if (object.use_count()>1)
			{
				// make a copy
				object = boost::shared_ptr<Class>(new Class(*object));
			}
			return object;
		}
		void reset()
		{
			object.reset();
		}
	};



};



#endif
