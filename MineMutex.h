#pragma once
#ifndef __MINE_MUTEX__
#define __MINE_MUTEX__
//注意考虑虚假唤醒问题
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <map>
#include <set>
#include <limits>
#include <functional>
#include <chrono>
#include <sstream>
namespace threadTool
{
	//template<bool _RECUR = false>
	class Mutex
	{
	private:
		std::mutex _M;
		const bool _RECUR = false;//是否允许同一线程递归加解锁
		std::condition_variable BlockingQueue;//阻塞队列
		std::multiset<uint_fast64_t> readers; //当正在写时为所有者thread_id，否则为0
		volatile std::atomic<uint_fast64_t> writer; //当正在写时为所有者thread_id，否则为0
		volatile std::atomic<int_fast64_t> writingTimes;//正在写的数目（同一线程可多次获取锁）
		volatile std::atomic<int_fast64_t> readingTimes;//正在读的数目
	public:
		const std::function<bool(void)> _AllowRead = [this]() {return (!(writingTimes > 0 || (readingTimes >= ULONG_MAX))) || (_RECUR && (writer == getThreadID() || (writer == 0 && readers.size() <= readers.count(getThreadID())))); };//判断是否允许读
		const std::function<bool(void)> _AllowWrite = [this]() {return (!(writingTimes >0 || readingTimes>0)) || (_RECUR && (writer == getThreadID() || (writer == 0 && readers.size() <= readers.count(getThreadID())))); };//判断是否允许写
	private:
		void _onWrite()//仅在加锁函数内调用，所以不加锁
		{
			++writingTimes;
			writer = getThreadID();
		}
		void _onRead()//仅在加锁函数内调用，所以不加锁
		{
			++readingTimes;
			readers.insert(getThreadID());
		}
		void _unWrite()//仅在加锁函数内调用，所以不加锁
		{
			--writingTimes;
			if (writingTimes <= 0)
			{
				writer = 0;
			}
		}
		void _unRead()//仅在加锁函数内调用，所以不加锁
		{
			--readingTimes;
			readers.erase(readers.find(getThreadID()));//不能直接使用readers.erase(getThreadID())，因为会删掉值相同的全部元素
			if (readingTimes != readers.size())
			{
				std::_Throw_Cpp_error(1);
			}
		}
	public:
		Mutex(const Mutex&) = delete;
		Mutex& operator=(const Mutex&) = delete;
		Mutex(bool _RECUR = false)
			:_RECUR(_RECUR)
		{
			writingTimes = 0;
			readingTimes = 0;
		}
		static uint_fast64_t getThreadID()
		{
			std::stringstream s("");
			s << std::this_thread::get_id();
			uint_fast64_t id;
			s >> id;
			return id;
		}
		void lock_write()
		{
			std::unique_lock<std::mutex> m(_M);
			BlockingQueue.wait(m, _AllowWrite);//自动解锁和加锁m，_AllowWrite为false时才能阻塞，为true时才能解除阻塞
			_onWrite();
		}
		bool try_lock_write()
		{
			std::unique_lock<std::mutex> m(_M);
			if (!_AllowWrite())
			{
				return false;
			}
			_onWrite();
			return true;
		}
		template <typename _Clock, typename _Duration>
		bool try_lock_write_until(const std::chrono::time_point<_Clock, _Duration>& _Abs_time)
		{
			std::unique_lock<std::mutex> m(_M);
			if (!_AllowWrite())
			{
				if (!BlockingQueue.wait_until(m, _Abs_time, _AllowWrite))
				{
					return false;
				}
			}
			_onWrite();
			return true;
		}
		template <typename _Rep, typename _Period>
		bool try_lock_write_for(const std::chrono::duration<_Rep, _Period>& _Rel_time)
		{
			return try_lock_write_until(std::chrono::steady_clock::now() + _Rel_time);
		}
		void unlock_write()
		{
			{
				std::unique_lock<std::mutex> m(_M);
				if (writingTimes <= 0)
				{
					std::_Throw_Cpp_error(1);
				}
				_unWrite();
			}
			BlockingQueue.notify_one();
		}
		bool try_unlock_write()
		{
			{
				std::unique_lock<std::mutex> m(_M);
				if (writingTimes <= 0)
				{
					return false;
				}
				_unWrite();
			}
			BlockingQueue.notify_one();
			return true;
		}
		void lock_read()
		{
			{
				std::unique_lock<std::mutex> m(_M);
				BlockingQueue.wait(m, _AllowRead);//自动解锁和加锁m，_AllowWrite为false时才能阻塞，为true时才能解除阻塞
				_onRead();
			}
			BlockingQueue.notify_one();
		}
		bool try_lock_read()
		{
			{
				std::unique_lock<std::mutex> m(_M);
				if (!_AllowRead())
				{
					return false;
				}
				_onRead();
			}
			BlockingQueue.notify_one();
			return true;
		}
		template <typename _Clock, typename _Duration>
		bool try_lock_read_until(const std::chrono::time_point<_Clock, _Duration>& _Abs_time)
		{
			{
				std::unique_lock<std::mutex> m(_M);
				if (!_AllowRead())
				{
					if (!BlockingQueue.wait_until(m, _Abs_time, _AllowRead))
					{
						return false;
					}
				}
				_onRead();
			}
			BlockingQueue.notify_one();
			return true;
		}
		template <typename _Rep, typename _Period>
		bool try_lock_read_for(const std::chrono::duration<_Rep, _Period>& _Rel_time)
		{
			return try_lock_read_until(std::chrono::steady_clock::now() + _Rel_time);
		}
		void unlock_read()
		{
			{
				std::unique_lock<std::mutex> m(_M);
				if (readingTimes <= 0)
				{
					std::_Throw_Cpp_error(1);
				}
				_unRead();
			}
			BlockingQueue.notify_one();
		}
		bool try_unlock_read()
		{
			{
				std::unique_lock<std::mutex> m(_M);
				if (readingTimes <= 0)
				{
					return false;
				}
				_unRead();
			}
			BlockingQueue.notify_one();
			return true;
		}

