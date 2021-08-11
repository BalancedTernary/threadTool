#pragma once
#ifndef __MINE_MUTEX__
#define __MINE_MUTEX__
//ע�⿼����ٻ�������
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <map>
#include <limits>
#include <functional>
#include <chrono>
namespace threadTool
{
	class Mutex
	{
	private:
		std::mutex _M;
		std::condition_variable BlockingQueue;//��������
		volatile std::atomic<bool> writing;//������дʱΪtrue
		volatile std::atomic<unsigned long long> readingTimes;//���ڶ�����Ŀ
	public:
		const std::function<bool(void)> _AllowRead = [this]() {return !(writing || (readingTimes >= ULONG_MAX)); };//�ж��Ƿ������
		const std::function<bool(void)> _AllowWrite = [this]() {return !(writing || readingTimes); };//�ж��Ƿ�����д
	public:
		Mutex(const Mutex&) = delete;
		Mutex& operator=(const Mutex&) = delete;
		Mutex()
		{
			writing = false;
			readingTimes = 0;
		}
		void lock_write()
		{
			std::unique_lock<std::mutex> m(_M);
			BlockingQueue.wait(m, _AllowWrite);//�Զ������ͼ���m��_AllowWriteΪfalseʱ����������Ϊtrueʱ���ܽ������
			writing = true;
		}
		bool try_lock_write()
		{
			std::unique_lock<std::mutex> m(_M);
			if (!_AllowWrite())
			{
				return false;
			}
			writing = true;
			return true;
		}
		template <class _Clock, class _Duration>
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
			writing = true;
			return true;
		}
		template <class _Rep, class _Period>
		bool try_lock_write_for(const std::chrono::duration<_Rep, _Period>& _Rel_time)
		{
			return try_lock_write_until(std::chrono::steady_clock::now() + _Rel_time);
		}
		void unlock_write()
		{
			{
				std::unique_lock<std::mutex> m(_M);
				if (writing != true)
				{
					std::_Throw_Cpp_error(1);
				}
				writing = false;
			}
			BlockingQueue.notify_one();
		}
		bool try_unlock_write()
		{
			{
				std::unique_lock<std::mutex> m(_M);
				if (writing != true)
				{
					return false;
				}
				writing = false;
			}
			BlockingQueue.notify_one();
			return true;
		}
		void lock_read()
		{
			{
				std::unique_lock<std::mutex> m(_M);
				BlockingQueue.wait(m, _AllowRead);//�Զ������ͼ���m��_AllowWriteΪfalseʱ����������Ϊtrueʱ���ܽ������
				++readingTimes;
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
				++readingTimes;
			}
			BlockingQueue.notify_one();
			return true;
		}
		template <class _Clock, class _Duration>
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
				++readingTimes;
			}
			BlockingQueue.notify_one();
			return true;
		}
		template <class _Rep, class _Period>
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
				--readingTimes;
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
				--readingTimes;
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
		template <class _Rep, class _Period>
		bool try_lock_for(const std::chrono::duration<_Rep, _Period>& _Rel_time)
		{
			return try_lock_write_for(_Rel_time);
		}
		template <class _Clock, class _Duration>
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
		template <class _Rep, class _Period>
		bool try_lock_shared_for(const std::chrono::duration<_Rep, _Period>& _Rel_time)
		{
			return try_lock_read_for(_Rel_time);
		}
		template <class _Clock, class _Duration>
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

};

#endif
