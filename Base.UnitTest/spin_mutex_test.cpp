#include <boost/test/unit_test.hpp>
#include "concurrent.h"

#include "rbx/threadsafe.h"

namespace spin_mutex_test
{
    BOOST_AUTO_TEST_SUITE(spin_mutex)
    
    struct ConcurrentAccess
    {
        ConcurrentAccess()
        {
            const int threads = 8;
            const int iterations = 1000000;
            rbx::spin_mutex mutex;
            int value = 0;
            int expected_value = 0;
            
            for (int i = 1; i <= threads; ++i)
                expected_value += i * iterations;
            
            BOOST_CHECK_EQUAL(util::ConcurrentLoops<Adder>::run(Adder(mutex, value), iterations, threads), 0);
            BOOST_CHECK_EQUAL(value, expected_value);
        }
        
    private:
        struct Adder
        {
            int operator()(int index)
            {
                rbx::spin_mutex::scoped_lock lock(mutex_);
                value_ += index + 1;
                return 0;
            }
            
            Adder(rbx::spin_mutex& mutex, int& value) : mutex_(mutex), value_(value) {}
            
        private:
            rbx::spin_mutex& mutex_;
            int& value_;
        };
    };
    
    BOOST_AUTO_TEST_CASE(concurrent_access)
    {
        ConcurrentAccess test;
    }
    
    BOOST_AUTO_TEST_SUITE_END() // spin_mutex
    
} // namespace spin_mutex_test