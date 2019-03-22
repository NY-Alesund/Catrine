/*************************************************************************
	> File Name: Timerqueue.cpp
	> Author: amoscykl
	> Mail: amoscykl@163.com 
 ************************************************************************/

#include "Timerqueue.h"
#include "Channel.h"
#include "Eventloop.h"
#include "Logger.h"
#include "Timer.h"
#include "Util.h"
#include <sys/timerfd.h>
#include <string.h>
#include <unistd.h>
#include <ratio>

TimerQueue::TimerQueue(Eventloop* loop)
	: loop_(loop),
	  timerfd_(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)),
	  timer_channel(std::make_shared<Channel>(timerfd_))
{
	timer_channel->SetReadCallback(std::bind(&TimerQueue::HandleTimerExpired,this));	//设置定时器超时处理事件
	timer_channel->EnableReadEvents();
	loop_->AddChannel(timer_channel);	//添加事件分发器,即在epoller_注册相应事件
}

TimerQueue::~TimerQueue()
{
	Util::Close(timerfd_);
}

void TimerQueue::AddTimer(const std::function<void()>& cb, Timestamp when)
{
	auto timer = std::make_shared<Timer>(cb,when);

	//新的定时器到期时间小于队列中最小的那个的话，则更新timerfd到期时间
	if(timer_queue_.empty() || timer->GetExpTime() < timer_queue_.top()->GetExpTime())
	{
		ResetTimerfd(timerfd_, timer->GetExpTime());
	}

	timer_queue_.push(timer);
}

void TimerQueue::ResetTimerfd(int timerfd, Timestamp when)
{
	//计算多久后到期
	struct timespec ts;
	Nanosecond gap = when - system_clock::now();	//到期时间-当前时间
	ts.tv_sec = static_cast<time_t>(gap.count() / std::nano::den);
	ts.tv_nsec = gap.count() % std::nano::den;

	//更新timerfd剩余到期时间
	struct itimerspec new_value, old_value;
	bzero(&new_value, sizeof(new_value));
	bzero(&old_value, sizeof(old_value));

	new_value.it_value = ts;
	timerfd_settime(timerfd, 0, &new_value, &old_value);
}

//处理定时器超时
void TimerQueue::HandleTimerExpired()
{
	//read the timerfd 
	uint64_t exp_cnt;
	ssize_t n = read(timerfd_, &exp_cnt, sizeof(exp_cnt));
	if(n != sizeof(exp_cnt))
	{
		LOG_ERROR << "read error";
	}

	Timestamp now(system_clock::now());
	//处理所有到期的定时器
	while(!timer_queue_.empty() && timer_queue_.top()->GetExpTime() < now)
	{
		timer_queue_.top()->Run();	//调用回调函数
		timer_queue_.pop();
	}

	//重设timerfd到期时间
	if(!timer_queue_.empty())
	{
		ResetTimerfd(timerfd_,timer_queue_.top()->GetExpTime());	//新的到期时间为定时器小顶堆顶部，即最近到期的定时器
	}
}
