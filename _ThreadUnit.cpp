#include "_ThreadUnit.h"
using namespace std;
using namespace threadTool;
_ThreadUnit::_ThreadUnit(std::function<void(void)> onActivate, std::function<void(void)> onIdle)
{
	unique_lock<mutex> m(_mCondition);
	_ThreadUnit::onActivate = onActivate;
	_ThreadUnit::onIdle = onIdle;
	getOneFunction = nullptr;
	activate = false;
	loopFlag = true;
	loop = thread(&_ThreadUnit::loopFunction, this);
}

_ThreadUnit::_ThreadUnit(std::function < Task(void)> functionSource, std::function<void(void)> onActivate, std::function<void(void)> onIdle)
{
	unique_lock<mutex> m(_mCondition);
	_ThreadUnit::onActivate = onActivate;
	_ThreadUnit::onIdle = onIdle;
	getOneFunction = functionSource;
	activate = false;
	notifyExit = false;
	loopFlag = true;
	loop = thread(&_ThreadUnit::loopFunction, this);
}

_ThreadUnit::~_ThreadUnit()
{
	{
		unique_lock<mutex> m(_mCondition);
		notifyExit = false;
		loopFlag = false;
		BlockingQueue.notify_all();
	}
	if (loop.joinable())
	{
		loop.join();
	}
}

void _ThreadUnit::notifyTaskExit()
{
	unique_lock<mutex> m(_mCondition);
	if (loopFlag)
	{
		notifyExit = true;
		loopFlag = false;
	}
	BlockingQueue.notify_all();
}

void _ThreadUnit::setFunctionSource(std::function < Task(void)> functionSource)
{
	unique_lock<mutex> m(_mCondition);
	getOneFunction = functionSource;
}

void _ThreadUnit::wakeUp()
{
	unique_lock<mutex> m(_mCondition);
	BlockingQueue.notify_one();
}

void _ThreadUnit::loopFunction()
{
	while (loopFlag)
	{
		try {
			//unique_lock<mutex> m(_mCondition);
			Task function;
			{
				if (getOneFunction != nullptr)
				{
					function = getOneFunction();
				}
				else
				{
					function = nullptr;
				}
			}
			unique_lock<mutex> m(_mCondition);
			if (function.index() > 0)
			{
				//idleStartTime = std::chrono::time_point<std::chrono::high_resolution_clock>::max();
				//unique_lock<mutex> m(_mActivate);
				{
					//unique_lock<mutex> m(_mCondition);
					if (!activate)
					{
						activate = true;
						if (onActivate != nullptr)
						{
							//m.unlock();
							onActivate();
							//m.lock();
						}
					}
				}
				{
					unique_writeUnlock m0(_mCondition);
					switch (function.index())
					{
					case 1:
						std::get<std::function<void(void)>>(function)();
						break;
					case 2:
						std::get<std::function<void(AtomicConstReference<bool>)>>(function)(loopFlag);
						break;
					default:
						break;
					}
				}
				{
					//std::cerr << "\nAAA\n" << std::endl << std::flush;
					//unique_lock<mutex> m(_mCondition);
					//std::cerr << "\nBBB\n" << std::endl << std::flush;

					if (notifyExit)//如果任务是被通知退出，则不应该结束线程池线程，所以恢复loopFlag
					{
						loopFlag = true;
						notifyExit = false;
					}
				}
			}
			else
			{
				//unique_lock<mutex> m(_mActivate);
				//idleStartTime = std::chrono::high_resolution_clock::now();

				//unique_lock<mutex> m(_mCondition);
				if (activate)
				{
					activate = false;
					if (onIdle != nullptr)
					{
						//m.unlock();
						onIdle();
						//m.lock();
					}
				}
				BlockingQueue.wait(m, [this](void) {return !loopFlag; });
			}
		}
		catch (...)
		{

		}
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

