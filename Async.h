#pragma once
#ifndef __ASYNC__
#define  __ASYNC__
/*#include <functional>
#include <mutex>
#include <atomic>
#include <condition_variable>*/
#include <mutex>
#include <variant>
#include <type_traits>
#include "MineMutex.h"
#include "ThreadPool.h"

namespace threadTool
{
	template<typename _Tp>
	class Async
	{
	private:
		typedef std::variant<std::function<_Tp(void)>, std::function<_Tp(AtomicConstReference<bool>)>> funcType;
	private:
		Mutex _m;
		ThreadPool& threadPool;
		funcType function;
		_Tp buffer;
	public:
		Async(ThreadPool& threadPool, funcType function)
			:threadPool(threadPool), function(function)
		{
			_m.lock();
			if (function.index() == 0)
			{
				threadPool.add([this](void) {
				buffer = std::get<0>(this->function)();
				_m.try_unlock();
				return; });
			}
			else
			{
				threadPool.add([this](AtomicConstReference<bool> flag) {
					buffer = std::get<1>(this->function)(flag);
					_m.try_unlock();
					return; });
			}
			
		}

		~Async()
		{
			_m.try_unlock();
		}

		void reRun()
		{
			_m.lock();
			if (function.index == 0)
			{
				threadPool.add([this](void) {
					buffer = this->function();
					_m.try_unlock();
					return; });
			}
			else
			{
				threadPool.add([this](AtomicConstReference<bool> flag) {
					buffer = this->function(flag);
					_m.try_unlock();
					return; });
			}
		}

		_Tp get()
		{
			unique_readLock m{ _m };
			return buffer;
		}

		bool check()
		{
			return _m._AllowWrite();
		}
	};
};

#endif

