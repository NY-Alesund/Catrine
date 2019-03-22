/*************************************************************************
	> File Name: Channel.cpp
	> Author: amoscykl
	> Mail: amoscykl@163.com 
 ************************************************************************/

#include "Channel.h"
#include "Logger.h"

Channel::Channel(int fd)
	: fd_(fd),
	  events_(EPOLLRDHUP),
	  revents_(0)
{
}

Channel::~Channel() {}


void Channel::HandleEvent()
{
	// 异常事件
	if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) 
	{
		if(closecallback_)
			closecallback_();
		return;
	}

	//关闭连接
	if(revents_ & EPOLLRDHUP) 
	{
		if(closecallback_)
			closecallback_();
		return;
	}

	//发生错误
	if(revents_ & EPOLLERR)
	{
		if(errorcallback_)
			errorcallback_();
		if(closecallback_)
			closecallback_();
		return;
	}

	//可读事件
	if(revents_ & (EPOLLIN | EPOLLPRI | EPOLLHUP))
	{
		if(readcallback_)
			readcallback_(system_clock::now());
	}

	//可写事件
	if(revents_ & EPOLLOUT)
	{
		if(writecallback_)
			writecallback_();
	}
}
