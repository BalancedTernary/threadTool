命名空间threadTool中主要包含了三大工具(需要C++17)
1.ThreadPool类
2.Scheduler类
3.Async类

1.ThreadPool类：
	一个可缓存线程池，是[Scheduler类，Async类]的基础。
    请依如下顺序构造并初始化
	ThreadPool tp; 
        //创建线程池对象

    tp.setIdleLife(std::chrono::seconds(1)); 
        //设定线程的空闲寿命，每隔此时间消灭一个空闲线程

    tp.setMaximumNumberOfThreads(200); 
        //设定线程池最大线程数，
        //在程序中没有多个线程池同时存在时，
        //将此值设定为计算机的逻辑处理器个数可获得最佳性能。

    tp.setMinimumNumberOfThreads(5);
        //设定线程池最少线程数，
        //即使线程池内没有任务，也会包含此数目的线程
        //建议取min(计算机物理核心数,可能会同时存在于线程池内的延迟敏感型任务的预估数量)

    tp.setRedundancyRatio(1.5);
        //设定线程冗余系数
        //在线程数不超过上限时，保持线程数为活跃线程数的此倍。
        //仅接受大于1的值

    初始化后即可使用add函数为线程池添加任务
        add函数接受[void(void),void(const volatile std::atomic<volatile bool>&)]两种函数
        第二种函数接收的参数为一原子的bool类型引用，其为false时表示线程池准备析构。
        内部任务可据此主动结束

    使用join函数可显式等待线程池任务全部完成

    线程池析构时会等待其内部全部任务执行完成


2.Scheduler类：
    一个定时任务发放器，可以在指定时刻、指定时间后或周期性的将指定任务加入到线程池，
    构造时需传入一个线程池，此类的每个对象将在对应的线程池中设置一个常驻的服务线程，用于任务发放。
    请确保线程池线程数足够.

    _SchedulerUnit addInterval(std::function<void(void)> task, const std::chrono::nanoseconds& duration);
        //函数用于设定周期性任务

	_SchedulerUnit addTimeOutFor(std::function<void(void)> task, const std::chrono::nanoseconds& duration);
        //用于设定在一段时间后发放的任务

	_SchedulerUnit addTimeOutUntil(std::function<void(void)> task, const std::chrono::time_point<std::chrono::high_resolution_clock>& timePoint);
        //用于设定在指定时间点发放的任务

    设定任务后返回一个_SchedulerUnit对象，可通过调用此对象的deleteUnit方法删除该任务。

3.Async类：
    可以方便的在线程池中添加简单的异步任务。构造时传入线程池和函数，构造后任务自动开始执行。
    可以通过check()函数检查任务是否执行完成，返回True则任务已执行完成。
    可以通过get()函数获取任务函数的返回值，如果调用时函数未执行完毕则阻塞。
    可以通过reRun()函数重新执行任务。