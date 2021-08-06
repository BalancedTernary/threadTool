#pragma once
#ifndef __THREAD_UNIT__
#define  __THREAD_UNIT__
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <chrono>
class ThreadUnit
{
private:
	std::thread loop;
	std::mutex _mCondition;//条件变量锁
	std::mutex _mGet;//getOneFunction锁
	//std::mutex _mActivate;//
	std::condition_variable BlockingQueue;
	volatile std::atomic<volatile bool> loopFlag;
	std::function < std::function<void(void)>(void)> getOneFunction;
	std::function<void(void)> onActivate;
	std::function<void(void)> onIdle;
	volatile std::atomic<volatile bool> activate;

private:
	void loopFunction();
public:
	ThreadUnit(std::function<void(void)> onActivate=nullptr, std::function<void(void)> onIdle=nullptr);
	ThreadUnit(const std::function < std::function<void(void)>(void)>& functionSource, std::function<void(void)> onActivate = nullptr, std::function<void(void)> onIdle = nullptr);
	~ThreadUnit();
public:
	void setFunctionSource(const std::function < std::function<void(void)>(void)>& functionSource);
	void wakeUp();
	void setOnActivate(const std::function<void(void)>&);
	void setOnIdle(const std::function<void(void)>&);
	bool isActivate();

}; 

#endif


