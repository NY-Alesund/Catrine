/*************************************************************************
	> File Name: ThreadPool.h
	> Author: amoscykl
	> Mail: amoscykl@163.com 
 ************************************************************************/

#ifndef THREAD_POOL_H
#define THREAD_POOL_H 

#include <vector>
#include <memory>

class Eventloop;
class EventloopThread;

class ThreadPool 
{
public:
	ThreadPool(Eventloop* base_loop, int thread_num);
	~ThreadPool();

	void Start();

	//取出loop消费
	Eventloop* TakeOutLoop();

private:
	//线程池所在的线程
	Eventloop* base_loop_;

	//线程池中线程数量
	int thread_num_;
	//指向线程池中要取出的下一个线程
	int next_;

	std::vector<std::unique_ptr<EventloopThread>> loop_threads_;
	std::vector<Eventloop*> loopers_;
};

#endif // THREAD_POOL_H 
