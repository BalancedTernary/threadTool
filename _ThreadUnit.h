#pragma once
#ifndef __THREAD_UNIT__
#define  __THREAD_UNIT__
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <chrono>
#include <variant>

class _ThreadUnit
{
public:
	typedef std::variant<nullptr_t, std::function<void(void)>, std::function<void(const volatile std::atomic<volatile bool>&)>> Task;
private:
	std::thread loop;
	std::mutex _mCondition;//条件变量锁
	std::mutex _mGet;//getOneFunction锁
	//std::mutex _mActivate;//
	std::condition_variable BlockingQueue;
	volatile std::atomic<volatile bool> loopFlag;
	std::function < Task(void)> getOneFunction;
	std::function<void(void)> onActivate;
	std::function<void(void)> onIdle;
	volatile std::atomic<volatile bool> activate;

private:
	void loopFunction();
public:
	_ThreadUnit(std::function<void(void)> onActivate=nullptr, std::function<void(void)> onIdle=nullptr);
	_ThreadUnit(std::function < Task(void)> functionSource, std::function<void(void)> onActivate = nullptr, std::function<void(void)> onIdle = nullptr);
	~_ThreadUnit();
public:
	void setFunctionSource(std::function < Task(void)> functionSource);
	void wakeUp();
	void setOnActivate(std::function<void(void)>);
	void setOnIdle(std::function<void(void)>);
	bool isActivate();
	

}; 

#endif


