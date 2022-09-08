#include <boost/test/unit_test.hpp>

#include "boost/shared_ptr.hpp"
#include "boost/weak_ptr.hpp"

#include "rbx/intrusive_ptr_target.h"
#include "rbx/intrusive_weak_ptr.h"


namespace IntrusiveTest
{

	class CountedBase : boost::noncopyable
	{
	public:
		std::string foo;
		static int count;
		int id;
		CountedBase()
		{
			count++; 
			static int guid = 0;
			id = ++guid; 
		}
		~CountedBase()
		{
			foo = "bye base!";
			count--;
		}
	};
	int CountedBase::count = 0;

	class Counted 
		: public rbx::intrusive_ptr_target<Counted>
		, public CountedBase
	{
	  public:
		 Counted() {
		 }
		virtual ~Counted()
		{
			foo = "bye!";
		}

	};

	class CountedChild : public Counted
	{
	public:
		~CountedChild()
		{
			foo = "bye child!";
		}
	};


	class QuickCounted 
		: public rbx::quick_intrusive_ptr_target<QuickCounted>
		, public CountedBase
	{
	  public:
		 QuickCounted() {
		 }
		virtual ~QuickCounted()
		{
			foo = "bye!";
		}

	};

	class QuickCountedChild : public QuickCounted
	{
	public:
		~QuickCountedChild()
		{
			foo = "bye child!";
		}
	};


	class CountedFixture
	{
		int count;
	public:
		CountedFixture() { 
			count = CountedBase::count;
		}
		~CountedFixture() { 
			BOOST_CHECK_EQUAL(CountedBase::count, count);
		}
	};

	BOOST_FIXTURE_TEST_SUITE(intrusive_ptr, CountedFixture)

		BOOST_AUTO_TEST_CASE(Test)
		{
			boost::intrusive_ptr<Counted> p1 = new CountedChild();

			BOOST_CHECK_EQUAL(1, Counted::count);
			//BOOST_CHECK_EQUAL(1, p1.use_count());

			{
				boost::intrusive_ptr<Counted> p2 = p1;
				boost::intrusive_ptr<Counted> p3(p1);

				//BOOST_CHECK_EQUAL(3, p1.use_count());
				//BOOST_CHECK_EQUAL(3, p2.use_count());
				//BOOST_CHECK_EQUAL(3, p3.use_count());

				p3.reset();

				BOOST_CHECK_EQUAL(1, Counted::count);
				//BOOST_CHECK_EQUAL(2, p1.use_count());
				//BOOST_CHECK_EQUAL(2, p2.use_count());
				//BOOST_CHECK_EQUAL(0, p3.use_count());
			}
			//BOOST_CHECK_EQUAL(1, p1.use_count());
		}


		BOOST_AUTO_TEST_CASE(Operators)
		{
			boost::intrusive_ptr<Counted> p1 = new CountedChild();
			BOOST_CHECK_EQUAL(1, Counted::count);

			Counted* c1 = p1.get();
			BOOST_CHECK_EQUAL(1, Counted::count);

			Counted& c2 = *p1;
			BOOST_CHECK_EQUAL(1, Counted::count);

			BOOST_CHECK_EQUAL(c1, &c2);
			BOOST_CHECK_EQUAL(c2.id, p1->id);
			BOOST_CHECK_EQUAL(c2.id, (*p1).id);
		}

#ifdef _DEBUG
#define ccc 1
#else
#define ccc 10000000
#endif

		BOOST_AUTO_TEST_CASE(Perf_Intrusive)
		{
			boost::intrusive_ptr<Counted> p1 = new CountedChild();
			boost::intrusive_ptr<Counted> w;
			for (int i = 0; i < ccc; ++i)
			{
				boost::intrusive_ptr<Counted> w1 = p1;
				w = p1;
				w.reset();
			}
		}

		BOOST_AUTO_TEST_CASE(Perf_Quick)
		{
			boost::intrusive_ptr<QuickCounted> p1 = new QuickCountedChild();
			boost::intrusive_ptr<QuickCounted> w;
			for (int i = 0; i < ccc; ++i)
			{
				boost::intrusive_ptr<QuickCounted> w1 = p1;
				w = p1;
				w.reset();
			}
		}

