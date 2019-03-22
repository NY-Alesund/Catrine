/*************************************************************************
	> File Name: Eventloopthread.h
	> Author: amoscykl
	> Mail: amoscykl@163.com 
 ************************************************************************/

#ifndef EVENTLOOPTHREAD_H
#define EVENTLOOPTHREAD_H 

#include <thread>
#include <mutex>
#include <condition_variable>

class Eventloop;

class EventloopThread 
{
public:
	EventloopThread();
	~EventloopThread();

	Eventloop* GetLoop();

private:
	//新线程所运行的函数，即执行Eventloop
	void ThreadFunc();

	Eventloop* loop_;
	std::thread thread_;

	//互斥锁与条件变量配合使用
	std::mutex mutex_;
	std::condition_variable condition_;
};

#endif // EVENTLOOPTHREAD_H
