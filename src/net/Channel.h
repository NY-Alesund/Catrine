/*************************************************************************
	> File Name: Channel.h
	> Author: amoscykl
	> Mail: amoscykl@163.com 
 ************************************************************************/

#ifndef CHANNEL_H
#define CHANNEL_H 

#include "Timestamp.h"
#include <sys/epoll.h>
#include <functional>


//	Channel的主要作用：
//		1.首先给定Channel所属的loop以及其要处理的fd
//		2.接着开始注册fd上需要监听的事件，如果是常用事件(读写等)的话，我们可以直接调用接口enable**来注册对应fd上的事件,
//		  与之对应的是disable**用来销毁特定的事件
//		3.然后通过set***Callback来事件发生时的回

class Channel 
{
public:
	// c++11
	using EventCallback = std::function<void()>;	//事件回调
	using ReadEventCallback = std::function<void(Timestamp)>;	//读事件回调

	Channel(int fd);
	~Channel();

	int GetFd() const { return fd_; }
	int GetEvents() const { return events_; }
	
	//关注可读事件
	void EnableReadEvents() { events_ |= (EPOLLIN | EPOLLPRI); }
	//关注可写事件
	void EnableWriteEvents() { events_ |= EPOLLOUT; }
	
	//销毁可读事件
	void DisableReadEvents() { events_ &= ~(EPOLLIN | EPOLLPRI); }
	//销毁可写事件
	void DisableWriteEvents() { events_ &= ~EPOLLOUT; }
	

	//是否注册可写事件
	bool IsWriting() const { return events_ & EPOLLOUT; }
	//是否注册可读事件
	bool IsReading() const { return events_ & (EPOLLIN | EPOLLPRI); }

	//分发事件处理,由Eventloop调用
	void HandleEvent();

	//设置相应的事件回调函数
	void SetReadCallback(ReadEventCallback&& cb) { readcallback_ = cb; }
	void SetWriteCallback(EventCallback&& cb) { writecallback_ = cb; }
	void SetCloseCallback(EventCallback&& cb) { closecallback_ = cb; }
	void SetErrorCallback(EventCallback&& cb) { errorcallback_ = cb; }

	//设置返回的活跃事件,由epoller调用
	void SetRevents(int revents) { revents_ = revents; }


private:
	//channel负责的文件描述符
	int fd_;	
	//关注的事件
	int events_;
	//epoller返回的活跃事件
	int revents_;

	//事件处理函数
	ReadEventCallback readcallback_;
	EventCallback writecallback_;
	EventCallback closecallback_;
	EventCallback errorcallback_;
	

};

#endif  //CHANNEL_H
