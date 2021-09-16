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
#include <variant>
#include <iostream>
#include "multMath.h"
#include "_ThreadUnit.h"
#include "Atomic.h"
#include "DeluxeMutex.h"

namespace threadTool
{
	class ThreadPool
	{
	private:
		std::deque<_ThreadUnit::Task> functionDeque;
		std::list<_ThreadUnit> threadDeque;
		std::thread serviceLoop;
		std::mutex _mCondition;//����������
		Mutex _mFunctionDeque;
		std::mutex _mThreadDeque;
		std::mutex _mTime;
		std::condition_variable BlockingQueue;
		std::condition_variable BlockingDeleting;

		Atomic<bool> loopFlag;
		Atomic<int_fast64_t> wakeUpLength;


		Atomic<int_fast64_t> minimumNumberOfThreads;
		Atomic<int_fast64_t> maximumNumberOfThreads;
		std::chrono::duration<volatile uint_fast64_t, std::nano> idleLife;
		Atomic<double> redundancyRatio;//�߳��� ������

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> idleStartTime;
		Atomic<int_least64_t> numberOfThreads;
		Atomic<int_least64_t> numberOfIdles;


		std::function <_ThreadUnit::Task(void)> funSrc;
		std::function<void(void)> fromAct;
		std::function<void(void)> fromIdl;


	public:
		ThreadPool();
		~ThreadPool();
		void join();//�ȴ���������ִ�����
	private:
		void mainService();//����һ��ʱ������̳߳������ƺ����functionDequeΪ��������functionDeque���涨�̴߳Ӻ���ǰ���ѣ���ǰ����ɾ�����Ƿ���Բ��Զ��м���ֻ��Ҫɾ����Ԫ�ؼ���������ɾ��һ��״̬Ϊ���е��̣߳�Ȼ����functionDeque������
		_ThreadUnit::Task functionSource();
		void fromActivate();
		void fromIdle();
	public:
		void add(_ThreadUnit::Task);

		void setMinimumNumberOfThreads(const uint_fast64_t&);
		void setMaximumNumberOfThreads(const uint_fast64_t&);
		void setIdleLife(const std::chrono::nanoseconds&);
		void setRedundancyRatio(const double&);
		uint_fast64_t getMinimumNumberOfThreads();
		uint_fast64_t getMaximumNumberOfThreads();
		std::chrono::nanoseconds getIdleLife();
		double getRedundancyRatio();
		std::chrono::time_point<std::chrono::high_resolution_clock> getIdleStartTime();
		int_least64_t getNumberOfThreads();
		int_least64_t getNumberOfIdles();

	};
};
#endif

