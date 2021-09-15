#include "Scheduler.h"

#include <iomanip>

using namespace std;
using namespace threadTool;
Scheduler::Scheduler(ThreadPool& threadPool)
	:mainAsync(Async<int>(threadPool, [this](AtomicConstReference<bool> loopFlag) {mainService(loopFlag); return 0; }))
{
	Scheduler::threadPool = &threadPool;
	increment = 0;
	workFlag = true;
	//deleted = false;
	auto n = threadPool.getMaximumNumberOfThreads();
	if (n < 2)
	{
		cerr << "The maximum number of threads in the thread pool is at least 2, It is currently " << n << ", And it will be automatically set to 2."<< endl << flush;
		threadPool.setMaximumNumberOfThreads(2);
	}
	//mainAsync = Async<void>(threadPool, [this](AtomicConstReference<bool> loopFlag) {return mainService(loopFlag); });
	//threadPool.add([this](AtomicConstReference<bool> loopFlag) {return mainService(loopFlag); });

}

Scheduler::~Scheduler()
{
	//_mTaskList.lock();
	{
		unique_lock<mutex> m(_mTaskList);
		workFlag = false;
		BlockingQueue.notify_all();
	}
	//_mTaskList.unlock();
	mainAsync.get();
}

void Scheduler::mainService(AtomicConstReference<bool> loopFlag)
{
	do
	{
		{//删除需要删除的任务
			unique_lock<mutex> m(_mDeleDeque);
			for (auto& deleID : deleDeque)
			{
				unique_lock<mutex> m(_mTaskList);
				taskList.remove_if([&deleID](const _TaskUnit& task) {return deleID == task.id; });
			}
			deleDeque.clear();
		}
		{//执行任务与重新定时
			std::chrono::time_point<std::chrono::high_resolution_clock> nextTime = std::chrono::time_point<std::chrono::high_resolution_clock>::max();
			unique_lock<mutex> m(_mTaskList);
			if (taskList.size() <= 0)
			{
				workFlag = false;
				return;
				//BlockingQueue.wait(m);
			}
			else
			{
				for (auto task = taskList.begin(); task != taskList.end();)
				{
					if (task->timePoint <= std::chrono::high_resolution_clock::now())
					{
						//std::cerr << "add-time: " << fixed << setprecision(8) << std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000000000.0 << std::endl << std::flush;

						threadPool->add(task->task);
						if (task->nextPoint != std::chrono::time_point<std::chrono::high_resolution_clock>::max())
						{
							std::chrono::nanoseconds duration = task->nextPoint - task->timePoint;
							
							task->timePoint = task->nextPoint;
							task->nextPoint += duration;
							/*//这段代码不起作用，因为任务线程阻塞无法影响任务发送的时间
							auto timeOut = multMath::max(duration, task->timePoint - std::chrono::high_resolution_clock::now());
							auto realDuration = duration * (duration / timeOut);
							if ((duration / timeOut) < 1)
							{
								std::cerr << "sssssssssssssss" << endl << flush;
							}
							task->timePoint = std::chrono::high_resolution_clock::now() + realDuration;
							task->nextPoint = task->timePoint + duration;
							*/
							nextTime = multMath::min(nextTime, task->timePoint);
							std::list<Scheduler::_TaskUnit>::iterator subTask = task;
							do
							{
								if (subTask == taskList.end() || subTask->timePoint > task->timePoint)
								{
									taskList.insert(subTask, std::move(*task));
									break;
								}
								++subTask;
							} while (true);
						}
						task = taskList.erase(task);
					}
					else
					{
						nextTime = multMath::min(nextTime, task->timePoint);
						break;
					}
				}
				BlockingQueue.wait_until(m, nextTime);
			}
		}
	} while (workFlag && loopFlag);

}

void Scheduler::add(const uint_fast64_t& id, std::function<void(void)> task, const std::chrono::time_point<std::chrono::high_resolution_clock>& timePoint, const std::chrono::time_point<std::chrono::high_resolution_clock>& nextPoint)
{//在调用处加锁，函数内部不加锁
	if (!workFlag)
	{
		workFlag = true;
		auto n = threadPool->getMaximumNumberOfThreads();
		if (n < 2)
		{
			cerr << "The maximum number of threads in the thread pool is at least 2, It is currently " << n << ", And it will be automatically set to 2." << endl << flush;
			threadPool->setMaximumNumberOfThreads(2);
		}
		threadPool->add([this](AtomicConstReference<bool> loopFlag) {return mainService(loopFlag); });
	}
	_TaskUnit unit;
	unit.id = id;
	unit.task = task;
	unit.timePoint = timePoint;
	unit.nextPoint = nextPoint;
	auto subTask = taskList.begin();
	do
	{
		if (subTask == taskList.end() || subTask->timePoint > unit.timePoint)
		{
			taskList.insert(subTask, std::move(unit));
			break;
		}
		++subTask;
	} while (true);
}

Scheduler::_SchedulerUnit Scheduler::addInterval(std::function<void(void)> task, const std::chrono::nanoseconds& duration)
{
	auto id=increment.fetch_add(1);
	auto timePoint = std::chrono::high_resolution_clock::now() + duration;
	auto nextPoint= timePoint + duration;
	unique_lock<mutex> m(_mTaskList);
	add(id, task, timePoint, nextPoint);
	BlockingQueue.notify_one();
	return Scheduler::_SchedulerUnit(this, id);
}

Scheduler::_SchedulerUnit Scheduler::addTimeOutFor(std::function<void(void)> task, const std::chrono::nanoseconds& duration)
{
	_TaskUnit unit;
	auto id = increment.fetch_add(1);
	auto timePoint = std::chrono::high_resolution_clock::now() + duration;
	auto nextPoint = std::chrono::time_point<std::chrono::high_resolution_clock>::max();
	unique_lock<mutex> m(_mTaskList);
	add(id, task, timePoint, nextPoint);
	BlockingQueue.notify_one();
	return Scheduler::_SchedulerUnit(this, id);
}

Scheduler::_SchedulerUnit Scheduler::addTimeOutUntil(std::function<void(void)> task, const std::chrono::time_point<std::chrono::high_resolution_clock>& timePoint)
{
	_TaskUnit unit;
	auto id = increment.fetch_add(1);
	auto nextPoint = std::chrono::time_point<std::chrono::high_resolution_clock>::max();
	unique_lock<mutex> m(_mTaskList);
	add(id, task, timePoint, nextPoint);
	BlockingQueue.notify_one();
	return Scheduler::_SchedulerUnit(this, id);
}

Scheduler::_SchedulerUnit::_SchedulerUnit(Scheduler* scheduler, const uint_fast64_t& id)
	:scheduler(scheduler), id(id)
{
	deleted = false;
}

void Scheduler::_SchedulerUnit::deleteUnit()
{
	if (!deleted)
	{
		unique_lock<mutex> m(scheduler->_mTaskList);
		scheduler->deleDeque.push_back(id);
		deleted = true;
	}
}
