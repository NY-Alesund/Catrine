/*************************************************************************
	> File Name: Timer.h
	> Author: amoscykl
	> Mail: amoscykl@163.com 
 ************************************************************************/

#ifndef TIMER_H
#define TIMER_H 

#include "Timestamp.h"
#include <stdint.h>
#include <functional>

class Timer 
{
public:
	Timer(const std::function<void()>& cb, Timestamp when)
		: callback_(cb),
		  when_(when) {}

	void Run() const { callback_(); }
	Timestamp GetExpTime() const { return when_; }	//获取定时器到期时间

private:
	const std::function<void()> callback_;
	Timestamp when_;
};

#endif //TIMER_H
