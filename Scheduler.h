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
#include "MineMath.h"

#include "ThreadPool.h"
class Scheduler
{
public:
	static class SchedulerUnit
	{
	private:
		Scheduler* const scheduler;
		const uint_fast64_t id;
		volatile std::atomic<volatile bool> deleted;
	public:
		SchedulerUnit(Scheduler*, const uint_fast64_t&);
		void deleteUnit();
	};
private:
	static class TaskUnit
	{
	public:
		std::chrono::time_point<std::chrono::high_resolution_clock> timePoint;
		std::chrono::time_point<std::chrono::high_resolution_clock> nextPoint;
		uint_fast64_t id;
		std::function<void(void)> task;
	};


private:
	ThreadPool* threadPool;
	std::list<TaskUnit> taskList;
	std::deque<uint_fast64_t> deleDeque;
	//std::mutex _mIncrement;
	std::mutex _mDeleDeque;
	std::mutex _mTaskList;
	std::condition_variable BlockingQueue;
	volatile std::atomic<volatile uint_fast64_t> increment;
	volatile std::atomic<volatile bool> workFlag;

private:
	void mainService(const volatile std::atomic<volatile bool>& loopFlag);
	void add(const uint_fast64_t& id, std::function<void(void)> task, const std::chrono::time_point<std::chrono::high_resolution_clock>& timePoint, const std::chrono::time_point<std::chrono::high_resolution_clock>& nextPoint);
public:
	Scheduler(ThreadPool& threadPool);
	~Scheduler();
public:
	SchedulerUnit addInterval(std::function<void(void)> task, const std::chrono::nanoseconds& duration);
	SchedulerUnit addTimeOutFor(std::function<void(void)> task, const std::chrono::nanoseconds& duration);
	SchedulerUnit addTimeOutUntil(std::function<void(void)> task, const std::chrono::time_point<std::chrono::high_resolution_clock>& timePoint);

};

#endif

