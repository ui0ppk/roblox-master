#pragma once

#include <boost/thread.hpp>

namespace util
{
    template <typename F>
    struct ConcurrentLoops
    {
        static int run(F f, int iterations, int num_threads)
        {
            int result = 0;
            std::vector<std::pair<boost::thread*, int> > threads(num_threads);
            int index = 0;
            for (auto &it : threads)
            {
                it.first = new boost::thread(threadFunc, f, index++, iterations, it.second);
            }
            
            for (auto &it : threads)
            {
                it.first->join();
                delete it.first;
                it.first = 0;
                if (it.second)
                    result = it.second;
            }
            return result;
        }
        
    private:
        static void threadFunc(F &f, int index, int iterations, int &result)
        {
            for (int i = 0; i < iterations; ++i)
            {
                result = f(index);
                if (result)
                    break;
            }
        }
    }; // struct ConcurrentLoops
    
} // namespace utils