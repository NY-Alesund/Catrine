/*************************************************************************
	> File Name: Eventloop.h
	> Author: amoscykl
	> Mail: amoscykl@163.com 
 ************************************************************************/

#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include "Timestamp.h"
#include "Epoller.h"
#include "Concurrentqueue.h"
#include <vector>
#include <memory>
#include <functional>
#include <mutex>
#include <thread>

class Channel;
class TimerQueue;

class Eventloop 
{
public:
	using Task = std::function<void()>;

	Eventloop();
	~Eventloop();

	//开始事件循环
	void Start();
	//退出循环
	void Quit() { quit_ = true; }

	//添加更改删除事件分发器,并在epoller_注册相应事件
	void AddChannel(std::shared_ptr<Channel> channel) { epoller_->Add(channel); }
	void ModChannel(std::shared_ptr<Channel> channel) { epoller_->Mod(channel); }
	void DelChannel(std::shared_ptr<Channel> channel) { epoller_->Del(channel); }

	//唤醒事件循环以处理Task,配合Handle使用
	void WakeUp();
	void HandleWakeUp();

	void RunTask(Task&& task);
	void AddTask(Task&& task);
	void HandleTask();

	void RunTaskAfter(Task&& task, Nanosecond interval);
	
	bool IsInBaseThread() const { return threadid_ == std::this_thread::get_id(); }

private:
	//是否运行标识
	bool looping_;
	//是否退出事件循环标识
	bool quit_;
	//是否正在处理事件标识
	bool eventHanding_;

	//唤醒描述符
	int wakeup_fd_;
	//封装唤醒描述符的Channel类
	std::shared_ptr<Channel> wakeupChannel_;

	//运行loop的线程ID
	const std::thread::id threadid_;

	std::unique_ptr<Epoller> epoller_;

	//定时器队列
	std::unique_ptr<TimerQueue> timerQueue_;

	//任务队列
	moodycamel::ConcurrentQueue<Task> task_queue_;
};

#endif // EVENTLOOP_H 


