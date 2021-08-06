#include "_ThreadUnit.h"
using namespace std;

ThreadUnit::ThreadUnit(std::function<void(void)> onActivate, std::function<void(void)> onIdle)
{
	ThreadUnit::onActivate = onActivate;
	ThreadUnit::onIdle = onIdle;
	getOneFunction = nullptr;
	activate = false;
	loopFlag = true;
	loop = thread(&ThreadUnit::loopFunction, this);
}

ThreadUnit::ThreadUnit(const std::function < std::function<void(void)>(void)>& functionSource, std::function<void(void)> onActivate, std::function<void(void)> onIdle)
{
	ThreadUnit::onActivate = onActivate;
	ThreadUnit::onIdle = onIdle;
	getOneFunction = functionSource;
	activate = false;
	loopFlag = true;
	loop = thread(&ThreadUnit::loopFunction, this);
}

ThreadUnit::~ThreadUnit()
{
	loopFlag = false;
	BlockingQueue.notify_all();
	if (loop.joinable())
	{
		loop.join();
	}
	else
	{
		loop.detach();
	}
}

void ThreadUnit::setFunctionSource(const std::function < std::function<void(void)>(void)>& functionSource)
{
	unique_lock<mutex> m(_mGet);
	getOneFunction = functionSource;
}

void ThreadUnit::wakeUp()
{
	unique_lock<mutex> m(_mCondition);
	BlockingQueue.notify_one();
}

void ThreadUnit::loopFunction()
{
	while (loopFlag)
	{
		//unique_lock<mutex> m(_mCondition);
		std::function<void(void)> function;
		{
			unique_lock<mutex> m(_mGet);
			if (getOneFunction != nullptr)
			{
				function = getOneFunction();
			}
			else
			{
				function = nullptr;
			}
		}
		if (function != nullptr)
		{
			//idleStartTime = std::chrono::time_point<std::chrono::high_resolution_clock>::max();
			//unique_lock<mutex> m(_mActivate);
			{
				unique_lock<mutex> m(_mCondition);
				if (!activate)
				{
					activate = true;
					if (onActivate != nullptr)
					{

						onActivate();
					}
				}
			}
			function();
		}
		else
		{
			//unique_lock<mutex> m(_mActivate);
			//idleStartTime = std::chrono::high_resolution_clock::now();

			unique_lock<mutex> m(_mCondition);
			if (activate)
			{
				if (onIdle != nullptr)
				{
					onIdle();
				}
				activate = false;
			}
			BlockingQueue.wait(m);
		}
	}
}

void ThreadUnit::setOnActivate(const std::function<void(void)>& fun)
{
	unique_lock<mutex> m(_mCondition);
	onActivate = fun;
}

void ThreadUnit::setOnIdle(const std::function<void(void)>& fun)
{
	unique_lock<mutex> m(_mCondition);
	onIdle = fun;
}

bool ThreadUnit::isActivate()
{
	unique_lock<mutex> m(_mCondition);
	return activate;
}
