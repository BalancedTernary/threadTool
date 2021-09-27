#pragma once
#ifndef __MESSAGE_LIMITER__
#define __MESSAGE_LIMITER__
#include <any>
#include <functional>
#include <chrono>
#include <string>
#include <map>
#include "ThreadPool.h"
#include "Scheduler.h"

class MessageLimiter
{
public:
	enum Mode
	{
		IMMEDIATE,
		CONTINUE,
		DISPERSE,
		FILTER,
	};
private:
	threadTool::ThreadPool& threadPool;
	threadTool::Scheduler scheduler;
	threadTool::_Scheduler::_SchedulerUnit schedulerUnit;
	threadTool::Atomic<bool> isNew; //�����һ����������ʱ���ڴ���������Ϊtrue(��������ģʽ��Ч��
	std::multimap<std::string, std::any> callbacks; //�ص���
	std::any condition; //ֱͨ����
	std::any oldValue; //�ϴδ���ʱ��ֵ
	std::any newValue; //���µ�ֵ
	std::any previousValue; //�ϴ������ֵ
	const Mode mode; //����ģʽ
	std::chrono::nanoseconds period; //��������
	threadTool::Mutex _mValue;
	threadTool::Mutex _mCallbacks;
public:
	/*
	* mode:
	* ��IMMEDIATE��: ����������
	* ��CONTINUE��: ����ֵģʽ�������ڽ���������ֱͨ������ϣ�
	* ��DISPERSE��: ��ɢֵģʽ�������ڷ���������ֱͨ������ϣ�
	* ��FILTER��: ������ģʽ������ֱͨ����ΪTRUEʱ��ʹ��Ϣͨ����
	*/
	template<typename _T = int>
	MessageLimiter(_T value= 0, const Mode& mode = CONTINUE, const std::chrono::nanoseconds& period = std::chrono::milliseconds(100), std::function<bool(const _T& newValue, const _T& oldValue)> condition = [](const _T& newValue, const _T& oldValue) {return false; }, threadTool::ThreadPool& threadPool = threadTool::GlobalThreadPool::get())
		:threadPool(threadPool), mode(mode), scheduler(threadPool)
	{
		oldValue = value;
		newValue = value;
		MessageLimiter::period = period;
		MessageLimiter::condition = condition;
		isNew = false;
	}

	~MessageLimiter()
	{
		schedulerUnit.deleteUnit();
		schedulerUnit.join();
		threadTool::unique_writeLock m(_mValue);
		threadTool::unique_writeLock m2(_mCallbacks);
	}

	/*template<typename _T>
	void regist(std::function<_T(const _T&)> callback)
	{
		regist("", callback);
	}
	*/
	template<typename _T, typename... _Args>
	void regist(std::function<void(const _T&, const _Args&...)> callback)
	{
		{
			regist("", callback);
		}
	}

	/*template<typename _T>
	void regist(const std::string& key, std::function<_T(const _T&)> callback)
	{
		threadTool::unique_writeLock(_mCallbacks);
		callbacks.insert(key, callback);
	}*/

	template<typename _T, typename... _Args>
	void regist(const std::string& key, std::function<void(const _T&, const _Args&...)> callback)
	{
		threadTool::unique_writeLock m(_mCallbacks);
		callbacks.insert(std::make_pair(key, callback));
	}

	void unregist(const std::string& key)
	{
		threadTool::unique_writeLock m(_mCallbacks);
		callbacks.erase(key);
	}

	/*template<typename _T>
	void sendMessage(const _T& value);
	*/
	template<typename _T, typename... _Args>
	void sendMessage(const _T& value, const _Args&... parameter)
	{
		{
			threadTool::unique_writeLock m(_mValue);
			previousValue = newValue;
			newValue = value;
		}
		switch (mode)
		{
		case IMMEDIATE:
			immediateMethod(value, parameter...);
			break;
		case CONTINUE:
			continueMethod(value, parameter...);
			break;
		case DISPERSE:
			disperseMethod(value, parameter...);
			break;
		case FILTER:
			filterMethod(value, parameter...);
			break;
		default:
			break;
		}
	}

private:
	/*template<typename _T>
	void call(const _T& value)
	{
		{
			threadTool::unique_writeLock(_mValue);
			oldValue = value;
		}
		{
			threadTool::unique_readLock(_mCallbacks);
			for (auto& callback : callbacks)
			{
				std::any_cast<std::function<_T(const _T&)>>(callback)(value);
			}
		}
	}
	*/
	template<typename _T, typename... _Args>
	void call(const _T& value, const _Args&... parameter)
	{
		{
			//threadTool::unique_writeLock m(_mValue);
			oldValue = value;
		}
		{
			threadTool::unique_readLock m(_mCallbacks);
			for (auto& [key,callback] : callbacks)
			{
				threadPool.add([callback, value, parameter...]()
					{
						std::any_cast<std::function<void(const _T&, const _Args&...)>>(callback)(value, parameter...);
					});
				
			}
		}
	}

	template<typename _T, typename... _Args>
	void immediateMethod(const _T& value, const _Args&... parameter)
	{
		threadTool::unique_readLock m(_mValue);
		call(std::any_cast<_T>(newValue), parameter...);
	}

	template<typename _T, typename... _Args>
	void continueMethod(const _T& value, const _Args&... parameter)
	{
		threadTool::unique_readLock m(_mValue);
		if (!isNew || std::any_cast<std::function<bool(const _T & newValue, const _T & oldValue)>>(condition)(std::any_cast<_T>(newValue), std::any_cast<_T>(oldValue)))
		{
			schedulerUnit.deleteUnit();
			isNew = true;
			call(std::any_cast<_T>(newValue), parameter...);
			schedulerUnit = scheduler->addInterval([this, value, parameter...]()
			{
				threadTool::unique_readLock m(_mValue);
				if (std::any_cast<_T>(newValue) == std::any_cast<_T>(oldValue))
				{
					schedulerUnit.deleteUnit();
					isNew = false;
				}
				else
				{
					isNew = true;
					call(std::any_cast<_T>(newValue), parameter...);
				}
			}, period);
		}
	}

	template<typename _T, typename... _Args>
	void disperseMethod(const _T& value, const _Args&... parameter)
	{
		threadTool::unique_readLock m(_mValue);
		if (std::any_cast<_T>(newValue) != std::any_cast<_T>(previousValue))
		{
			schedulerUnit.deleteUnit();
			if (std::any_cast<std::function<bool(const _T & newValue, const _T & oldValue)>>(condition)(std::any_cast<_T>(newValue), std::any_cast<_T>(oldValue)))
			{
				call(std::any_cast<_T>(newValue), parameter...);
			}
			else
			{
				schedulerUnit = scheduler->addTimeOutFor([this, value, parameter...]()
				{
					threadTool::unique_readLock m(_mValue);
					if (std::any_cast<_T>(newValue) != std::any_cast<_T>(oldValue))
					{
						call(std::any_cast<_T>(newValue), parameter...);
					}
				}, period);
			}
		}
	}

	template<typename _T, typename... _Args>
	void filterMethod(const _T& value, const _Args&... parameter)
	{
		threadTool::unique_readLock m(_mValue);
		if (std::any_cast<std::function<bool(const _T & newValue, const _T & oldValue)>>(condition)(std::any_cast<_T>(newValue), std::any_cast<_T>(oldValue)))
		{
			call(std::any_cast<_T>(newValue), parameter...);
		}
	}
};



#endif

