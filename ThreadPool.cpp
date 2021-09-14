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
	maximumNumberOfThreads = 2;

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
				//BlockingQueue.notify_all();
				//unique_lock<mutex> m(_mFunctionDeque);
				//std::cerr << "_mFunctionDeque\n" << std::flush;
				unique_readLock _m(_mFunctionDeque);
				return functionDeque.size() <= 0;
			});
		//loopFlag = false;
		BlockingQueue.notify_all();
	}
	if (serviceLoop.joinable())
	{
		//std::cerr << "AAserviceLoop.join();" << std::endl << std::flush;
		serviceLoop.join();
		//std::cerr << "BBserviceLoop.join();" << std::endl << std::flush;
	}
	unique_writeLock m3(_mThreadDeque);
	threadDeque.clear();
	//unique_writeLock m2(_mFunctionDeque);
	//unique_writeLock m3(_mThreadDeque);
	//unique_writeLock m4(_mTime);
}

void ThreadPool::join()
{
	unique_lock<mutex> m(_mCondition);
	BlockingDeleting.wait(m, [this]()
		{
			//unique_lock<mutex> m(_mFunctionDeque);
			unique_readLock m(_mFunctionDeque);
			return (functionDeque.size() <= 0 && numberOfThreads <= numberOfIdles);
		});

}

void ThreadPool::mainService()
{
	//unique_writeLock m1(_mFunctionDeque);
	while (true)
	{
		unique_readLock m1(_mFunctionDeque);
		if (!(loopFlag || functionDeque.size() > 0))
		{
			break;
		}
		//unique_writeUnlock m2(_mFunctionDeque);
		if (!loopFlag)
		{
			//unique_writeLock m3(_mFunctionDeque);
			if (wakeUpLength > 0)
			//for (size_t i = 0; i < wakeUpLength; ++i)
			{
				auto unit = threadDeque.begin();
				if (unit != threadDeque.end())
				{
					//unique_writeUnlock m4(_mFunctionDeque);
					//unique_readUnlock m2(_mFunctionDeque);
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
				unique_readLock m3(_mFunctionDeque);
				//--wakeUpLength;
				{
					unique_lock<mutex> m(_mThreadDeque);
					//std::cerr << "step1" << std::endl << std::flush;

					if (threadDeque.size() > 0)
					{
						auto unit = threadDeque.end();
						do
						{
							--unit;
							if (!unit->isActivate())
							{
								unit->wakeUp();
								//++notifyTimes;
								break;
							}
						} while (unit != threadDeque.begin());
					}
				}
				size_t dequeLength;
				{
					//std::cerr << "step2" << std::endl << std::flush;

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
					//std::cerr << "step3" << std::endl << std::flush;
					threadDeque.emplace_back(funSrc, fromAct, fromIdl);
					++numberOfThreads;
					++numberOfIdles;
				}
				if (numberOfIdles <= 0)
				{
					break;
				}
			}
			unique_lock<mutex> m(_mCondition);
			if (wakeUpLength > 0 && loopFlag)
			{
				BlockingQueue.wait(m);
			}
		}
		else
		{



			size_t dequeLength;
			{
				//unique_readLock m(_mFunctionDeque);
				//std::cerr << "step4" << std::endl << std::flush;
				dequeLength = functionDeque.size();
			}

			//unique_lock<mutex> m(_mCondition);
			//std::cerr << "step5" << std::endl << std::flush;

			{
				//unique_lock<mutex> m(_mFunctionDeque);//不能用这个，因为循环退出后会再次解锁

				//unique_writeLock m5(_mFunctionDeque);
				if (!(loopFlag || functionDeque.size() > 0))
				{
					break;
				}
			}
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
							//std::cerr << "step6" << std::endl << std::flush;
							time = idleStartTime;
						}
						if (numberOfIdles > 0 && (chrono::high_resolution_clock::now() - time) > idleLife)
						{
							{
								unique_lock<mutex> m(_mThreadDeque);

								//std::cerr << "step7" << std::endl << std::flush;
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
								//std::cerr << "step8" << std::endl << std::flush;
								idleStartTime = chrono::high_resolution_clock::now();
							}
						}
					}
					//unique_lock<mutex> m(_mCondition);
					BlockingQueue.wait_until(m, multMath::min<std::chrono::time_point<std::chrono::high_resolution_clock>, std::chrono::time_point<std::chrono::high_resolution_clock>>(chrono::high_resolution_clock::now() + idleLife, time));
					//std::cerr << "step9" << std::endl << std::flush;


				}
				else
				{
					//unique_lock<mutex> m(_mCondition);
					BlockingQueue.wait(m);
					//std::cerr << "step10" << std::endl << std::flush;
				}
			}
		}
		////增加：在适当时候永久休眠等待唤醒以节约性能
		//BlockingQueue.wait_until(m, multMath::min<std::chrono::time_point<std::chrono::high_resolution_clock>, std::chrono::time_point<std::chrono::high_resolution_clock>>(chrono::high_resolution_clock::now() + idleLife, time));		

		//std::cerr << "step11" << std::endl << std::flush;
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
	unique_lock<mutex> m(_mCondition);
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
		//unique_lock<mutex> m(_mFunctionDeque);
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
		//unique_lock<mutex> m(_mFunctionDeque);
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
		unique_lock<mutex> m(_mCondition);
		BlockingDeleting.notify_all();
	}
}

void ThreadPool::add(_ThreadUnit::Task fun)
{
	{
		unique_lock<Mutex> m(_mFunctionDeque);
		functionDeque.push_back(fun);
		++wakeUpLength;
	}
	unique_lock<mutex> m(_mCondition);
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


