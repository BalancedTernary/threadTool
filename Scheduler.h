#pragma once
#ifndef __SCHEDULER__
#define  __SCHEDULER__
#include <deque>
#include <list>
#include <string>
#include <functional>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <iostream>
#include <chrono>
#include "multMath.h"

#include "ThreadPool.h"
#include "Atomic.h"
#include "Async.h"
#include "DeluxeMutex.h"

namespace threadTool
{
	class Scheduler
	{
	private:
		static class _SchedulerUnit
		{
		private:
			Scheduler* const scheduler;
			const uint_fast64_t id;
			Atomic<bool> deleted;
		public:
			_SchedulerUnit(Scheduler*, const uint_fast64_t&);
			void deleteUnit();
		};
	private:
		static class _TaskUnit
		{
		public:
			std::chrono::time_point<std::chrono::high_resolution_clock> timePoint;
			std::chrono::time_point<std::chrono::high_resolution_clock> nextPoint;
			uint_fast64_t id;
			std::function<void(void)> task;
		};


	private:
		ThreadPool* threadPool;
		std::list<_TaskUnit> taskList;
		std::deque<uint_fast64_t> deleDeque;
		//std::mutex _mIncrement;
		std::mutex _mDeleDeque;
		std::mutex _mTaskList;
		std::condition_variable BlockingQueue;
		Atomic<uint_fast64_t> increment;
		Atomic<bool> workFlag;
		//Atomic<bool> deleted;
		Async<int> mainAsync;
		
	private:
		void mainService(AtomicConstReference<bool> loopFlag);
		void add(const uint_fast64_t& id, std::function<void(void)> task, const std::chrono::time_point<std::chrono::high_resolution_clock>& timePoint, const std::chrono::time_point<std::chrono::high_resolution_clock>& nextPoint);
	public:
		Scheduler(ThreadPool& threadPool);
		~Scheduler();
	public:
		_SchedulerUnit addInterval(std::function<void(void)> task, const std::chrono::nanoseconds& duration);
		_SchedulerUnit addTimeOutFor(std::function<void(void)> task, const std::chrono::nanoseconds& duration);
		_SchedulerUnit addTimeOutUntil(std::function<void(void)> task, const std::chrono::time_point<std::chrono::high_resolution_clock>& timePoint);

	};
};

#endif

