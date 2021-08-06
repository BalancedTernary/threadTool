#include <iostream>
#include "ThreadPool.h"
#include "_ThreadUnit.h"
#include <cstdlib>
#include <string>
int main()
{
    /*volatile std::atomic<int_least64_t> waitFlag;
    std::condition_variable BlockingQueue;
    std::mutex _mCondition;//条件变量锁*/
    std::cout << "Hello World!\n";
    ThreadPool tp;
    tp.setIdleLife(std::chrono::seconds(1));
    tp.setMaximumNumberOfThreads(200);
    tp.setMinimumNumberOfThreads(5);
    tp.setRedundancyRatio(1.5);
        

    long long t = 1000;
    while (t-- > 0)
    {
        //++waitFlag;
        tp.add([&tp, t/*, &waitFlag, &_mCondition, &BlockingQueue*/]()
            {
                long long y = rand() * 50000;
                while ((--y) * 10);
                if (t % 1 == 0)
                {
                    std::cout << (std::ostringstream("") << y << t << ": " << tp.getNumberOfThreads() << " : " << tp.getNumberOfIdles() << "\n").str() << std::endl << std::flush;

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