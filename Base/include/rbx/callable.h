
#include "boost/type_traits/function_traits.hpp"

namespace rbx
{
	// icallable and callable are a lightweight wrapper similar to boost::function,
	// but with more efficient storage. Because the instances are special-cased for
	// the functor involved you can't treat callable generically - it has to be
	// used in the context of template code that knows what to do with it.

	// See rbx::signals for an implementation that uses this in lieu of boost::function

	template<int arity, typename Signature>
	class icallable;

	template<typename Signature>
	class icallable<0, Signature>
	{
	public:
		virtual void call() = 0;
	};

	template<typename Signature>
	class icallable<1, Signature>
	{
	public:
		virtual void call(typename boost::function_traits<Signature>::arg1_type arg1) = 0;
	};

	template<typename Signature>
	class icallable<2, Signature>
	{
	public:
		virtual void call(typename boost::function_traits<Signature>::arg1_type arg1,
			typename boost::function_traits<Signature>::arg2_type arg2) = 0;
	};

	template<typename Signature>
	class icallable<3, Signature>
	{
	public:
		virtual void call(typename boost::function_traits<Signature>::arg1_type arg1,
			typename boost::function_traits<Signature>::arg2_type arg2,
			typename boost::function_traits<Signature>::arg3_type arg3) = 0;
	};

	template<typename Signature>
	class icallable<4, Signature>
	{
	public:
		virtual void call(typename boost::function_traits<Signature>::arg1_type arg1,
			typename boost::function_traits<Signature>::arg2_type arg2,
			typename boost::function_traits<Signature>::arg3_type arg3,
			typename boost::function_traits<Signature>::arg4_type arg4) = 0;
	};
    
    template<typename Signature>
	class icallable<5, Signature>
	{
	public:
		virtual void call(typename boost::function_traits<Signature>::arg1_type arg1,
                          typename boost::function_traits<Signature>::arg2_type arg2,
                          typename boost::function_traits<Signature>::arg3_type arg3,
                          typename boost::function_traits<Signature>::arg4_type arg4,
                          typename boost::function_traits<Signature>::arg5_type arg5) = 0;
	};

	template<typename Signature>
	class icallable<6, Signature>
	{
	public:
		virtual void call(typename boost::function_traits<Signature>::arg1_type arg1,
			typename boost::function_traits<Signature>::arg2_type arg2,
			typename boost::function_traits<Signature>::arg3_type arg3,
			typename boost::function_traits<Signature>::arg4_type arg4,
			typename boost::function_traits<Signature>::arg5_type arg5,
			typename boost::function_traits<Signature>::arg6_type arg6) = 0;
	};

	template<typename Signature>
	class icallable<7, Signature>
	{
	public:
		virtual void call(typename boost::function_traits<Signature>::arg1_type arg1,
			typename boost::function_traits<Signature>::arg2_type arg2,
			typename boost::function_traits<Signature>::arg3_type arg3,
			typename boost::function_traits<Signature>::arg4_type arg4,
			typename boost::function_traits<Signature>::arg5_type arg5,
			typename boost::function_traits<Signature>::arg6_type arg6,
			typename boost::function_traits<Signature>::arg7_type arg7) = 0;
	};

	template<class Base, class Delegate, int arity, typename Signature>
	class callable;

	template<class Base, class Delegate, typename Signature>
	class callable<Base, Delegate, 0, Signature> : public Base
	{
		Delegate deleg;
	public:
		template<typename Arg1>
		callable(const Delegate& d, Arg1 arg1)
			:Base(arg1)
			,deleg(d) 
		{}
		virtual void call() { deleg(); }
	};

	template<class Base, class Delegate, typename Signature>
	class callable<Base, Delegate, 1, Signature> : public Base
	{
		Delegate deleg;
	public:
		template<typename Arg1>
		callable(const Delegate& deleg, Arg1 arg1):Base(arg1),deleg(deleg) {}
		virtual void call(typename boost::function_traits<Signature>::arg1_type arg1) { deleg(arg1); }
	};
		
