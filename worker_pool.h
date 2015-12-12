#ifndef _WORKER_POOL_H_
#define _WORKER_POOL_H_

//#define  _CRT_SECURE_NO_WARNINGS
// g++ -std=c++11 1.cc -D_GLIBCXX_USE_NANOSLEEP -lpthread */

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
//#include <chrono>

template<typename T>
class WorkerPool
{
public:
	typedef WorkerPool<T> THIS_TYPE;
	typedef std::function<void(T*)> WorkerProc;
	typedef std::vector< std::thread* > ThreadVec;

	WorkerPool()
	{		
		active_ = false;
	}
	virtual ~WorkerPool()
	{
		for(ThreadVec::iterator it = all_thread_.begin();it != all_thread_.end();++it)
			delete *it;
		all_thread_.clear();
	}
	void Start(WorkerProc f,int worker_num=1)
	{
		active_ = true;		
		all_thread_.resize(worker_num);
		for (int i = 0; i < worker_num;i++ )
		{
			all_thread_[i] = new std::thread(std::bind(&THIS_TYPE::consumer,this));
		}
		proc_ = f;
	}
	//生产者
	void Push(T *t)
	{
		std::unique_lock < std::mutex > lck(mutex_);
		task_.push(t);
		cv_.notify_one();
	}

	void Stop()
	{
		//等待所有的任务执行完毕
		mutex_.lock();
		while (!task_.empty())
		{	
			mutex_.unlock();
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			cv_.notify_one();
			mutex_.lock();
		}
		mutex_.unlock();

		//关闭连接后，等待线程自动退出
		active_ = false;
		cv_.notify_all();
		for(ThreadVec::iterator it = all_thread_.begin();
			it != all_thread_.end();++it)
			(*it)->join();
	}
private:
	//消费者
	void consumer()
	{
		//第一次上锁
		std::unique_lock < std::mutex > lck(mutex_);
		while (active_)
		{
			//如果是活动的，并且任务为空则一直等待
			while (active_ && task_.empty())
				cv_.wait(lck);

			//如果已经停止则退出
			if(!active_)
				break;

			T *quest = task_.front();
			task_.pop();

			//从任务队列取出后该解锁(任务队列锁)了
			lck.unlock();

			//执行任务后释放
			proc_(quest);

			//delete quest;   //在proc_已经释放该指针了

			//重新上锁
			lck.lock();
		}
	}

	std::mutex mutex_;
	std::queue<T*> task_;
	std::condition_variable cv_;
	bool active_;
	std::vector< std::thread* > all_thread_;
	WorkerProc proc_;
};

#endif

