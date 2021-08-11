#include "_ThreadUnit.h"
using namespace std;

_ThreadUnit::_ThreadUnit(std::function<void(void)> onActivate, std::function<void(void)> onIdle)
{
	_ThreadUnit::onActivate = onActivate;
	_ThreadUnit::onIdle = onIdle;
	getOneFunction = nullptr;
	activate = false;
	loopFlag = true;
	loop = thread(&_ThreadUnit::loopFunction, this);
}

_ThreadUnit::_ThreadUnit(std::function < Task(void)> functionSource, std::function<void(void)> onActivate, std::function<void(void)> onIdle)
{
	_ThreadUnit::onActivate = onActivate;
	_ThreadUnit::onIdle = onIdle;
	getOneFunction = functionSource;
	activate = false;
	loopFlag = true;
	loop = thread(&_ThreadUnit::loopFunction, this);
}

_ThreadUnit::~_ThreadUnit()
{
	loopFlag = false;
	BlockingQueue.notify_all();
	if (loop.joinable())
	{
		loop.join();
	}
}

void _ThreadUnit::setFunctionSource(std::function < Task(void)> functionSource)
{
	unique_lock<mutex> m(_mGet);
	getOneFunction = functionSource;
}

void _ThreadUnit::wakeUp()
{
	unique_lock<mutex> m(_mCondition);
	BlockingQueue.notify_one();
}

void _ThreadUnit::loopFunction()
{
	try {
		while (loopFlag)
		{
			//unique_lock<mutex> m(_mCondition);
			Task function;
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
			if (function.index() > 0)
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
				switch (function.index())
				{
				case 1:
					std::get<std::function<void(void)>>(function)();
					break;
				case 2:
					std::get<std::function<void(const volatile std::atomic<volatile bool>&)>>(function)(loopFlag);
					break;
				default:
					break;
				}
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
	catch (...)
	{

	}
}

void _ThreadUnit::setOnActivate(std::function<void(void)> fun)
{
	unique_lock<mutex> m(_mCondition);
	onActivate = fun;
}

void _ThreadUnit::setOnIdle(std::function<void(void)> fun)
{
	unique_lock<mutex> m(_mCondition);
	onIdle = fun;
}

bool _ThreadUnit::isActivate()
{
	unique_lock<mutex> m(_mCondition);
	return activate;
}

