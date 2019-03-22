/*************************************************************************
	> File Name: Timerqueue.h
	> Author: amoscykl
	> Mail: amoscykl@163.com 
 ************************************************************************/

#ifndef TIMERQUEUE_H
#define TIMERQUEUE_H 

#include "Timer.h"
#include <stdint.h>
#include <vector>
#include <functional>
#include <queue>
#include <memory>

class Channel;
class Eventloop;

//比较器
struct cmp	//自定义比较器从而构建小顶堆
{
	bool operator()(const std::shared_ptr<Timer>& lhs, const  std::shared_ptr<Timer>& rhs)
	{
		return lhs->GetExpTime() > rhs->GetExpTime();
	}
};

class TimerQueue 
{
public:
	TimerQueue(Eventloop* loop);
	~TimerQueue();

	void AddTimer(const std::function<void()>& cb, Timestamp when);

private:
	void ResetTimerfd(int timerfd, Timestamp when);	//更新定时器到期时间(即最早到期的定时器时间)

	void HandleTimerExpired();		//定时器超时事件处理

	Eventloop* loop_;

	const int timerfd_;			//定时器描述符
	std::shared_ptr<Channel> timer_channel;		//定时器分发器

	//小顶堆实现的定时器队列
	using TimerPtr = std::shared_ptr<Timer>;
	std::priority_queue<TimerPtr, std::vector<TimerPtr>, cmp> timer_queue_;
};

#endif // TIMERQUEUE_H 
