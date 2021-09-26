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
	std::multimap<std::string, std::any> callbacks; //�ص���
	std::any condition; //ֱͨ����
	std::any oldValue; //�ϴδ���ʱ��ֵ
	const Mode mode; //����ģʽ
	std::chrono::nanoseconds period; //��������
public:
	/*
	* mode:
	* ��IMMEDIATE��: ����������
	* ��CONTINUE��: ����ֵģʽ�������ڽ���������ֱͨ������ϣ�
	* ��DISPERSE��: ��ɢֵģʽ�������ڷ���������ֱͨ������ϣ�
	* ��FILTER��: ������ģʽ������ֱͨ����ΪTRUEʱ��ʹ��Ϣͨ����
	*/
	template<typename _T>
	MessageLimiter(threadTool::ThreadPool& threadPool, const Mode& mode = CONTINUE, const std::chrono::nanoseconds& period = std::chrono::milliseconds(100), std::function<bool(const _T& newValue, const _T& oldValue)> condition = [](const _T& newValue, const _T& oldValue) {return false; })
		:threadPool(threadPool), mode(mode)
	{
		MessageLimiter::period = period;
		MessageLimiter::condition = condition;
	}

	template<typename _T>
	void regist(std::function<_T(const _T&)> callback)
	{
		regist("", callback);
	}

	template<typename _T, typename... _Args>
	void regist(std::function<_T(const _T&, const _Args&...)> callback)
	{
		{
			regist("", callback);
		}
	}

	template<typename _T>
	void regist(const std::string& key, std::function<_T(const _T&)> callback)
	{
		callbacks.insert(key, callback);
	}

	template<typename _T, typename... _Args>
	void regist(const std::string& key, std::function<_T(const _T&, const _Args&...)> callback)
	{
		callbacks.insert(key, callback);
	}

	void unregist(const std::string& key)
	{
		callbacks.erase(key);
	}

	template<typename _T>
	void sendMessage(const _T& value);

	template<typename _T, typename... _Args>
	void sendMessage(const _T& value, const _Args&... parameter);

private:
	template<typename _T>
	void call(const _T& value)
	{
		oldValue = value;
		for (auto& callback : callbacks)
		{
			std::any_cast<std::function<_T(const _T&)>>(callback)(value);
		}
	}

	template<typename _T, typename... _Args>
	void call(const _T& value, const _Args&... parameter)
	{
		oldValue = value;
		for (auto& callback : callbacks)
		{
			std::any_cast<std::function<_T(const _T&, const _Args&...)>>(callback)(value, parameter...);
		}
	}

	//template<typename _T>
};

#endif

