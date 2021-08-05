#include <iostream>
#include "ThreadPool.h"
#include "ThreadUnit.h"
#include <cstdlib>
#include <string>
#include <iostream>
int main()
{
    std::cout << "Hello World!\n";
    //ThreadUnit tu;
    ThreadPool tp;
    tp.setIdleLife(std::chrono::seconds(1));
    tp.setMaximumNumberOfThreads(200);
    tp.setMinimumNumberOfThreads(5);
    tp.setRedundancyRatio(1.5);
    long long t = 1000000;
    while (t-- > 0)
    {
        tp.add([&tp, t]() { long long y = rand()* 50000; while ((--y)*10); if(t%10==0)std::cout << (std::ostringstream("") <<y<< t << ": " << tp.getNumberOfThreads() << " : " << tp.getNumberOfIdle() << "\n").str() << std::endl << std::flush; });
        std::this_thread::sleep_for(std::chrono::microseconds(rand()));
    }
    
    /*tp.add([&tp]() {std::this_thread::sleep_for(std::chrono::microseconds(rand())); std::cout << "1:"<<tp.getNumberOfThreads()<<"\n" << std::flush; });
    tp.add([&tp]() {std::this_thread::sleep_for(std::chrono::microseconds(rand())); std::cout << "2:" << tp.getNumberOfThreads() << "\n" << std::flush; });
    tp.add([&tp]() {std::this_thread::sleep_for(std::chrono::microseconds(rand())); std::cout << "3:" << tp.getNumberOfThreads() << "\n" << std::flush; });
    tp.add([&tp]() {std::this_thread::sleep_for(std::chrono::microseconds(rand())); std::cout << "4:" << tp.getNumberOfThreads() << "\n" << std::flush; });
    tp.add([&tp]() {std::this_thread::sleep_for(std::chrono::microseconds(rand())); std::cout << "5:" << tp.getNumberOfThreads() << "\n" << std::flush; });
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    tp.add([&tp]() {std::this_thread::sleep_for(std::chrono::microseconds(rand())); std::cout << "6:" << tp.getNumberOfThreads() << "\n" << std::flush; });
    tp.add([&tp]() {std::this_thread::sleep_for(std::chrono::microseconds(rand())); std::cout << "7:" << tp.getNumberOfThreads() << "\n" << std::flush; });
    tp.add([&tp]() {std::this_thread::sleep_for(std::chrono::microseconds(rand())); std::cout << "8:" << tp.getNumberOfThreads() << "\n" << std::flush; });
    tp.add([&tp]() {std::this_thread::sleep_for(std::chrono::microseconds(rand())); std::cout << "9:" << tp.getNumberOfThreads() << "\n" << std::flush; });
    tp.add([&tp]() {std::this_thread::sleep_for(std::chrono::microseconds(rand())); std::cout << "10:" << tp.getNumberOfThreads() << "\n" << std::flush; });
    tp.add([&tp]() {std::this_thread::sleep_for(std::chrono::microseconds(rand())); std::cout << "11:" << tp.getNumberOfThreads() << "\n" << std::flush; });
    tp.add([&tp]() {std::this_thread::sleep_for(std::chrono::microseconds(rand())); std::cout << "12:" << tp.getNumberOfThreads() << "\n" << std::flush; });
    tp.add([&tp]() {std::this_thread::sleep_for(std::chrono::microseconds(rand())); std::cout << "13:" << tp.getNumberOfThreads() << "\n" << std::flush; });
    tp.add([&tp]() {std::this_thread::sleep_for(std::chrono::microseconds(rand())); std::cout << "14:" << tp.getNumberOfThreads() << "\n" << std::flush; });

    tp.add([&tp]() {std::this_thread::sleep_for(std::chrono::microseconds(rand())); std::cout << "15:" << tp.getNumberOfThreads() << "\n" << std::flush; });
    tp.add([&tp]() {std::this_thread::sleep_for(std::chrono::microseconds(rand())); std::cout << "16:" << tp.getNumberOfThreads() << "\n" << std::flush; });
    tp.add([&tp]() {std::this_thread::sleep_for(std::chrono::microseconds(rand())); std::cout << "17:" << tp.getNumberOfThreads() << "\n" << std::flush; });
    tp.add([&tp]() {std::this_thread::sleep_for(std::chrono::microseconds(rand())); std::cout << "18:" << tp.getNumberOfThreads() << "\n" << std::flush; });
    tp.add([&tp]() {std::this_thread::sleep_for(std::chrono::microseconds(rand())); std::cout << "19:" << tp.getNumberOfThreads() << "\n" << std::flush; });
    tp.add([&tp]() {std::this_thread::sleep_for(std::chrono::microseconds(rand())); std::cout << "20:" << tp.getNumberOfThreads() << "\n" << std::flush; });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    tp.add([&tp]() {std::this_thread::sleep_for(std::chrono::microseconds(rand())); std::cout << "21:" << tp.getNumberOfThreads() << "\n" << std::flush; });
    tp.add([&tp]() {std::this_thread::sleep_for(std::chrono::microseconds(rand())); std::cout << "22:" << tp.getNumberOfThreads() << "\n" << std::flush; });
    tp.add([&tp]() {std::this_thread::sleep_for(std::chrono::microseconds(rand())); std::cout << "23:" << tp.getNumberOfThreads() << "\n" << std::flush; });
    tp.add([&tp]() {std::this_thread::sleep_for(std::chrono::microseconds(rand())); std::cout << "24:" << tp.getNumberOfThreads() << "\n" << std::flush; });
    tp.add([&tp]() {std::this_thread::sleep_for(std::chrono::microseconds(rand())); std::cout << "25:" << tp.getNumberOfThreads() << "\n" << std::flush; });
    tp.add([&tp]() {std::this_thread::sleep_for(std::chrono::microseconds(rand())); std::cout << "26:" << tp.getNumberOfThreads() << "\n" << std::flush; });
    tp.add([&tp]() {std::this_thread::sleep_for(std::chrono::microseconds(rand())); std::cout << "27:" << tp.getNumberOfThreads() << "\n" << std::flush; });
    tp.add([&tp]() {std::this_thread::sleep_for(std::chrono::microseconds(rand())); std::cout << "28:" << tp.getNumberOfThreads() << "\n" << std::flush; });
    tp.add([&tp]() {std::this_thread::sleep_for(std::chrono::microseconds(rand())); std::cout << "29:" << tp.getNumberOfThreads() << "\n" << std::flush; });
    tp.add([&tp]() {std::this_thread::sleep_for(std::chrono::microseconds(rand())); std::cout << "30:" << tp.getNumberOfThreads() << "\n" << std::flush; });
    */
    std::this_thread::sleep_for(std::chrono::seconds(1000));

    std::cout << "Hello World!\n";
}