#include "Scheduler.h"

#include <iomanip>

using namespace std;
using namespace threadTool;
_Scheduler::_Scheduler(ThreadPool& threadPool)
	:mainAsync(Async<int>(threadPool, [this](AtomicConstReference<bool> loopFlag) {mainService(loopFlag); return 0; }))
{
	unique_lock<mutex> m1(_mDeleDeque);
	unique_lock<mutex> m2(_mTaskList);

	_Scheduler::threadPool = &threadPool;
	increment = 0;
	workFlag = true;
	auto n = threadPool.getMaximumNumberOfThreads();
	if (n < 2)
	{
		cerr << "The maximum number of threads in the thread pool is at least 2, It is currently " << n << ", And it will be automatically set to 2."<< endl << flush;
		threadPool.setMaximumNumberOfThreads(2);
	}
}

_Scheduler::~_Scheduler()
{
	{
		unique_lock<mutex> m(_mTaskList);
		workFlag = false;
		BlockingQueue.notify_all();
	}
	mainAsync.get();
}

void _Scheduler::mainService(AtomicConstReference<bool> loopFlag)
{
	do
	{
		{//ɾ����Ҫɾ��������
			unique_lock<mutex> m(_mDeleDeque);
			for (auto& deleID : deleDeque)
			{
				unique_lock<mutex> m(_mTaskList);
				taskList.remove_if([&deleID](const _TaskUnit& task) {return deleID == task.id; });
			}
			deleDeque.clear();
		}
		{//ִ�����������¶�ʱ
			std::chrono::time_point<std::chrono::high_resolution_clock> nextTime = std::chrono::time_point<std::chrono::high_resolution_clock>::max();
			{
				unique_lock<mutex> m(_mTaskList);
				if (taskList.size() <= 0)
				{
					workFlag = false;
					return;
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
								nextTime = multMath::min(nextTime, task->timePoint);
								std::list<_Scheduler::_TaskUnit>::iterator subTask = task;
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
		}
	} while (workFlag && loopFlag);

}

void _Scheduler::add(const uint_fast64_t& id, std::function<void(void)> task, const std::chrono::time_point<std::chrono::high_resolution_clock>& timePoint, const std::chrono::time_point<std::chrono::high_resolution_clock>& nextPoint)
{//�ڵ��ô������������ڲ�������
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

_Scheduler::_SchedulerUnit _Scheduler::addInterval(std::function<void(void)> task, const std::chrono::nanoseconds& duration)
{
	auto id=increment.fetch_add(1);
	auto timePoint = std::chrono::high_resolution_clock::now() + duration;
	auto nextPoint= timePoint + duration;
	{
		unique_lock<mutex> m(_mTaskList);
		add(id, task, timePoint, nextPoint);
		BlockingQueue.notify_one();
		return _Scheduler::_SchedulerUnit(this, id);
	}
}

_Scheduler::_SchedulerUnit _Scheduler::addTimeOutFor(std::function<void(void)> task, const std::chrono::nanoseconds& duration)
{
	_TaskUnit unit;
	auto id = increment.fetch_add(1);
	auto timePoint = std::chrono::high_resolution_clock::now() + duration;
	auto nextPoint = std::chrono::time_point<std::chrono::high_resolution_clock>::max();
	{
		unique_lock<mutex> m(_mTaskList);
		add(id, task, timePoint, nextPoint);
		BlockingQueue.notify_one();
		return _Scheduler::_SchedulerUnit(this, id);
	}
}

_Scheduler::_SchedulerUnit _Scheduler::addTimeOutUntil(std::function<void(void)> task, const std::chrono::time_point<std::chrono::high_resolution_clock>& timePoint)
{
	_TaskUnit unit;
	auto id = increment.fetch_add(1);
	auto nextPoint = std::chrono::time_point<std::chrono::high_resolution_clock>::max();
	{
		unique_lock<mutex> m(_mTaskList);
		add(id, task, timePoint, nextPoint);
		BlockingQueue.notify_one();
		return _Scheduler::_SchedulerUnit(this, id);
	}
}

_Scheduler::_SchedulerUnit::_SchedulerUnit(_Scheduler* scheduler, const uint_fast64_t& id)
	:scheduler(scheduler), id(id)
{
	deleted = false;
}

_Scheduler::_SchedulerUnit::_SchedulerUnit(const _SchedulerUnit& schedulerUnit)
	: scheduler(schedulerUnit.scheduler), id(schedulerUnit.id), deleted(schedulerUnit.deleted)
{

}

_Scheduler::_SchedulerUnit& _Scheduler::_SchedulerUnit::operator= (const _SchedulerUnit& schedulerUnit)
{
	auto& scheduler0 = scheduler;
	const_cast<_Scheduler*&>(scheduler0) = schedulerUnit.scheduler;
	auto& id0 = id;
	const_cast<uint_fast64_t&>(id0) = schedulerUnit.id;
	deleted = schedulerUnit.deleted;
	return *this;
}

_Scheduler::_SchedulerUnit::_SchedulerUnit()
	: scheduler(nullptr), id(-1)
{
	deleted = true;
}

void _Scheduler::_SchedulerUnit::deleteUnit()
{//todo:ɾ��������Ѿ������̳߳ص����񻹻����ִ�У�
//�����������Դɾ��ǰɾ�������޷��޷��𵽰�ȫ��Ч������δ�뵽�������
	if (!deleted)
	{
		unique_lock<mutex> m(scheduler->_mDeleDeque);
		unique_lock<mutex> m2(scheduler->_mTaskList);
		scheduler->deleDeque.push_back(id);
		deleted = true;
	}
}

std::mutex Scheduler::_m = std::mutex();

map<ThreadPool*, std::shared_ptr<_Scheduler>> Scheduler::schedulers = {};

Scheduler::Scheduler(ThreadPool& threadPool)
	:threadPool(&threadPool)
{
	unique_lock<mutex> m(_m);
	auto unit = schedulers.find(&threadPool);
	if (unit == schedulers.end())
	{
		scheduler = std::make_shared<_Scheduler>(threadPool);
		schedulers.insert(std::make_pair(&threadPool, scheduler));
	}
	else
	{
		scheduler = unit->second;
	}
}

Scheduler::~Scheduler()
{
	scheduler.reset();
	{
		unique_lock<mutex> m(_m);
		auto unit = schedulers.find(const_cast<ThreadPool*>(threadPool));
		if (unit->second.use_count() <= 1)
		{
			schedulers.erase(unit);
		}
	}
}

_Scheduler* Scheduler::operator->()
{
	return scheduler.get();
}
