#include <Python.h>
#include "annoy.h"

#include <atomic>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>

namespace {

    struct thread_parms{
        std::atomic<bool> quit;
    };

    typedef std::shared_ptr<thread_parms> thread_parms_ptr;
    typedef std::pair<std::thread, thread_parms_ptr> thread_data;
    std::vector<thread_data> *threads;
}

void init_globals()
{
    threads = new std::vector<thread_data>();
    std::cout << "threads object created" << std::endl;
}

void worker(thread_parms_ptr parms)
{
    auto counter = 0;
    while(false == (parms->quit))
    {
        if(counter++ >= 100)
        {
            std::stringstream sstr;
            sstr << "print('Thread " << std::this_thread::get_id()
                 << " counter = " << counter << "')";
            auto state = PyGILState_Ensure();
            PyRun_SimpleString(sstr.str().c_str());
            PyGILState_Release(state);
            counter = 0;
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

    std::cout << "checking thread pool for removal" << std::endl;
    while(count < threads->size())
    {
        std::cout << "stopping thread" << std::endl;
        auto& back = threads->back();
        back.second->quit = true;
        back.first.join();
        std::cout << "removing thread" << std::endl;
        threads->pop_back();
    }

    std::cout << "checking thread pool for additions" << std::endl;
    while(count > threads->size())
    {
        std::cout << "creating thread" << std::endl;
        threads->push_back(thread_data{});
        std::cout << "setting thread" << std::endl;
        auto& back = threads->back();
        back.second = std::make_shared<thread_parms>();
        back.second->quit = false;
        std::cout << "launching thread" << std::endl;
        back.first = std::thread(worker, back.second);
    }
}
