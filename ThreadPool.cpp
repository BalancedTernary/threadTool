#include "ThreadPool.h"
//#include <iostream>
using namespace std;
using namespace threadTool;
ThreadPool::ThreadPool()
{
	//funSrc = [this]() {return functionSource(); };
	//fromAct = [this]() {return fromActivate(); };
	//fromIdl = [this]() {return fromIdle(); };
	unique_lock<mutex> m1(_mCondition);
	unique_lock<Mutex> m2(_mFunctionDeque);
	funSrc = std::bind(&ThreadPool::functionSource, this);
	fromAct = std::bind(&ThreadPool::fromActivate, this);
	fromIdl = std::bind(&ThreadPool::fromIdle, this);
	idleStartTime = std::chrono::time_point<std::chrono::high_resolution_clock>::max();
	numberOfThreads = 0;
	numberOfIdles = 0;
	wakeUpLength = 0;


	redundancyRatio = 1.5;
	idleLife = std::chrono::minutes(1);
	minimumNumberOfThreads = 0;
	maximumNumberOfThreads = omp_get_num_procs();

	loopFlag = true;
	serviceLoop= thread(&ThreadPool::mainService, this);
}

ThreadPool::~ThreadPool()
{
	{
		unique_lock<mutex> m(_mCondition);
		loopFlag = false;
		BlockingDeleting.wait(m, [this]()
			{
				unique_readLock _m(_mFunctionDeque);
				return functionDeque.size() <= 0;
			});
		BlockingQueue.notify_all();
	}
	if (serviceLoop.joinable())
	{
		serviceLoop.join();
	}
	{
		unique_writeLock m3(_mThreadDeque);
		threadDeque.clear();
	}
}

void ThreadPool::join()
{
	unique_lock<mutex> m(_mCondition);
	BlockingDeleting.wait(m, [this]()
		{
			unique_readLock m(_mFunctionDeque);
			return (functionDeque.size() <= 0 && numberOfThreads <= numberOfIdles);
		});

}

void ThreadPool::mainService()
{
	while (true)
	{
		unique_readLock m1(_mFunctionDeque);

		if (!(loopFlag || functionDeque.size() > 0))
		{
			break;
		}
		if (!loopFlag)
		{
			if (wakeUpLength > 0)
			{

				auto unit = threadDeque.begin();
				if (unit != threadDeque.end())
				{
					unique_lock<mutex> m(_mThreadDeque);
					unit->notifyTaskExit();
					threadDeque.splice(threadDeque.end(), threadDeque, threadDeque.begin());
				}
			}
		}
		if (wakeUpLength > 0)
		{
			unique_readUnlock m2(_mFunctionDeque);
			
			while (wakeUpLength > 0)
			{
				{
					unique_lock<mutex> m(_mThreadDeque);
					if (numberOfIdles>0)
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
				{
					unique_readLock m3(_mFunctionDeque);

					size_t dequeLength;
					{
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
			}
			{
				unique_lock<mutex> m(_mCondition);
				if (wakeUpLength > 0 && loopFlag)
				{
					BlockingQueue.wait(m);
				}
			}
		}
		else
		{
			size_t dequeLength;
			{
				dequeLength = functionDeque.size();
			}
			{
				if (!(loopFlag || functionDeque.size() > 0))
				{
					break;
				}
			}
			{
				unique_readUnlock m4(_mFunctionDeque);
				unique_lock<mutex> m(_mCondition);
				if (loopFlag)
				{
					if (numberOfThreads
			> multMath::max<int_least64_t, int_least64_t>
						(multMath::min<int_least64_t, int_least64_t>
							(maximumNumberOfThreads
								, redundancyRatio
								* (numberOfThreads - numberOfIdles + dequeLength))
							, minimumNumberOfThreads))
					{//线程超了，减少线程
						std::chrono::time_point<std::chrono::high_resolution_clock> time;
						{
							unique_writeUnlock m0(_mCondition);
							{
								unique_lock<mutex> m(_mTime);
								time = idleStartTime;
							}
							if (numberOfIdles > 0 && (time) < (chrono::high_resolution_clock::now() - idleLife))
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
						}
						BlockingQueue.wait_until(m, multMath::min<std::chrono::time_point<std::chrono::high_resolution_clock>, std::chrono::time_point<std::chrono::high_resolution_clock>>(chrono::high_resolution_clock::now() + idleLife, multMath::max<std::chrono::time_point<std::chrono::high_resolution_clock>, std::chrono::time_point<std::chrono::high_resolution_clock>>(time + idleLife, time)));

					}
					else
					{
						BlockingQueue.wait(m);
					}
				}
			}
		}

	}
}

_ThreadUnit::Task ThreadPool::functionSource()
{
	_ThreadUnit::Task fun;
	{
		unique_lock<Mutex> m(_mFunctionDeque);
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
	{
		unique_lock<mutex> m(_mCondition);
		if (wakeUpLength <= 0)
		{
			BlockingQueue.notify_all();
		}
		BlockingDeleting.notify_all();
		return fun;
	}
}

void ThreadPool::fromActivate()
{
	{
		--numberOfIdles;
	}
	size_t dequeLength;
	{
		unique_readLock m(_mFunctionDeque);
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
		unique_readLock m(_mFunctionDeque);
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
		{
			unique_lock<mutex> m(_mCondition);
			BlockingDeleting.notify_all();
		}
	}
}

void ThreadPool::add(_ThreadUnit::Task fun)
{
	{
		unique_lock<Mutex> m(_mFunctionDeque);
		functionDeque.push_back(fun);
		++wakeUpLength;
	}
	{
		unique_lock<mutex> m(_mCondition);
		BlockingQueue.notify_one();
	}
}

void ThreadPool::setMinimumNumberOfThreads(const uint_fast64_t& in)
{
	minimumNumberOfThreads = in;
	{
		unique_lock<mutex> m(_mThreadDeque);
		while (threadDeque.size() < in)
		{
			threadDeque.emplace_back(funSrc, fromAct, fromIdl);
			++numberOfThreads;
			++numberOfIdles;
		}
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
	return numberOfIdles;
}

std::mutex GlobalThreadPool::_m = std::mutex();

ThreadPool* GlobalThreadPool::threadPool = nullptr;

ThreadPool& GlobalThreadPool::get()
{
	if (threadPool == nullptr)
	{
		unique_lock<mutex> m(_m);
		if (threadPool == nullptr)
		{//创建后永不删除
			threadPool = new ThreadPool();
			threadPool->setMaximumNumberOfThreads(ULONG_MAX);
		}
	}
	return *threadPool;
}




