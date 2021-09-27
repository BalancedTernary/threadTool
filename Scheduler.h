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
#include <map>
#include "multMath.h"

#include "ThreadPool.h"
#include "Atomic.h"
#include "Async.h"
#include "DeluxeMutex.h"

namespace threadTool
{
	class _Scheduler
	{
	public:
		static class _SchedulerUnit
		{
			friend _Scheduler;
		private:
			_Scheduler* const scheduler;
			const uint_fast64_t id;
			Atomic<bool> deleted;
			std::shared_ptr<Atomic<int_fast64_t>> runningTimesPtr = nullptr;
			std::shared_ptr<std::condition_variable> BlockingQueuePtr = nullptr;
			std::shared_ptr<std::mutex> ptr_m = nullptr;
			//std::shared_ptr<Mutex> ptr_m;
		private:
			_SchedulerUnit(_Scheduler*, const uint_fast64_t&);
		public:
			_SchedulerUnit();
			_SchedulerUnit(const _SchedulerUnit&);
			_SchedulerUnit& operator= (const _SchedulerUnit&);
			void deleteUnit();
			void join();
		};
	private:
		static class _TaskUnit
		{
		public:
			std::chrono::time_point<std::chrono::high_resolution_clock> timePoint;
			std::chrono::time_point<std::chrono::high_resolution_clock> nextPoint;
			uint_fast64_t id;
			std::function<void(void)> task;
			std::shared_ptr<Atomic<int_fast64_t>> runningTimesPtr;
			std::shared_ptr<std::condition_variable> BlockingQueuePtr;
			std::shared_ptr<std::mutex> ptr_m;
			//std::shared_ptr<Mutex> ptr_m;
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
		_SchedulerUnit add(const uint_fast64_t& id, std::function<void(void)> task, const std::chrono::time_point<std::chrono::high_resolution_clock>& timePoint, const std::chrono::time_point<std::chrono::high_resolution_clock>& nextPoint);
	public:
		_Scheduler(ThreadPool& threadPool);
		~_Scheduler();
	public:
		_SchedulerUnit addInterval(std::function<void(void)> task, const std::chrono::nanoseconds& duration);
		_SchedulerUnit addTimeOutFor(std::function<void(void)> task, const std::chrono::nanoseconds& duration);
		_SchedulerUnit addTimeOutUntil(std::function<void(void)> task, const std::chrono::time_point<std::chrono::high_resolution_clock>& timePoint);

	};

	class Scheduler
	{
	private:
		static std::mutex _m;
		static std::map<ThreadPool*, std::shared_ptr<_Scheduler>> schedulers;
		std::shared_ptr<_Scheduler> scheduler;
		const ThreadPool* threadPool;
	public:
		Scheduler(ThreadPool& threadPool = GlobalThreadPool::get());
		~Scheduler();
		_Scheduler* operator->();
	};
};

#endif