		BOOST_AUTO_TEST_CASE(Perf_Shared)
		{
			boost::shared_ptr<Counted> p1(new CountedChild());
			boost::shared_ptr<Counted> w;
			for (int i = 0; i < ccc; ++i)
			{
				boost::shared_ptr<Counted> w1 = p1;
				w = p1;
				w.reset();
			}
		}

		BOOST_AUTO_TEST_CASE(PerfDelete_Intrusive)
		{
			for (int i = 0; i <= ccc/10; ++i)
			{
				boost::intrusive_ptr<Counted> p1 = new CountedChild();
			}
		}

		BOOST_AUTO_TEST_CASE(PerfDelete_Quick)
		{
			for (int i = 0; i <= ccc/10; ++i)
			{
				boost::intrusive_ptr<QuickCounted> p1 = new QuickCountedChild();
			}
		}

		BOOST_AUTO_TEST_CASE(PerfDelete_Shared)
		{
			for (int i = 0; i <= ccc/10; ++i)
			{
				boost::shared_ptr<Counted> p1(new CountedChild());
			}
		}


		BOOST_AUTO_TEST_CASE(PerfWeak_Intrusive)
		{
			boost::intrusive_ptr<Counted> p1 = new CountedChild();
			rbx::intrusive_weak_ptr<Counted> w;
			for (int i = 0; i < ccc; ++i)
			{
				rbx::intrusive_weak_ptr<Counted> w1 = p1;
				w = p1;
				w.reset();
			}
		}

		BOOST_AUTO_TEST_CASE(PerfWeak_Shared)
		{
			boost::shared_ptr<Counted> p1(new CountedChild());
			boost::weak_ptr<Counted> w;
			for (int i = 0; i < ccc; ++i)
			{
				boost::weak_ptr<Counted> w1 = p1;
				w = p1;
				w.reset();
			}
		}


		BOOST_AUTO_TEST_CASE(WeakTest)
		{
			boost::intrusive_ptr<CountedChild> p1 = new CountedChild();

			BOOST_CHECK_EQUAL(1, Counted::count);
			//BOOST_CHECK_EQUAL(1, p1.use_count());

			rbx::intrusive_weak_ptr<CountedChild> w1 = p1;

			//BOOST_CHECK_EQUAL(1, p1.use_count());

			rbx::intrusive_weak_ptr<CountedChild> w2 = w1;

			//BOOST_CHECK_EQUAL(1, p1.use_count());

			rbx::intrusive_weak_ptr<Counted> w4(w1);
			rbx::intrusive_weak_ptr<Counted> w5(p1);
			rbx::intrusive_weak_ptr<Counted> w6(p1.get());

			rbx::intrusive_weak_ptr<Counted> w;
			w = w1;
			w = p1;
			w = p1.get();

			boost::intrusive_ptr<Counted> p2 = w1.lock();
			//BOOST_CHECK_EQUAL(2, p1.use_count());

			BOOST_CHECK_EQUAL(1, Counted::count);

			{
				if (boost::intrusive_ptr<Counted> p3 = w1.lock())
					//BOOST_CHECK_EQUAL(3, p1.use_count());
					;
				else
					BOOST_CHECK(false);
			}

			p1.reset();

			BOOST_CHECK_NO_THROW(boost::intrusive_ptr<Counted>(w1.lock()).get());

			p2.reset();

			BOOST_CHECK_EQUAL(0, Counted::count);
			//BOOST_CHECK_EQUAL(0, p1.use_count());

			p2 = w1.lock();
			BOOST_CHECK(!p2);

			//BOOST_CHECK_THROW(boost::intrusive_ptr<Counted>(w1.lock()).get(), rbx::bad_weak_ptr);
		}
#ifndef RBX_PLATFORM_IOS

		class OF
			: public rbx::intrusive_ptr_target<OF, int, 1, 2>
		{
		};

		BOOST_AUTO_TEST_CASE(Overflow)
		{
			boost::intrusive_ptr<OF> of(new OF());
			boost::intrusive_ptr<OF> of2;
			BOOST_CHECK_THROW(of2 = of, rbx::too_many_refs);
			BOOST_CHECK_THROW(of2 = of, std::exception);

			rbx::intrusive_weak_ptr<OF> w[10];
			w[0] = of;
			w[1] = w[0];
			BOOST_CHECK_THROW(w[2] = w[0], rbx::too_many_refs);
			BOOST_CHECK_THROW(w[2] = w[0], std::exception);
		}
#endif

	BOOST_AUTO_TEST_SUITE_END()

}
