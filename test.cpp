#include <iostream>
#include "ThreadPool.h"
#include "_ThreadUnit.h"
#include "Scheduler.h"
#include "Async.h"

#include <cstdlib>
#include <string>
using namespace threadTool;
int main()
{
    /*volatile std::atomic<int_least64_t> waitFlag;
    std::condition_variable BlockingQueue;
    std::mutex _mCondition;//条件变量锁*/
    std::cout << "Hello World!\n";
    {
        ThreadPool tp2;
        tp2.setIdleLife(std::chrono::seconds(1));
        //tp2.setMaximumNumberOfThreads(8);
        //tp2.setMinimumNumberOfThreads(4);
        //tp2.setRedundancyRatio(1.5);
        tp2.add([](const volatile std::atomic<volatile bool>& tag) {while (tag)std::cout<<"aaa" << std::endl << std::flush; });
    }

    ThreadPool tp;
    tp.setIdleLife(std::chrono::seconds(1));
    tp.setMaximumNumberOfThreads(200);
    tp.setMinimumNumberOfThreads(5);
    tp.setRedundancyRatio(1.5);
    

    //std::this_thread::sleep_for(std::chrono::seconds(200));

    {
        std::mutex _mCondition;
        std::condition_variable BlockingQueue;
        std::unique_lock<std::mutex> m(_mCondition);
        ThreadPool tp;
        tp.setMaximumNumberOfThreads(4);
        Scheduler scheduler(tp);
        scheduler.addTimeOutFor([]() {std::cout << "ccc" << std::endl << std::flush; }, std::chrono::seconds(10));
        auto s = scheduler.addInterval([]() {std::cout << "bbb" << std::endl << std::flush; }, std::chrono::seconds(1));
        auto te = Async<std::string>(tp, [](void)
            {
                std::this_thread::sleep_for(std::chrono::seconds(20));
                return "AsyncEnd";
            });
        std::cout << "AsyncStart" << std::endl << std::flush;
        std::cout << te.get() << std::endl << std::flush;
        auto s2 = scheduler.addInterval([]() {std::cout << "ddd" << std::endl << std::flush; }, std::chrono::milliseconds(500));
        auto s3 = scheduler.addInterval([]() {std::cout << "eee" << std::endl << std::flush; }, std::chrono::milliseconds(500));
        s.deleteUnit();
        BlockingQueue.wait_for(m, std::chrono::seconds(10));
        s2.deleteUnit();
        BlockingQueue.wait_for(m, std::chrono::seconds(10));
        
    }
    long long t = 1000;
    while (t-- > 0)
    {
        //++waitFlag;
        tp.add([&tp, t/*, &waitFlag, &_mCondition, &BlockingQueue*/](const bool& tag)
            {
                long long y = rand() * 50000ull;
                while ((--y) * 10);
                if (t % 1 == 0)
                {
                    std::cout << (std::ostringstream("") <<tag<< y << t << ": " << tp.getNumberOfThreads() << " : " << tp.getNumberOfIdles() << "\n").str() << std::endl << std::flush;

                }
                /*std::unique_lock<std::mutex> m(_mCondition);
                --waitFlag;
                BlockingQueue.notify_one();*/
            });
        //std::this_thread::sleep_for(std::chrono::microseconds(rand()));
    }
    tp.join();
    //std::this_thread::sleep_for(std::chrono::seconds(1000));
    //std::unique_lock<std::mutex> m(_mCondition);
    //BlockingQueue.wait(m, [&waitFlag]() {return waitFlag <= 0; });
    std::cout << "Hello World!\n";
}