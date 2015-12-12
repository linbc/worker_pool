#include "worker_pool.h"
#include <iostream>

//为了多耗点cpu，计算斐波那契数列吧
static int fibonacci(int a)
{
	//ASSERT(a > 0);
	if (a == 1 || a == 2)
		return 1;
	return fibonacci(a-1) + fibonacci(a-2);
}

//异步计算任务
struct AsyncCalcQuest
{
	AsyncCalcQuest():num(0),result(0)
	{}
	//计算需要用到的变量
	int num;
	int result; 
};

//为了测试方便,引入全局变量用于标识线程池已将所有计算完成
const int TOTAL_COUNT = 1000000;
int now_count = 0;

//继承一下线程池类,在子类处理计算完成的业务，在我们这里，只是打印一下计算结果
class CalcWorkerPool:public WorkerPool<AsyncCalcQuest>
{
public:
	CalcWorkerPool(){}

	virtual ~CalcWorkerPool()
	{
	}

	//在工人线程中执行
	void DoWork(AsyncCalcQuest *quest)
	{
		quest->result = fibonacci(quest->num);	

		//并将已完成任务返回到准备回调的列表
		std::unique_lock<std::mutex > lck(mutex_callbacks_);
		callbacks_.push_back(quest);
	}

	//在主线程执行
	void DoCallback()
	{
		//组回调任务上锁
		std::unique_lock<std::mutex > lck(mutex_callbacks_);
		while (!callbacks_.empty())
		{
			auto *quest = callbacks_.back();			
			{//此处为业务代码打印一下吧
				std::cout << quest->num << " " << quest->result << std::endl;
				now_count ++;
			}
			delete quest;		//TODO:这里如果采用内存池就更好了
			callbacks_.pop_back();
		}
	}

private:
	//这里是准备给回调的任务列表
	std::vector<AsyncCalcQuest*> callbacks_;
	std::mutex mutex_callbacks_;
};

int main()
{
	CalcWorkerPool workers;

	//工厂开工了 8个工人喔
	workers.Start(std::bind(&CalcWorkerPool::DoWork,&workers,std::placeholders::_1),8);	
	
	//开始产生任务了
	for (int i=0; i<TOTAL_COUNT; i++)
	{
		AsyncCalcQuest *quest = new AsyncCalcQuest;
		quest->num = i%40+1;
		workers.Push(quest);
	}

	while (now_count != TOTAL_COUNT)
	{
		workers.DoCallback();
	}

	workers.Stop();

    return 0;
}

