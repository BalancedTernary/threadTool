#include "ThreadPool.h"
#include "MineMath.h"
#include <iostream>
using namespace std;

ThreadPool::ThreadPool()
{
	funSrc = [this]() {return functionSource(); };
	fromAct = [this]() {return fromActivate(); };
	fromIdl = [this]() {return fromIdle(); };
	idleStartTime = std::chrono::time_point<std::chrono::high_resolution_clock>::max();
	numberOfThreads = 0;
	numberOfIdle = 0;
	
	loopFlag = true;
	serviceLoop= thread(&ThreadPool::mainService, this);
}

ThreadPool::~ThreadPool()
{
	loopFlag = false;
	BlockingQueue.notify_all();
	if (serviceLoop.joinable())
	{
		serviceLoop.join();
	}
	else
	{
		serviceLoop.detach();
	}
}

void ThreadPool::mainService()
{
	while (loopFlag)
	{
		unique_lock<mutex> m(_mCondition);
		std::chrono::time_point<std::chrono::high_resolution_clock> time;
		{
			unique_lock<mutex> m(_mTime);
			time = idleStartTime;
		}
		
		{
			//unique_lock<mutex> m(_mThreadDeque);

			if (numberOfThreads > mineMath::max<uint_least64_t, uint_least64_t>(mineMath::min<uint_least64_t, uint_least64_t>(maximumNumberOfThreads, redundancyRatio * (numberOfThreads - numberOfIdle)), minimumNumberOfThreads))
			{//线程超了，减少线程
				if (numberOfIdle > 0 && (chrono::high_resolution_clock::now() - time) > idleLife)
				{
					{
						unique_lock<mutex> m(_mThreadDeque);
						
						auto unit = threadDeque.begin();
						while(unit != threadDeque.end())
						{
							if (!unit->isActivate())
							{
								--numberOfIdle;
								--numberOfThreads;
								threadDeque.erase(unit);
								break;
							}
							unit++;
						}

					}
					{
						unique_lock<mutex> m(_mTime);
						idleStartTime = chrono::high_resolution_clock::now();
					}
				}

			}
		}
		BlockingQueue.wait_until(m, mineMath::min(chrono::high_resolution_clock::now(), time) + idleLife);
	}
}

std::function<void(void)> ThreadPool::functionSource()
{
	unique_lock<mutex> m(_mFunctionDeque);
	if (functionDeque.size() > 0)
	{
		std::function<void(void)> fun = functionDeque.front();
		functionDeque.pop_front();
		return fun;
	}
	else
	{
		return nullptr;
	}
}

void ThreadPool::fromActivate()
{
	{
		--numberOfIdle;
	}
	
	if (numberOfThreads < mineMath::max<uint_least64_t, uint_least64_t>(mineMath::min<uint_least64_t, uint_least64_t>(maximumNumberOfThreads, redundancyRatio * (numberOfThreads - numberOfIdle)), minimumNumberOfThreads))
	{
		unique_lock<mutex> m(_mTime);
		idleStartTime= std::chrono::time_point<std::chrono::high_resolution_clock>::max();
	}
}

void ThreadPool::fromIdle()
{
	{
		++numberOfIdle;
	}

	if (numberOfThreads > mineMath::max<uint_least64_t, uint_least64_t>(mineMath::min<uint_least64_t, uint_least64_t>(maximumNumberOfThreads, redundancyRatio * (numberOfThreads - numberOfIdle)), minimumNumberOfThreads))
	{
		unique_lock<mutex> m(_mTime);
		if (idleStartTime == std::chrono::time_point<std::chrono::high_resolution_clock>::max())
		{
			idleStartTime = chrono::high_resolution_clock::now();
		}
	}
}

void ThreadPool::add(const std::function<void(void)>& fun)
{
	{
		unique_lock<mutex> m(_mFunctionDeque);
		functionDeque.push_back(fun);
	}
	{
		unique_lock<mutex> m(_mThreadDeque);
		auto unit = threadDeque.end();
		do
		{
			--unit;
			if (!unit->isActivate())
			{
				unit->wakeUp();
				break;
			}
		} while (unit != threadDeque.begin());

	}
	if ((numberOfThreads + 1) <= mineMath::max<uint_least64_t, uint_least64_t>(mineMath::min<uint_least64_t, uint_least64_t>(maximumNumberOfThreads, redundancyRatio * (numberOfThreads - numberOfIdle)), minimumNumberOfThreads))
	{
		unique_lock<mutex> m(_mThreadDeque);
		threadDeque.emplace_back(funSrc, fromAct, fromIdl);
		++numberOfThreads;
		++numberOfIdle;
	}
}

void ThreadPool::setMinimumNumberOfThreads(const uint_least64_t& in)
{
	minimumNumberOfThreads = in;
	unique_lock<mutex> m(_mThreadDeque);
	while (threadDeque.size() < in)
	{
		threadDeque.emplace_back(funSrc, fromAct, fromIdl);
		++numberOfThreads;
		++numberOfIdle;
	}

}

void ThreadPool::setMaximumNumberOfThreads(const uint_least64_t& in)
{
	maximumNumberOfThreads = in;
}

void ThreadPool::setIdleLife(const std::chrono::nanoseconds& in)
{
	idleLife = in;
}

void ThreadPool::setRedundancyRatio(const double& in)
{
	redundancyRatio = in;
}

uint_least64_t ThreadPool::getMinimumNumberOfThreads()
{
	return minimumNumberOfThreads;
}

uint_least64_t ThreadPool::getMaximumNumberOfThreads()
{
	return maximumNumberOfThreads;
}

std::chrono::nanoseconds ThreadPool::getIdleLife()
{
	return idleLife;
}

double ThreadPool::getRedundancyRatio()
{
	return redundancyRatio;
}

std::chrono::time_point<std::chrono::high_resolution_clock> ThreadPool::getIdleStartTime()
{
	return idleStartTime;
}

int_least64_t ThreadPool::getNumberOfThreads()
{
	return numberOfThreads;
}

int_least64_t ThreadPool::getNumberOfIdle()
{
	return numberOfIdle;
}

