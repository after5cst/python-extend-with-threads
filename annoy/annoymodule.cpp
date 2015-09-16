#include <Python.h>
#include "annoy.h"

#include <atomic>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <thread>
#include <vector>

#define PRINT_DEBUG_MESSAGES

namespace {

    struct thread_parms{
        std::atomic<bool> quit;
    };

    typedef std::shared_ptr<thread_parms> thread_parms_ptr;
    typedef std::pair<std::thread, thread_parms_ptr> thread_data;
    std::vector<thread_data> threads;
}

void destroy_globals()
{
#ifdef PRINT_DEBUG_MESSAGES
    std::cout << "annoy::destroy_globals called" << std::endl;
#endif
    annoy(0);
}

void init_globals()
{
    if (0 != atexit(destroy_globals))
    {
        std::cerr << "annoy::destroy_globals not installed" << std::endl;
        exit(-1);
    }

#ifdef PRINT_DEBUG_MESSAGES
    std::cout << "annoy::init_globals called" << std::endl;
#endif
}

#undef PRINT_DEBUG_MESSAGES

void worker(thread_parms_ptr parms)
{
    auto counter = 0;
    while(false == (parms->quit))
    {
        if(0 == (counter++ % 100))
        {
            std::stringstream sstr;
            sstr << "print('Thread " << std::this_thread::get_id()
                 << " counter = " << counter << "')";
            auto state = PyGILState_Ensure();
            PyRun_SimpleString(sstr.str().c_str());
            PyGILState_Release(state);
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void annoy(int count)
{
    count = std::max(count, 0);

    if (!PyEval_ThreadsInitialized())
    {
        PyEval_InitThreads();
    }

#ifdef PRINT_DEBUG_MESSAGES
    std::cout << "checking thread pool for removal" << std::endl;
#endif
    while(count < threads.size())
    {
#ifdef PRINT_DEBUG_MESSAGES
        std::cout << "stopping thread" << std::endl;
#endif
        auto& back = threads.back();
        back.second->quit = true;
        back.first.join();
#ifdef PRINT_DEBUG_MESSAGES
        std::cout << "removing thread" << std::endl;
#endif
        threads.pop_back();
    }

#ifdef PRINT_DEBUG_MESSAGES
    std::cout << "checking thread pool for additions" << std::endl;
#endif
    while(count > threads.size())
    {
#ifdef PRINT_DEBUG_MESSAGES
        std::cout << "creating thread" << std::endl;
#endif
        threads.push_back(thread_data{});
#ifdef PRINT_DEBUG_MESSAGES
        std::cout << "setting thread" << std::endl;
#endif
        auto& back = threads.back();
        back.second = std::make_shared<thread_parms>();
        back.second->quit = false;
#ifdef PRINT_DEBUG_MESSAGES
        std::cout << "launching thread" << std::endl;
#endif
        back.first = std::thread(worker, back.second);
    }
}
