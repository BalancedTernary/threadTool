#pragma once
#ifndef __MINE_MUTEX__
#define __MINE_MUTEX__
//ע�⿼����ٻ�������
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <atomic>
#include <map>
#include <set>
#include <limits>
#include <functional>
#include <chrono>
#include <sstream>
#include <iostream>
#include "Atomic.h"

#include <chrono>

namespace threadTool
{


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
			m.lock();
		}
		~unique_writeLock()
		{
			m.unlock();
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
			m.lock_shared();
		}
		~unique_readLock()
		{
			m.unlock_shared();
		}
		_Mutex* operator->()
		{
			return &m;
		}
	};

	template <typename _Mutex>
	class unique_writeUnlock
	{
	private:
		_Mutex& m;
	public:
		unique_writeUnlock(const unique_writeUnlock&) = delete;
		unique_writeUnlock& operator=(const unique_writeUnlock&) = delete;
		unique_writeUnlock(_Mutex& m)
			:m(m)
		{
			m.unlock();
		}
		~unique_writeUnlock()
		{
			m.lock();
		}
		_Mutex* operator->()
		{
			return &m;
		}
	};

	template <typename _Mutex>
	class unique_readUnlock
	{
	private:
		_Mutex& m;
	public:
		unique_readUnlock(const unique_readUnlock&) = delete;
		unique_readUnlock& operator=(const unique_readUnlock&) = delete;
		unique_readUnlock(_Mutex& m)
			:m(m)
		{
			m.unlock_shared();
		}
		~unique_readUnlock()
		{
			m.lock_shared();
		}
		_Mutex* operator->()
		{
			return &m;
		}
	};


	//template<bool _RECUR = false>
	class Mutex
	{
	private:
		std::mutex _M;
		const bool _RECUR = false;//�Ƿ�����ͬһ�̵߳ݹ�ӽ���
		std::condition_variable BlockingQueue;//��������
		std::multiset<std::thread::id> readers = {}; //������дʱΪ������thread_id������Ϊ0
		std::shared_mutex _mReaders;
		Atomic<std::thread::id> writer; //������дʱΪ������thread_id������Ϊ0
		Atomic<int_fast64_t> writingTimes;//����д����Ŀ��ͬһ�߳̿ɶ�λ�ȡ����
		Atomic<int_fast64_t> readingTimes;//���ڶ�����Ŀ
	public:
		const std::function<bool(void)> _AllowRead = [this]() {unique_readLock m(_mReaders); return (!(writingTimes > 0 || (readingTimes >= ULONG_MAX))) || (_RECUR && (writer == getThreadID() || (writer == std::thread::id() && readers.size() <= readers.count(getThreadID())))); };//�ж��Ƿ������
		const std::function<bool(void)> _AllowWrite = [this]() {unique_readLock m(_mReaders); return (!(writingTimes > 0 || readingTimes > 0)) || (_RECUR && (writer == getThreadID() || (writer == std::thread::id() && readers.size() <= readers.count(getThreadID())))); };//�ж��Ƿ�����д
	private:
		void _onWrite()//���ڼ��������ڵ��ã����Բ�����
		{
			++writingTimes;
			writer = getThreadID();
		}
		void _onRead()//���ڼ��������ڵ��ã����Բ�����
		{
			unique_writeLock m(_mReaders);
			++readingTimes;
			std::thread::id temp = getThreadID();
			readers.insert(temp);
			
		}
		void _unWrite()//���ڼ��������ڵ��ã����Բ�����
		{
			--writingTimes;
			if (writingTimes <= 0)
			{
				writer = std::thread::id();
			}
		}
		void _unRead()//���ڼ��������ڵ��ã����Բ�����
		{
			unique_writeLock m(_mReaders);
			--readingTimes;
			std::thread::id temp = getThreadID();
			if (readers.find(temp) == readers.end())
			{
				std::cerr<<"readers.find(temp) != readers.end()\n"<<std::flush;
			}
			readers.erase(readers.find(temp));//����ֱ��ʹ��readers.erase(getThreadID())����Ϊ��ɾ��ֵ��ͬ��ȫ��Ԫ��
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
			std::unique_lock<std::mutex> m(_M);
			writingTimes = 0;
			readingTimes = 0;
		}
		~Mutex()
		{
			std::unique_lock<std::mutex> m(_M);
			if (writingTimes > 0)
			{
				std::cerr << "����д�����������޷��ڱ�����ǰ����\n" << std::flush;
			}
			if (readingTimes > 0)
			{
				std::cerr << "���Ӷ������������޷��ڱ�����ǰ����\n" << std::flush;
			}
		}
		/*static uint_fast64_t getThreadID()
		{
			std::stringstream s("");
			s << std::this_thread::get_id();
			uint_fast64_t id;
			s >> id;
			//std::cerr << id << std::endl << std::flush;
			return id;
		}*/
		static std::thread::id getThreadID()
		{
			return std::this_thread::get_id();
		}
		void lock_write()
		{
			//auto start = std::chrono::high_resolution_clock::now();
			std::unique_lock<std::mutex> m(_M);
			BlockingQueue.wait(m, _AllowWrite);//�Զ������ͼ���m��_AllowWriteΪfalseʱ����������Ϊtrueʱ���ܽ������
			_onWrite();
			//auto end = std::chrono::high_resolution_clock::now();
			//std::cerr << getThreadID() << "-writeLockTime: " << (end - start).count() / 1000000000.0 << std::endl << std::flush;
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
			//auto start = std::chrono::high_resolution_clock::now();
			{
				std::unique_lock<std::mutex> m(_M);
				BlockingQueue.wait(m, _AllowRead);//�Զ������ͼ���m��_AllowWriteΪfalseʱ����������Ϊtrueʱ���ܽ������
				_onRead();
			}
			BlockingQueue.notify_one();
			//auto end = std::chrono::high_resolution_clock::now();
			//std::cerr << getThreadID() << "-readLockTime: " << (end - start).count() / 1000000000.0 << std::endl << std::flush;

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


};

#endif
