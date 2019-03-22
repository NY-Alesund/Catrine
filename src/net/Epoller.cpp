/*************************************************************************
	> File Name: Epoller.cpp
	> Author: amoscykl
	> Mail: amoscykl@163.com 
 ************************************************************************/

#include "Epoller.h"
#include "Channel.h"
#include "Util.h"
#include "Logger.h"

// epoll_wait最多监听事件数
const int EVENTS_NUM = 4096;
// epoll_wait超时时间，这里是10秒
const int EPOLL_WAIT_TIME = 10000;

//创建epoll描述符
Epoller::Epoller()
	: epollfd_(epoll_create1(EPOLL_CLOEXEC)),
	  active_events(EVENTS_NUM)
{
	if(epollfd_ < 0)
	{
		LOG_FATAL << "create epollfd failed";
	}
}

//析构时关闭epoll事件例程
Epoller::~Epoller()
{
	Util::Close(epollfd_);
}

//往epoll的事件表上注册事件
void Epoller::Add(std::shared_ptr<Channel> channel)
{
	//创建epoll_event结构体作为event_ctl的参数
	int fd = channel->GetFd();
	struct epoll_event event;
	event.events = channel->GetEvents();
	event.data.fd = fd;

	//添加进映射表
	ChannelMap[fd] = channel;

	//往内核epoll事件表注册
	if(epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &event) < 0)
	{
		LOG_ERROR << "epoll_ctl add failed";
		//注册失败，从映射表清除
		ChannelMap.erase(fd);
	}
}

//修改注册的事件
void Epoller::Mod(std::shared_ptr<Channel> channel)
{
	int fd = channel->GetFd();
	struct epoll_event event;
	event.events = channel->GetEvents();
	event.data.fd = fd;

	if(epoll_ctl(epollfd_, EPOLL_CTL_MOD, fd, &event) < 0)
	{
		LOG_ERROR << "epoll_ctl mod failed";
		//修改失败,从映射表清除 
		ChannelMap.erase(fd);
	}
}

//删除注册的事件
void Epoller::Del(std::shared_ptr<Channel> channel)
{
	int fd = channel->GetFd();
	struct epoll_event event;
	event.events = channel->GetEvents();
	event.data.fd = fd;

	if(epoll_ctl(epollfd_, EPOLL_CTL_DEL, fd, &event) < 0)
	{
		LOG_ERROR << "epoll_ctl del failed";
	}

	ChannelMap.erase(fd);	//不管epoll_ctl是否失败都清除
}


//等待事件发生
std::vector<std::shared_ptr<Channel>> Epoller::Poll()	//返回一个活跃事件集
{
	//发生的活跃事件将会把epoll_event结构体放到active_events中去
	int active_event_count = epoll_wait(epollfd_, &*active_events.begin(), active_events.size(), EPOLL_WAIT_TIME);

	if(active_event_count < 0)
		LOG_ERROR << "epoll_wait error";

	std::vector<std::shared_ptr<Channel>> activeChannels_;
	for(int i = 0; i < active_event_count; ++i)
	{
		//从映射表中取出Channel
		std::shared_ptr<Channel> channel = ChannelMap[active_events[i].data.fd];
		//设置eventbase的活跃事件
		channel->SetRevents(active_events[i].events);

		activeChannels_.push_back(channel);
	}

	return activeChannels_;
}






