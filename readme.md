命名空间threadTool中主要包含了三大工具
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