## �����ռ�threadTool����Ҫ���������󹤾�(��ҪC++17)  
### 1.ThreadPool��  
### 2.Scheduler��  
### 3.Async��  
### 4.MessageLimiter��  

### 1.ThreadPool�ࣺ
	һ���ɻ����̳߳أ���[Scheduler�࣬Async��]�Ļ�����  
    ��������˳���첢��ʼ��  
``` cpp  
	ThreadPool tp; 
        //�����̳߳ض���

    tp.setIdleLife(std::chrono::seconds(1)); 
        //�趨�̵߳Ŀ���������ÿ����ʱ������һ�������߳�

    tp.setMaximumNumberOfThreads(200); 
        //�趨�̳߳�����߳�����
        //�ڳ�����û�ж���̳߳�ͬʱ����ʱ��
        //����ֵ�趨Ϊ��������߼������������ɻ��������ܡ�

    tp.setMinimumNumberOfThreads(5);
        //�趨�̳߳������߳�����
        //��ʹ�̳߳���û������Ҳ���������Ŀ���߳�
        //����ȡmin(��������������,���ܻ�ͬʱ�������̳߳��ڵ��ӳ������������Ԥ������)

    tp.setRedundancyRatio(1.5);
        //�趨�߳�����ϵ��
        //���߳�������������ʱ�������߳���Ϊ��Ծ�߳����Ĵ˱���
        //�����ܴ���1��ֵ
```
    ��ʼ���󼴿�ʹ��add����Ϊ�̳߳��������  
        add��������[void(void),void(const volatile std::atomic<volatile bool>&)]���ֺ���  
        �ڶ��ֺ������յĲ���Ϊһԭ�ӵ�bool�������ã���Ϊfalseʱ��ʾ�̳߳�׼��������  
        �ڲ�����ɾݴ���������  

    ʹ��join��������ʽ�ȴ��̳߳�����ȫ�����  

    �̳߳�����ʱ��ȴ����ڲ�ȫ������ִ�����  

    ��ʹ��GlobalThreadPool::get()��ȡȫ��Ĭ���̳߳�  

    ͬһ�̳߳������񻥳�ʱ��ע���ֹ�˷��̻߳����������˳��������  


### 2.Scheduler�ࣺ
    һ����ʱ���񷢷�����������ָ��ʱ�̡�ָ��ʱ���������ԵĽ�ָ��������뵽�̳߳أ�
    ����ʱ�贫��һ���̳߳ء�������ʱʹ��ȫ��Ĭ���̳߳ء������һ���̳߳��Ϲ���ö��󣬱������ǻ�ȡ��ͬһ����������á�
    �������ÿ�������ڶ�Ӧ���̳߳�������һ����פ�ķ����̣߳��������񷢷š�
    ��ȷ���̳߳��߳����㹻.
``` cpp
    _SchedulerUnit addInterval(std::function<void(void)> task, const std::chrono::nanoseconds& duration);
        //���������趨����������

	_SchedulerUnit addTimeOutFor(std::function<void(void)> task, const std::chrono::nanoseconds& duration);
        //�����趨��һ��ʱ��󷢷ŵ�����

	_SchedulerUnit addTimeOutUntil(std::function<void(void)> task, const std::chrono::time_point<std::chrono::high_resolution_clock>& timePoint);
        //�����趨��ָ��ʱ��㷢�ŵ�����
```
    �趨����󷵻�һ��_SchedulerUnit���󣬿�ͨ�����ô˶����deleteUnit����ɾ��������  
    ɾ���������ȴ������е����������  
    ���ͷ���Դǰ��Ҫ�ȴ������е��������������deleteUnit�����join()��  

### 3.Async�ࣺ
    ���Է�������̳߳�����Ӽ򵥵��첽���񡣹���ʱ�����̳߳غͺ���������������Զ���ʼִ�С�  

    ����ͨ��check()������������Ƿ�ִ����ɣ�����True��������ִ����ɡ�  
    ����ͨ��get()������ȡ�������ķ���ֵ���������ʱ����δִ�������������  
    ����ͨ��reRun()��������ִ������  

### 4.MessageLimiter�ࣺ
    ��������Ϣ�����������Է�ֹ�¼��ص�����Ƶ������ 
    
    ���캯��MessageLimiter(_T value = 0, const Mode& mode = CONTINUE, const std::chrono::nanoseconds& period = std::chrono::milliseconds(100), std::function<bool(const _T& newValue, const _T& oldValue)> condition = [](const _T& newValue, const _T& oldValue) {return false; }, threadTool::ThreadPool& threadPool = threadTool::GlobalThreadPool::get());  
		_TΪ�������ڲ�����ʹ�õ�ֵ�����ͣ���Ҫ�ܹ�ʹ��==������ж����  
        valueΪ��Ϣ�ĳ�ʼֵ  
        mode����������ѡ��ö�٣�  
            ��IMMEDIATE��: ����������  
	        ��CONTINUE��: ����ֵģʽ�������ڽ���������ֱͨ������ϣ�  
	        ��DISPERSE��: ��ɢֵģʽ�������ڷ���������ֱͨ������ϣ�  
	        ��FILTER��: ������ģʽ������ֱͨ����ΪTRUEʱ��ʹ��Ϣͨ����  
        periodΪ�������ڣ�  
            ��������ֵģʽ����Ϣ������ڹ�������ʱ��Ϣ��ֱͨ��  
            ������ɢֵģʽ��������ڹ������ڵġ�ֵ�ı仯����ֱͨ��  
        conditionΪֱͨ��������������const _T&�������ֱ�Ϊ��ν��յ���ֵ���ϴ�ͨ����ֵ������boolֵ  
            ��������ֵģʽ��condition����ֵΪtrueʱ����Ϣֱ��ͨ�����������������߼�  
            ������ɢֵģʽ����Ϣ�����仯��condition����ֵΪtrueʱ����Ϣֱ��ͨ�����������������߼�  
        threadPoolΪ����ʹ�õ��̳߳أ����ʹ��ȫ��Ĭ���̳߳ء�  
    
    ͨ��void regist(std::function<void(const _T&, const _Args&...)> callback)ע����Ϣ�ص�������  
        argsΪ��ѡ���Ӳ����飬�������߼�  

    ͨ��void regist(const std::string& key, std::function<void(const _T&, const _Args&...)> callback)��key��ע����Ϣ�ص�����  
        һ��key�Ͽ���ע�����ص�����  

    ͨ��void unregist(const std::string& key)ɾ��ע����key�ϵĻص�����  

    ͨ��void sendMessage(const _T& value, const _Args&... parameters)������Ϣ  
        ���뱣֤parameter�����������ͺ�ע��Ļص��������ܵ�һ�£��粻һ�����׳�std::bad_any_cast�쳣  

    ͬһ�����ڣ����к�����_TӦ����һ�£�����ᴥ��std::bad_any_cast�쳣  
    