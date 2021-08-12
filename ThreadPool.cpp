#include "ThreadPool.h"
//#include <iostream>
using namespace std;
using namespace threadTool;
ThreadPool::ThreadPool()
{
	//funSrc = [this]() {return functionSource(); };
	//fromAct = [this]() {return fromActivate(); };
	//fromIdl = [this]() {return fromIdle(); };
	funSrc = std::bind(&ThreadPool::functionSource, this);
	fromAct = std::bind(&ThreadPool::fromActivate, this);
	fromIdl = std::bind(&ThreadPool::fromIdle, this);
	idleStartTime = std::chrono::time_point<std::chrono::high_resolution_clock>::max();
	numberOfThreads = 0;
	numberOfIdles = 0;
	wakeUpLength = 0;


	redundancyRatio = 1;
	idleLife = std::chrono::nanoseconds(0);
	minimumNumberOfThreads = 0;
	maximumNumberOfThreads = 1;

	loopFlag = true;
	serviceLoop= thread(&ThreadPool::mainService, this);
}

ThreadPool::~ThreadPool()
{
	{
		unique_lock<mutex> m(_mCondition);
		BlockingDeleting.wait(m, [this]()
			{
				unique_lock<mutex> m(_mFunctionDeque);
				return functionDeque.size() <= 0;
			});
		loopFlag = false;
		BlockingQueue.notify_all();
	}
	if (serviceLoop.joinable())
	{
		serviceLoop.join();
	}
}

void ThreadPool::join()
{
	unique_lock<mutex> m(_mCondition);
	BlockingDeleting.wait(m, [this]()
		{
			unique_lock<mutex> m(_mFunctionDeque);
			return (functionDeque.size() <= 0 && numberOfThreads <= numberOfIdles);
		});

}