		void lock()
		{
			lock_write();
		}
		bool try_lock()
		{
			return try_lock_write();
		}
		template <typename _Rep, typename _Period>
		bool try_lock_for(const std::chrono::duration<_Rep, _Period>& _Rel_time)
		{
			return try_lock_write_for(_Rel_time);
		}
		template <typename _Clock, typename _Duration>
		bool try_lock_until(const std::chrono::time_point<_Clock, _Duration>& _Abs_time)
		{
			return try_lock_write_until(_Abs_time);
		}
		void unlock()
		{
			unlock_write();
		}
		bool try_unlock()
		{
			return try_unlock_write();
		}



		void lock_shared()
		{
			lock_read();
		}
		bool try_lock_shared()
		{
			return try_lock_read();
		}
		template <typename _Rep, typename _Period>
		bool try_lock_shared_for(const std::chrono::duration<_Rep, _Period>& _Rel_time)
		{
			return try_lock_read_for(_Rel_time);
		}
		template <typename _Clock, typename _Duration>
		bool try_lock_shared_until(const std::chrono::time_point<_Clock, _Duration>& _Abs_time)
		{
			return try_lock_read_until(_Abs_time);
		}
		void unlock_shared()
		{
			unlock_read();
		}
		bool try_unlock_shared()
		{
			return try_unlock_read();
		}
	};

	template <typename _Mutex>
	class unique_writeLock
	{
	private:
		_Mutex& m;
	public:
		unique_writeLock(const unique_writeLock&) = delete;
		unique_writeLock& operator=(const unique_writeLock&) = delete;
		unique_writeLock(_Mutex& m)
			:m(m)
		{
			m.lock_write();
		}
		~unique_writeLock()
		{
			m.unlock_write();
		}
		_Mutex* operator->()
		{
			return &m;
		}
	};

	template <typename _Mutex>
	class unique_readLock
	{
	private:
		_Mutex& m;
	public:
		unique_readLock(const unique_readLock&) = delete;
		unique_readLock& operator=(const unique_readLock&) = delete;
		unique_readLock(_Mutex& m)
			:m(m)
		{
			m.lock_read();
		}
		~unique_readLock()
		{
			m.unlock_read();
		}
		_Mutex* operator->()
		{
			return &m;
		}
	};

};

#endif
