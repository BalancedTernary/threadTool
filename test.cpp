#include <iostream>
#include "ThreadPool.h"
#include "_ThreadUnit.h"
#include "Scheduler.h"
#include "Async.h"
#include "MessageLimiter.h"
#include <cstdlib>
#include <string>
#include <thread>
using namespace threadTool;


int main()
{
    {
        ThreadPool tp;
        std::cout << "Hello World!\n";
        MessageLimiter ml;
        int times = 0;
        ml.regist(std::function<void(const int& n)>([&times](const int& n) {std::cout << "callback: " << n<<", times: "<<++times << std::endl; }));
        int n = 0;
        Scheduler scheduler(tp);
        auto x=scheduler->addInterval([&n, &ml]() {std::cout << "sendMessage: " << std::endl; ml.sendMessage(++n); }, std::chrono::milliseconds(90));
        std::this_thread::sleep_for(std::chrono::seconds(10));
        x.deleteUnit();
        MessageLimiter ml2(false, MessageLimiter::DISPERSE, std::chrono::milliseconds(100), std::function<bool(const bool&, const bool&)>([](const bool& newV, const bool& oldV) {return(newV == true && oldV == false); }));
        ml2.regist(std::function<void(const bool& n)>([&times](const bool& n) {std::cout << "callback: " << n << ", times: " << ++times << std::endl; }));
        x = scheduler->addInterval([&n, &ml2]() {std::cout << "sendMessage: " <<n<< std::endl; ml2.sendMessage(!bool((++n)%2)); }, std::chrono::milliseconds(80));
        std::this_thread::sleep_for(std::chrono::seconds(10));
        x.deleteUnit();
        x.join();

        //std::this_thread::sleep_for(std::chrono::seconds(10));
    }
    /*volatile std::atomic<int_least64_t> waitFlag;
    std::condition_variable BlockingQueue;
    std::mutex _mCondition;//条件变量锁*/
    {
        Mutex m{true};
        unique_readLock s4(m);
        unique_readLock s5(m);
        unique_writeLock s1(m);
        unique_readLock s2(m);
        unique_writeLock s3(m);
    }
    {
        Mutex m{ true };
        m.lock_write();
        m.lock_read();
        m.unlock_write();
        std::thread t1([&m]() {
            m.lock_read();
            std::cout << "m.lock_read()" << std::endl;
            std::cout << std::flush;
            m.unlock_read();
            /*m.lock_write();
            std::cout << "m.lock_write()" << std::endl;
            std::cout << std::flush;*/
            });
        
        t1.join();
        m.unlock_read();
    }


    std::cout << "Hello World!\n";
    {
        ThreadPool tp2;
        tp2.setIdleLife(std::chrono::seconds(1));
        //tp2.setMaximumNumberOfThreads(8);
        //tp2.setMinimumNumberOfThreads(4);
        //tp2.setRedundancyRatio(1.5);
        tp2.add([](AtomicConstReference<bool> tag) {do { std::cout << "aaa1" << std::endl << std::flush; } while (tag); });
        tp2.add([](AtomicConstReference<bool> tag) {do { std::cout << "aaa2" << std::endl << std::flush; } while (tag); });
        tp2.add([](AtomicConstReference<bool> tag) {do { std::cout << "aaa3" << std::endl << std::flush; } while (tag); });
        tp2.add([](AtomicConstReference<bool> tag) {do { std::cout << "aaa4" << std::endl << std::flush; } while (tag); });
        tp2.add([](AtomicConstReference<bool> tag) {do { std::cout << "aaa5" << std::endl << std::flush; } while (tag); });
        //std::this_thread::sleep_for(std::chrono::seconds(1));

    }

    

    //std::this_thread::sleep_for(std::chrono::seconds(200));

    {
        std::mutex _mCondition;
        std::condition_variable BlockingQueue;
        std::unique_lock<std::mutex> m(_mCondition);
        ThreadPool tp;
        tp.setMaximumNumberOfThreads(4);
        Scheduler scheduler(tp);
        scheduler->addTimeOutFor([]() {std::cout << "ccc" << std::endl << std::flush; }, std::chrono::seconds(10));
        auto s = scheduler->addInterval([]() {std::cout << "bbb" << std::endl << std::flush; }, std::chrono::seconds(1));
        auto te = Async<std::string>(tp, [](void)
            {
                std::this_thread::sleep_for(std::chrono::seconds(20));
                return "AsyncEnd";
            });
        std::cout << "AsyncStart" << std::endl << std::flush;
        std::cout << te.get() << std::endl << std::flush;
        auto s2 = scheduler->addInterval([]() {std::cout << "ddd" << std::endl << std::flush; }, std::chrono::milliseconds(500));
        auto s3 = scheduler->addInterval([]() {std::cout << "eee" << std::endl << std::flush; }, std::chrono::milliseconds(100));
        s.deleteUnit();
        BlockingQueue.wait_for(m, std::chrono::seconds(10));
        s2.deleteUnit();
        BlockingQueue.wait_for(m, std::chrono::seconds(10));
        s3.deleteUnit();
        //BlockingQueue.wait_for(m, std::chrono::seconds(10));
        auto s4 = scheduler->addInterval([]() {std::cout << "fff" << std::endl << std::flush; }, std::chrono::milliseconds(500));
        //BlockingQueue.wait_for(m, std::chrono::seconds(10));

    }


    ThreadPool tp;
    tp.setIdleLife(std::chrono::seconds(1));
    tp.setMaximumNumberOfThreads(200);
    tp.setMinimumNumberOfThreads(5);
    tp.setRedundancyRatio(1.5);
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
    //std::unique_lock<std::mutex> m(_mCondition);
    //BlockingQueue.wait(m, [&waitFlag]() {return waitFlag <= 0; });
    
    std::this_thread::sleep_for(std::chrono::seconds(60));

}