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
#include <iostream>
#include "Atomic.h"
#include "DeluxeMutex.h"

namespace threadTool
{
	class _ThreadUnit
	{
	public:
		
		typedef std::variant<nullptr_t, std::function<void(void)>, std::function<void(AtomicConstReference<bool>)>> Task;
	private:
		std::thread loop;
		std::mutex _mCondition;//条件变量锁
		std::condition_variable BlockingQueue;
		Atomic<bool> loopFlag;
		Atomic<bool> notifyExit;
		std::function < _ThreadUnit::Task(void)> getOneFunction;
		std::function<void(void)> onActivate;
		std::function<void(void)> onIdle;
		Atomic<bool> activate;

	private:
		void loopFunction();
	public:
		_ThreadUnit(std::function<void(void)> onActivate = nullptr, std::function<void(void)> onIdle = nullptr);
		_ThreadUnit(std::function < Task(void)> functionSource, std::function<void(void)> onActivate = nullptr, std::function<void(void)> onIdle = nullptr);
		//_ThreadUnit(_ThreadUnit&& threadUnit);
		//_ThreadUnit& operator= (_ThreadUnit&& threadUnit);
		~_ThreadUnit();
	public:
		void notifyTaskExit();
		void setFunctionSource(std::function < Task(void)> functionSource);
		void wakeUp();
		void setOnActivate(std::function<void(void)>);
		void setOnIdle(std::function<void(void)>);
		bool isActivate();


	};
};

#endif


