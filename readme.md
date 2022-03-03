## 命名空间threadTool中主要包含了三大工具(需要C++17)  
### 1.ThreadPool类  
### 2.Scheduler类  
### 3.Async类  
### 4.MessageLimiter类  

### 1.ThreadPool类：
	一个可缓存线程池，是[Scheduler类，Async类]的基础。  
    请依如下顺序构造并初始化  
``` cpp  
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
```
    初始化后即可使用add函数为线程池添加任务  
        add函数接受[void(void),void(const volatile std::atomic<volatile bool>&)]两种函数  
        第二种函数接收的参数为一原子的bool类型引用，其为false时表示线程池准备析构。  
        内部任务可据此主动结束  

    使用join函数可显式等待线程池任务全部完成  

    线程池析构时会等待其内部全部任务执行完成  

    可使用GlobalThreadPool::get()获取全局默认线程池  

    同一线程池有任务互斥时，注意防止浪费线程或因任务添加顺序死锁。  


### 2.Scheduler类：
    一个定时任务发放器，可以在指定时刻、指定时间后或周期性的将指定任务加入到线程池，
    构造时需传入一个线程池。不传入时使用全局默认线程池。多次在一个线程池上构造该对象，本质上是获取了同一个对象的引用。
    。此类的每个对象将在对应的线程池中设置一个常驻的服务线程，用于任务发放。
    请确保线程池线程数足够.
``` cpp
    _SchedulerUnit addInterval(std::function<void(void)> task, const std::chrono::nanoseconds& duration);
        //函数用于设定周期性任务

	_SchedulerUnit addTimeOutFor(std::function<void(void)> task, const std::chrono::nanoseconds& duration);
        //用于设定在一段时间后发放的任务

	_SchedulerUnit addTimeOutUntil(std::function<void(void)> task, const std::chrono::time_point<std::chrono::high_resolution_clock>& timePoint);
        //用于设定在指定时间点发放的任务
```
    设定任务后返回一个_SchedulerUnit对象，可通过调用此对象的deleteUnit方法删除该任务。  
    删除操作不等待已运行的任务结束。  
    在释放资源前需要等待已运行的任务结束，请在deleteUnit后调用join()。  

### 3.Async类：
    可以方便的在线程池中添加简单的异步任务。构造时传入线程池和函数，构造后任务自动开始执行。  

    可以通过check()函数检查任务是否执行完成，返回True则任务已执行完成。  
    可以通过get()函数获取任务函数的返回值，如果调用时函数未执行完毕则阻塞。  
    可以通过reRun()函数重新执行任务。  

### 4.MessageLimiter类：
    该类是消息限流器，可以防止事件回调被高频触发。 
    
    构造函数MessageLimiter(_T value = 0, const Mode& mode = CONTINUE, const std::chrono::nanoseconds& period = std::chrono::milliseconds(100), std::function<bool(const _T& newValue, const _T& oldValue)> condition = [](const _T& newValue, const _T& oldValue) {return false; }, threadTool::ThreadPool& threadPool = threadTool::GlobalThreadPool::get());  
		_T为限流器内部处理使用的值的类型，需要能够使用==运算符判断相等  
        value为消息的初始值  
        mode有如下四种选择（枚举）  
            ”IMMEDIATE“: 立即触发；  
	        “CONTINUE”: 连续值模式，类似于节流，可与直通条件配合；  
	        ”DISPERSE“: 离散值模式，类似于防抖，可与直通条件配合；  
	        “FILTER”: 过滤器模式，仅当直通条件为TRUE时才使消息通过；  
        period为工作周期，  
            对于连续值模式，消息间隔大于工作周期时消息将直通；  
            对于离散值模式，间隔大于工作周期的“值的变化”将直通；  
        condition为直通条件，接收两个const _T&参数，分别为这次接收到的值和上次通过的值，返回bool值  
            对于连续值模式，condition返回值为true时，消息直接通过，否则进入类节流逻辑  
            对于离散值模式，消息发生变化且condition返回值为true时，消息直接通过，否则进入类防抖逻辑  
        threadPool为工作使用的线程池，不填将使用全局默认线程池。  
    
    通过void regist(std::function<void(const _T&, const _Args&...)> callback)注册消息回调函数。  
        args为可选附加参数组，不参与逻辑  

    通过void regist(const std::string& key, std::function<void(const _T&, const _Args&...)> callback)在key上注册消息回调函数  
        一个key上可以注册多个回调函数  

    通过void unregist(const std::string& key)删除注册在key上的回调函数  

    通过void sendMessage(const _T& value, const _Args&... parameters)发送消息  
        必须保证parameter的数量和类型和注册的回调函数接受的一致，如不一致则抛出std::bad_any_cast异常  

    同一对象内，所有函数的_T应保持一致，否则会触发std::bad_any_cast异常  
    