/*************************************************************************
	> File Name: EventloopThread.cpp
	> Author: amoscykl
	> Mail: amoscykl@163.com 
 ************************************************************************/

#include "EventloopThread.h"
#include "Eventloop.h"

EventloopThread::EventloopThread() 
	: loop_(nullptr),
	//初始化后线程便开始运行,线程函数为ThreadFunc
	thread_(std::bind(&EventloopThread::ThreadFunc, this)),
	mutex_(),
	condition_()
{}

EventloopThread::~EventloopThread()
{
	loop_->Quit();
	thread_.join();	//等待线程结束
}

Eventloop* EventloopThread::GetLoop()
{
	//互斥锁与条件变量配合使用，当新线程运行起来后才能够得到loop的指针
	{	
		//退出此作用域时锁失效
		std::unique_lock<std::mutex> lock(mutex_);
		while (loop_ == nullptr)
		{
			condition_.wait(lock);
		}
	}

	return loop_;
}

void EventloopThread::ThreadFunc()
{
	Eventloop loop;
	{	//退出此作用域时锁就失效,不会造成锁争用
		std::unique_lock<std::mutex> lock(mutex_);	
		loop_ = &loop;
		//得到了loop的指针,通知wait
		condition_.notify_one();
	}

	loop.Start();	//运行新线程
}
