#pragma once
#ifndef __THREAD_POOL__
#define  __THREAD_POOL__
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <list>
#include <chrono>
#include <any>
#include "MineMath.h"
#include "_ThreadUnit.h"
class ThreadPool
{
private:
	std::deque<std::function<void(void)>> functionDeque;
	std::list<ThreadUnit> threadDeque;
	std::thread serviceLoop;
	std::mutex _mCondition;//条件变量锁
	std::mutex _mFunctionDeque;
	std::mutex _mThreadDeque;
	std::mutex _mTime;
	std::condition_variable BlockingQueue;
	volatile std::atomic<bool> loopFlag;
	volatile std::atomic<uint_least64_t> wakeUpLength;


	volatile std::atomic<uint_least64_t> minimumNumberOfThreads;
	volatile std::atomic<uint_least64_t> maximumNumberOfThreads;
	std::chrono::duration<volatile uint_least64_t ,std::nano> idleLife;
	volatile std::atomic<double> redundancyRatio;//线程数 冗余率

private:
	std::chrono::time_point<std::chrono::high_resolution_clock> idleStartTime;
	volatile std::atomic<int_least64_t> numberOfThreads;
	volatile std::atomic<int_least64_t> numberOfIdles;


	std::function < std::function<void(void)>(void)> funSrc;
	std::function<void(void)> fromAct;
	std::function<void(void)> fromIdl;


public:
	ThreadPool();
	~ThreadPool();
private:
	void mainService();//连续一定时间空闲线程超过限制后如果functionDeque为空则锁定functionDeque（规定线程从后往前唤醒，从前往后删除。是否可以不对队列加锁只对要删除的元素加锁），并删除一个状态为空闲的线程，然后解除functionDeque的锁定
	std::function<void(void)> functionSource();
	void fromActivate();
	void fromIdle();
public:
	void add(const std::function<void(void)>&);

	void setMinimumNumberOfThreads(const uint_least64_t&);
	void setMaximumNumberOfThreads(const uint_least64_t&);
	void setIdleLife(const std::chrono::nanoseconds&);
	void setRedundancyRatio(const double&);
	uint_least64_t getMinimumNumberOfThreads();
	uint_least64_t getMaximumNumberOfThreads();
	std::chrono::nanoseconds getIdleLife();
	double getRedundancyRatio();
	std::chrono::time_point<std::chrono::high_resolution_clock> getIdleStartTime();
	int_least64_t getNumberOfThreads();
	int_least64_t getNumberOfIdles();


};

#endif

