#include <boost/test/unit_test.hpp>
#include "concurrent.h"

#include "rbx/atomic.h"
#include "rbx/boost.hpp"

namespace AtomicTest
{

	BOOST_AUTO_TEST_SUITE(Atomic)

		template<class V, class UV>
		static void test()
		{
#ifndef RBX_PLATFORM_IOS
			{
				rbx::atomic<V> v;
				v = 0;
				BOOST_CHECK_EQUAL(++v, (V)1);
				BOOST_CHECK_EQUAL(--v, (V)0);
				BOOST_CHECK_EQUAL(--v, -(V)1);
				BOOST_CHECK_EQUAL(++v, (V)0);
/* // we dont test unsigned until further notice
				rbx::atomic<UV> uv;
				uv = 0;
				BOOST_CHECK_EQUAL(--uv, (UV)~0);
				BOOST_CHECK_EQUAL(++uv, (UV)0);
*/
			}
#endif

			{
				rbx::atomic<V> v;
				BOOST_CHECK_EQUAL(++v, (V)1);
				BOOST_CHECK_EQUAL(--v, (V)0);
				BOOST_CHECK_EQUAL(--v, -(V)1);
				BOOST_CHECK_EQUAL(++v, (V)0);

#ifndef RBX_PLATFORM_IOS
/*              // we dont test unsigned until further notice
				rbx::atomic<UV> uv;
				BOOST_CHECK_EQUAL(--uv, (UV)~0);
				BOOST_CHECK_EQUAL(++uv, (UV)0);
*/
#endif
			}
		}

		BOOST_AUTO_TEST_CASE(Int)
		{
			test<int, unsigned int>();
		}
    
        BOOST_AUTO_TEST_CASE(Long)
        {
            test<long, unsigned long>();
        }

        // may need longlong test when we finally implement it

        template<typename T>
        struct TestIncrement
        {
            TestIncrement()
            {
                const int threads = 8;
                const int iterations = 1000000;
                rbx::atomic<T> state;
                
                util::ConcurrentLoops<Incrementer>::run(Incrementer(state), iterations, threads);
                BOOST_CHECK_EQUAL(state, threads * iterations);
                
                util::ConcurrentLoops<Decrementer>::run(Decrementer(state), iterations / 2, threads);
                BOOST_CHECK_EQUAL(state, threads * iterations / 2);
                
                util::ConcurrentLoops<Decrementer>::run(Decrementer(state), iterations / 2, threads);
                BOOST_CHECK_EQUAL(state, 0);
            }
            
        private:
            struct Incrementer
            {
                int operator()(int)
                {
                    ++state_;
                    return 0;
                }
                
                Incrementer(rbx::atomic<T> &state) : state_(state) {}
                
            private:
                rbx::atomic<T> &state_;
            };
            
            struct Decrementer
            {
                int operator()(int)
                {
                    --state_;
                    return 0;
                }
                
                Decrementer(rbx::atomic<T> &state) : state_(state) {}
                
            private:
                rbx::atomic<T> &state_;
            };
        };

        template <typename T>
        struct TestCASFalsePositive
        {
            TestCASFalsePositive()
            {
                const int threads = 8;
                const int iterations = 1000000;
                rbx::atomic<T> state;
                
                BOOST_CHECK_EQUAL(state, 0);
                BOOST_CHECK_EQUAL(util::ConcurrentLoops<StateGrabber>::run(StateGrabber(state), iterations, threads), 0);
                BOOST_CHECK_EQUAL(state, 0);
            }
            
        private:
            struct StateGrabber
            {
                int operator()(int index)
                {
                    const T target = index + 1;
                    if (0 == state_.compare_and_swap(target, 0))
                    {
                        if (state_.compare_and_swap(0, target) != target)
                            return 1;
                    }
                    return 0;
                }
                
                StateGrabber(rbx::atomic<T> &state) : state_(state) {}

            private:
                rbx::atomic<T> &state_;
            };
        };

        template <typename T, int NThreads, int NIterations>
        struct TestSwap
        {
            TestSwap()
            {
                rbx::atomic<T> histogram[NThreads + 1];
                rbx::atomic<T> state;
                
                BOOST_CHECK_EQUAL(state, 0);
                for (auto &h : histogram) // curly braces are important here due to compiler bug in visual studio < 2015
                {
                    BOOST_CHECK_EQUAL(h, 0);
                }

                ++histogram[0];
                BOOST_CHECK_EQUAL(histogram[0], 1);
                
                BOOST_CHECK_EQUAL(util::ConcurrentLoops<StateSwapper>::run(StateSwapper(state, histogram), NIterations, NThreads), 0);
                
                BOOST_CHECK_LE(state, NThreads);
                
                --histogram[state];
                
                for (auto &h : histogram) // curly braces are important here due to compiler bug in visual studio < 2015
                {
                    BOOST_CHECK_EQUAL(h, 0);
                }
            }
            
        private:
            struct StateSwapper
            {
                int operator()(int index)
                {
                    const T target = index + 1;
                    const T old = state_.swap(target);
                    
                    if (old > NThreads)
                        return 1;
                    
                    ++histogram_[target];
                    --histogram_[old];
                    
                    return 0;
                }
                
                StateSwapper(rbx::atomic<T> &state, rbx::atomic<T> *histogram)
                    : state_(state)
                    , histogram_(histogram) {}
                
            private:
                rbx::atomic<T> &state_;
                rbx::atomic<T> *histogram_;
            };
        };
    
        // The tests below are independent tests -- only atomicity relative to
        // the same kind of operation performed in another threads is tested.

        // Currenly there are no tests for cas vs. inc/dec vs. swap() atomicity.
        // TODO

        BOOST_AUTO_TEST_CASE(increment)
        {
            TestIncrement<int> test_int;
            TestIncrement<long> test_long;
        }

        BOOST_AUTO_TEST_CASE(cas_false_positive)
        {
            TestCASFalsePositive<int> test_int;
            TestCASFalsePositive<long> test_long;
        }
    
        BOOST_AUTO_TEST_CASE(swap)
        {
            TestSwap<int, 8, 1000000> test_int;
            TestSwap<long, 8, 1000000> test_long;
        }

	BOOST_AUTO_TEST_SUITE_END() // Atomic

}
