#include "Scheduler.h"
using namespace std;
Scheduler::Scheduler(ThreadPool& threadPool)
{
	Scheduler::threadPool = &threadPool;
	increment = 0;
	workFlag = true;
	auto n = threadPool.getMaximumNumberOfThreads();
	if (n < 2)
	{
		cerr << "The maximum number of threads in the thread pool is at least 2, It is currently " << n << ", And it will be automatically set to 2."<< endl << flush;
		threadPool.setMaximumNumberOfThreads(2);
	}
	threadPool.add([this](const volatile std::atomic<volatile bool>& loopFlag) {return mainService(loopFlag); });

}

Scheduler::~Scheduler()
{
	workFlag = false;
	unique_lock<mutex> m(_mTaskList);
	BlockingQueue.notify_all();
}

void Scheduler::mainService(const volatile std::atomic<volatile bool>& loopFlag)
{
	do
	{
		{//删除需要删除的任务
			unique_lock<mutex> m(_mDeleDeque);
			for (auto& deleID : deleDeque)
			{
				unique_lock<mutex> m(_mTaskList);
				taskList.remove_if([&deleID](const TaskUnit& task) {return deleID == task.id; });
			}
			deleDeque.clear();
		}
		{//执行任务与重新定时
			std::chrono::time_point<std::chrono::high_resolution_clock> nextTime = std::chrono::time_point<std::chrono::high_resolution_clock>::max();
			unique_lock<mutex> m(_mTaskList);
			if (taskList.size() <= 0)
			{
				BlockingQueue.wait(m);
			}
			else
			{
				for (auto task = taskList.begin(); task != taskList.end();)
				{
					if (task->timePoint <= std::chrono::high_resolution_clock::now())
					{
						threadPool->add(task->task);
						if (task->nextPoint != std::chrono::time_point<std::chrono::high_resolution_clock>::max())
						{
							std::chrono::nanoseconds duration = task->nextPoint - task->timePoint;
							task->timePoint = task->nextPoint;
							task->nextPoint += duration;
							nextTime = mineMath::min(nextTime, task->timePoint);
							std::list<Scheduler::TaskUnit>::iterator subTask = task;
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
						nextTime = mineMath::min(nextTime, task->timePoint);
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
	TaskUnit unit;
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

Scheduler::SchedulerUnit Scheduler::addInterval(std::function<void(void)> task, const std::chrono::nanoseconds& duration)
{
	auto id=increment.fetch_add(1);
	auto timePoint = std::chrono::high_resolution_clock::now() + duration;
	auto nextPoint= timePoint + duration;
	unique_lock<mutex> m(_mTaskList);
	add(id, task, timePoint, nextPoint);
	BlockingQueue.notify_one();
	return Scheduler::SchedulerUnit(this, id);
}

Scheduler::SchedulerUnit Scheduler::addTimeOutFor(std::function<void(void)> task, const std::chrono::nanoseconds& duration)
{
	TaskUnit unit;
	auto id = increment.fetch_add(1);
	auto timePoint = std::chrono::high_resolution_clock::now() + duration;
	auto nextPoint = std::chrono::time_point<std::chrono::high_resolution_clock>::max();
	unique_lock<mutex> m(_mTaskList);
	add(id, task, timePoint, nextPoint);
	BlockingQueue.notify_one();
	return Scheduler::SchedulerUnit(this, id);
}

Scheduler::SchedulerUnit Scheduler::addTimeOutUntil(std::function<void(void)> task, const std::chrono::time_point<std::chrono::high_resolution_clock>& timePoint)
{
	TaskUnit unit;
	auto id = increment.fetch_add(1);
	auto nextPoint = std::chrono::time_point<std::chrono::high_resolution_clock>::max();
	unique_lock<mutex> m(_mTaskList);
	add(id, task, timePoint, nextPoint);
	BlockingQueue.notify_one();
	return Scheduler::SchedulerUnit(this, id);
}

Scheduler::SchedulerUnit::SchedulerUnit(Scheduler* scheduler, const uint_fast64_t& id)
	:scheduler(scheduler), id(id)
{
	deleted = false;
}

void Scheduler::SchedulerUnit::deleteUnit()
{
	if (!deleted)
	{
		unique_lock<mutex> m(scheduler->_mTaskList);
		scheduler->deleDeque.push_back(id);
		deleted = true;
	}
}
