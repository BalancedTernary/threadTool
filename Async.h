#pragma once
#ifndef __ASYNC__
#define  __ASYNC__
/*#include <functional>
#include <mutex>
#include <atomic>
#include <condition_variable>*/
#include <mutex>
#include "MineMutex.h"
#include "ThreadPool.h"

namespace threadTool
{
	template<typename _Tp>
	class Async
	{
	private:
		Mutex _m;
		ThreadPool& threadPool;
		std::function<_Tp(void)> function;
		_Tp buffer;
	public:
		Async(ThreadPool& threadPool, std::function<_Tp(void)> function)
			:threadPool(threadPool), function(function)
		{
			_m.lock();
			threadPool.add([this](void) {
				buffer = this->function();
				_m.try_unlock();
				return; });
		}

		~Async()
		{
			_m.try_unlock();
		}

		void reRun()
		{
			_m.lock();
			threadPool.add([this]() {buffer = function(); _m.try_unlock(); });
		}

		_Tp get()
		{
			std::unique_lock<Mutex> m{ _m };
			return buffer;
		}

		bool check()
		{
			return _m._AllowWrite();
		}
	};
};

#endif

