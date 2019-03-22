/*************************************************************************
	> File Name: Eventloop.cpp
	> Author: amoscykl
	> Mail: amoscykl@163.com 
 ************************************************************************/

#include "Eventloop.h"
#include "Channel.h"
#include "Timerqueue.h"
#include "Logger.h"
#include <sys/eventfd.h>
#include <unistd.h>

Eventloop::Eventloop()	//初始化事件循环
	: quit_(false),
	  eventHanding_(false),
	  wakeup_fd_(eventfd(0,EFD_NONBLOCK | EFD_CLOEXEC)),
	  wakeupChannel_(std::make_shared<Channel>(wakeup_fd_)),
	  threadid_(std::this_thread::get_id()),
	  epoller_(new Epoller()),
	  timerQueue_(new TimerQueue(this))
{
	if(wakeup_fd_ < 0)
	{
		LOG_FATAL << "create wakeup_fd_ failed";
	}

	//注册唤醒的可读事件
	wakeupChannel_->SetReadCallback(std::bind(&Eventloop::HandleWakeUp,this));
	wakeupChannel_->EnableReadEvents();

	//注册到epoller上
	AddChannel(wakeupChannel_);
}

Eventloop::~Eventloop() {}

//开始事件循环
void Eventloop::Start()
{
	std::vector<std::shared_ptr<Channel>> activeChannels_;	//活跃的事件集

	while(!quit_)
	{
		activeChannels_.clear();
		activeChannels_ = epoller_->Poll();	//Poll返回活跃的事件集
		for (auto& it : activeChannels_)
		{
			it->HandleEvent();		//处理活跃事件集的事件
		}
		HandleTask();	//处理一些其它的任务
	}
}

//唤醒循环，由其它线程调用
void Eventloop::WakeUp()
{
	uint64_t one = 1;
	ssize_t n = write(wakeup_fd_, &one, sizeof(one));
	if (n != sizeof(one))
	{
		LOG_ERROR << "wake up error in write()";
	}
}

//唤醒事件处理函数
void Eventloop::HandleWakeUp()
{
	uint64_t one = 1;
	ssize_t n = read(wakeup_fd_, &one, sizeof(one));
	if(n != sizeof(one))
	{
		LOG_ERROR << "handle wake up error in read()";
	}
}

//运行Task，可由其它线程调用
void Eventloop::RunTask(Task&& task)
{
	//如果不是在自己所在线程调用，则添加进任务队列后唤醒该loop进行处理
	if(IsInBaseThread())
		task();
	else 
		AddTask(std::move(task));	//添加其它线程的任务到任务队列，并在事件循环中处理。
}

//添加任务
void Eventloop::AddTask(Task&& task)
{
	task_queue_.enqueue(std::move(task));
	if(!IsInBaseThread() || eventHanding_)
	{
		WakeUp();	//唤醒循环处理任务
	}
}

//执行任务队列
void Eventloop::HandleTask()
{
	eventHanding_ = true;
	Task task;
	while(task_queue_.try_dequeue(task))
	{
		task();
	}
	eventHanding_ = false;
}

//一定时间后执行该任务,只能在Eventloop所在线程调用
void Eventloop::RunTaskAfter(Task&& task, Nanosecond interval)
{
	timerQueue_->AddTimer(task,system_clock::now() + interval);
}
