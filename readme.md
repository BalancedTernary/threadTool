�����ռ�threadTool����Ҫ���������󹤾�(��ҪC++17)
1.ThreadPool��
2.Scheduler��
3.Async��

1.ThreadPool�ࣺ
	һ���ɻ����̳߳أ���[Scheduler�࣬Async��]�Ļ�����
    ��������˳���첢��ʼ��
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

    ��ʼ���󼴿�ʹ��add����Ϊ�̳߳��������
        add��������[void(void),void(const volatile std::atomic<volatile bool>&)]���ֺ���
        �ڶ��ֺ������յĲ���Ϊһԭ�ӵ�bool�������ã���Ϊfalseʱ��ʾ�̳߳�׼��������
        �ڲ�����ɾݴ���������

    ʹ��join��������ʽ�ȴ��̳߳�����ȫ�����

    �̳߳�����ʱ��ȴ����ڲ�ȫ������ִ�����


2.Scheduler�ࣺ
    һ����ʱ���񷢷�����������ָ��ʱ�̡�ָ��ʱ���������ԵĽ�ָ��������뵽�̳߳أ�
    ����ʱ�贫��һ���̳߳أ������ÿ�������ڶ�Ӧ���̳߳�������һ����פ�ķ����̣߳��������񷢷š�
    ��ȷ���̳߳��߳����㹻.

    _SchedulerUnit addInterval(std::function<void(void)> task, const std::chrono::nanoseconds& duration);
        //���������趨����������

	_SchedulerUnit addTimeOutFor(std::function<void(void)> task, const std::chrono::nanoseconds& duration);
        //�����趨��һ��ʱ��󷢷ŵ�����

	_SchedulerUnit addTimeOutUntil(std::function<void(void)> task, const std::chrono::time_point<std::chrono::high_resolution_clock>& timePoint);
        //�����趨��ָ��ʱ��㷢�ŵ�����

    �趨����󷵻�һ��_SchedulerUnit���󣬿�ͨ�����ô˶����deleteUnit����ɾ��������

3.Async�ࣺ
    ���Է�������̳߳�����Ӽ򵥵��첽���񡣹���ʱ�����̳߳غͺ���������������Զ���ʼִ�С�
    ����ͨ��check()������������Ƿ�ִ����ɣ�����True��������ִ����ɡ�
    ����ͨ��get()������ȡ�������ķ���ֵ���������ʱ����δִ�������������
    ����ͨ��reRun()��������ִ������