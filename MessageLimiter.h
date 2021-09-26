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
	std::multimap<std::string, std::any> callbacks; //回调表
	std::any condition; //直通条件
	std::any oldValue; //上次触发时的值
	const Mode mode; //工作模式
	std::chrono::nanoseconds period; //工作周期
public:
	/*
	* mode:
	* ”IMMEDIATE“: 立即触发；
	* “CONTINUE”: 连续值模式，类似于节流，可与直通条件配合；
	* ”DISPERSE“: 离散值模式，类似于防抖，可与直通条件配合；
	* “FILTER”: 过滤器模式，仅当直通条件为TRUE时才使消息通过；
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