	template<class Base, class Delegate, typename Signature>
	class callable<Base, Delegate, 2, Signature> : public Base
	{
		Delegate deleg;
	public:
		template<typename Arg1>
		callable(const Delegate& deleg, Arg1 arg1):Base(arg1),deleg(deleg) {}
		virtual void call(
			typename boost::function_traits<Signature>::arg1_type arg1,
			typename boost::function_traits<Signature>::arg2_type arg2
			) { deleg(arg1, arg2); }
	};
	
	template<class Base, class Delegate, typename Signature>
	class callable<Base, Delegate, 3, Signature> : public Base
	{
		Delegate deleg;
	public:
		template<typename Arg1>
		callable(const Delegate& deleg, Arg1 arg1):Base(arg1),deleg(deleg) {}
		virtual void call(
			typename boost::function_traits<Signature>::arg1_type arg1,
			typename boost::function_traits<Signature>::arg2_type arg2,
			typename boost::function_traits<Signature>::arg3_type arg3
			) { deleg(arg1, arg2, arg3); }
	};
	
	template<class Base, class Delegate, typename Signature>
	class callable<Base, Delegate, 4, Signature> : public Base
	{
		Delegate deleg;
	public:
		template<typename Arg1>
		callable(const Delegate& deleg, Arg1 arg1):Base(arg1),deleg(deleg) {}
		virtual void call(
			typename boost::function_traits<Signature>::arg1_type arg1,
			typename boost::function_traits<Signature>::arg2_type arg2,
			typename boost::function_traits<Signature>::arg3_type arg3,
			typename boost::function_traits<Signature>::arg4_type arg4
			) { deleg(arg1, arg2, arg3, arg4); }
	};
    
    template<class Base, class Delegate, typename Signature>
	class callable<Base, Delegate, 5, Signature> : public Base
	{
		Delegate deleg;
	public:
		template<typename Arg1>
		callable(const Delegate& deleg, Arg1 arg1):Base(arg1),deleg(deleg) {}
		virtual void call(
                          typename boost::function_traits<Signature>::arg1_type arg1,
                          typename boost::function_traits<Signature>::arg2_type arg2,
                          typename boost::function_traits<Signature>::arg3_type arg3,
                          typename boost::function_traits<Signature>::arg4_type arg4,
                          typename boost::function_traits<Signature>::arg5_type arg5
                          ) { deleg(arg1, arg2, arg3, arg4, arg5); }
	};

	template<class Base, class Delegate, typename Signature>
	class callable<Base, Delegate, 6, Signature> : public Base
	{
		Delegate deleg;
	public:
		template<typename Arg1>
		callable(const Delegate& deleg, Arg1 arg1):Base(arg1),deleg(deleg) {}
		virtual void call(
			typename boost::function_traits<Signature>::arg1_type arg1,
			typename boost::function_traits<Signature>::arg2_type arg2,
			typename boost::function_traits<Signature>::arg3_type arg3,
			typename boost::function_traits<Signature>::arg4_type arg4,
			typename boost::function_traits<Signature>::arg5_type arg5,
			typename boost::function_traits<Signature>::arg6_type arg6
			) { deleg(arg1, arg2, arg3, arg4, arg5, arg6); }
	};

	template<class Base, class Delegate, typename Signature>
	class callable<Base, Delegate, 7, Signature> : public Base
	{
		Delegate deleg;
	public:
		template<typename Arg1>
		callable(const Delegate& deleg, Arg1 arg1):Base(arg1),deleg(deleg) {}
		virtual void call(
			typename boost::function_traits<Signature>::arg1_type arg1,
			typename boost::function_traits<Signature>::arg2_type arg2,
			typename boost::function_traits<Signature>::arg3_type arg3,
			typename boost::function_traits<Signature>::arg4_type arg4,
			typename boost::function_traits<Signature>::arg5_type arg5,
			typename boost::function_traits<Signature>::arg6_type arg6,
			typename boost::function_traits<Signature>::arg7_type arg7
			) { deleg(arg1, arg2, arg3, arg4, arg5, arg6, arg7); }
	};
	
}