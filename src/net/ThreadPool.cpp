/*************************************************************************
	> File Name: ThreadPool.cpp
	> Author: amoscykl
	> Mail: amoscykl@163.com 
 ************************************************************************/

#include "ThreadPool.h"
#include "Eventloop.h"
#include "EventloopThread.h"

ThreadPool::ThreadPool(Eventloop* base_loop, int thread_num)
	: base_loop_(base_loop),
	  thread_num_(thread_num),
	  next_(0)
{}

ThreadPool::~ThreadPool() {}

//启动线程池，即创建相应数量的线程放入池中
void ThreadPool::Start()
{
	for(int i = 0; i < thread_num_; ++i)
	{
		std::unique_ptr<EventloopThread> t(new EventloopThread());
		loopers_.push_back(t->GetLoop());
		loop_threads_.push_back(std::move(t));
	}
}

//取出loop消费，简单的循环取用
Eventloop* ThreadPool::TakeOutLoop()
{
	Eventloop* loop = base_loop_;
	if(!loopers_.empty())
	{
		loop = loopers_[next_];
		next_ = (next_ + 1) % thread_num_;
	}

	return loop;
}