void ThreadPool::mainService()
{
	while (loopFlag)
	{
		while (wakeUpLength > 0)
		{
			//--wakeUpLength;
			{
				unique_lock<mutex> m(_mThreadDeque);
				if (threadDeque.size() > 0)
				{

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
			}
			size_t dequeLength;
			{
				unique_lock<mutex> m(_mFunctionDeque);
				dequeLength = functionDeque.size();
			}
			
			if ((numberOfThreads + 1) 
				<= multMath::max<int_least64_t, int_least64_t>
				(multMath::min<int_least64_t, int_least64_t>
					(maximumNumberOfThreads
						, redundancyRatio 
						* (numberOfThreads - numberOfIdles + dequeLength))
					, minimumNumberOfThreads))
			{
				unique_lock<mutex> m(_mThreadDeque);
				threadDeque.emplace_back(funSrc, fromAct, fromIdl);
				++numberOfThreads;
				++numberOfIdles;
			}
			if (numberOfIdles <= 0)
			{
				break;
			}
		}
		
		size_t dequeLength;
		{
			unique_lock<mutex> m(_mFunctionDeque);
			dequeLength = functionDeque.size();
		}

		unique_lock<mutex> m(_mCondition);
		std::chrono::time_point<std::chrono::high_resolution_clock> time;
		{
			unique_lock<mutex> m(_mTime);
			time = idleStartTime;
		}

		if (numberOfThreads 
			> multMath::max<int_least64_t, int_least64_t>
			(multMath::min<int_least64_t, int_least64_t>
				(maximumNumberOfThreads
					, redundancyRatio 
					* (numberOfThreads - numberOfIdles + dequeLength))
				, minimumNumberOfThreads))
		{//线程超了，减少线程
			if (numberOfIdles > 0 && (chrono::high_resolution_clock::now() - time) > idleLife)
			{
				{
					unique_lock<mutex> m(_mThreadDeque);

					auto unit = threadDeque.begin();
					while (unit != threadDeque.end())
					{
						if (!unit->isActivate())
						{
							--numberOfIdles;
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
			BlockingQueue.wait_until(m, multMath::min<std::chrono::time_point<std::chrono::high_resolution_clock>, std::chrono::time_point<std::chrono::high_resolution_clock>>(chrono::high_resolution_clock::now() + idleLife, time));

		}
		else
		{
			BlockingQueue.wait(m);
		}
		////增加：在适当时候永久休眠等待唤醒以节约性能
		//BlockingQueue.wait_until(m, multMath::min<std::chrono::time_point<std::chrono::high_resolution_clock>, std::chrono::time_point<std::chrono::high_resolution_clock>>(chrono::high_resolution_clock::now() + idleLife, time));		
	}
}

_ThreadUnit::Task ThreadPool::functionSource()
{
	_ThreadUnit::Task fun;
	{
		unique_lock<mutex> m(_mFunctionDeque);
		if (functionDeque.size() > 0)
		{
			--wakeUpLength;
			fun = functionDeque.front();
			functionDeque.pop_front();
		}
		else
		{
			fun = nullptr;
		}
	}
	BlockingDeleting.notify_all();
	return fun;
}

void ThreadPool::fromActivate()
{
	{
		--numberOfIdles;
	}
	size_t dequeLength;
	{
		unique_lock<mutex> m(_mFunctionDeque);
		dequeLength = functionDeque.size();
	}
	if (dequeLength > 0 
		|| (numberOfThreads
			< multMath::max<int_least64_t, int_least64_t>
			(multMath::min<int_least64_t, int_least64_t>
				(maximumNumberOfThreads
					, redundancyRatio 
					* (numberOfThreads - numberOfIdles))
				, minimumNumberOfThreads)))
	{
		unique_lock<mutex> m(_mTime);
		idleStartTime= std::chrono::time_point<std::chrono::high_resolution_clock>::max();
	}
}

void ThreadPool::fromIdle()
{
	{
		++numberOfIdles;
	}

	size_t dequeLength;
	{
		unique_lock<mutex> m(_mFunctionDeque);
		dequeLength = functionDeque.size();
	}

	if (dequeLength <= 0 && 
		(numberOfThreads
			> multMath::max<int_least64_t, int_least64_t>
			(multMath::min<int_least64_t, int_least64_t>
				(maximumNumberOfThreads
					, redundancyRatio 
					* (numberOfThreads - numberOfIdles))
				, minimumNumberOfThreads)))
	{
		{
			unique_lock<mutex> m(_mTime);
			if (idleStartTime == std::chrono::time_point<std::chrono::high_resolution_clock>::max())
			{
				idleStartTime = chrono::high_resolution_clock::now();
			}
		}
		//unique_lock<mutex> m(_mCondition);
		BlockingDeleting.notify_all();
	}
}

void ThreadPool::add(_ThreadUnit::Task fun)
{
	{
		unique_lock<mutex> m(_mFunctionDeque);
		functionDeque.push_back(fun);
	}
	++wakeUpLength;
	//unique_lock<mutex> m(_mCondition);
	BlockingQueue.notify_one();
}

void ThreadPool::setMinimumNumberOfThreads(const uint_fast64_t& in)
{
	minimumNumberOfThreads = in;
	unique_lock<mutex> m(_mThreadDeque);
	while (threadDeque.size() < in)
	{
		threadDeque.emplace_back(funSrc, fromAct, fromIdl);
		++numberOfThreads;
		++numberOfIdles;
	}

}

void ThreadPool::setMaximumNumberOfThreads(const uint_fast64_t& in)
{
	maximumNumberOfThreads = in;
}

void ThreadPool::setIdleLife(const std::chrono::nanoseconds& in)
{
	idleLife = in;
}

void ThreadPool::setRedundancyRatio(const double& in)
{
	if (in < 1)
	{
		cerr << "'redundancyRatio' should not be less than 1." << endl << flush;
	}
	redundancyRatio = in;
}

uint_fast64_t ThreadPool::getMinimumNumberOfThreads()
{
	return minimumNumberOfThreads;
}

uint_fast64_t ThreadPool::getMaximumNumberOfThreads()
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

int_least64_t ThreadPool::getNumberOfIdles()
{
	/*{
		if (numberOfIdles < 0)
		{
			std::cerr << "\an\au\am\ab\ae\ar\aO\af\aI\ad\al\ae < 0" << std::endl << flush;
		}
	}*/
	return numberOfIdles;
}


