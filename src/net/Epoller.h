/*************************************************************************
	> File Name: Epoller.h
	> Author: amoscykl
	> Mail: amoscykl@163.com 
 ************************************************************************/

#ifndef EPOLLER_H
#define EPOLLER_H 

#include <sys/epoll.h>
#include <vector>
#include <map>
#include <memory>

//向前声明
class Channel;

class Epoller 
{
public:
	Epoller();
	~Epoller();

	//往epoll的事件表上注册事件
	void Add(std::shared_ptr<Channel> channel);
	//修改注册的事件
	void Mod(std::shared_ptr<Channel> channel);
	//删除注册的事件
	void Del(std::shared_ptr<Channel> channel);

	//等待事件发生
	std::vector<std::shared_ptr<Channel>> Poll();

private:
	//epoll的文件描述符，用来唯一标识内核中的epoll事件表:epoll例程
	int epollfd_;

	//传给epoll_wait的参数，epoll_wait将会把发生的事件写入到这个列表中返回给用户
	std::vector<epoll_event> active_events;

	//fd到Channel事件分发器的映射，可以根据fd来得到对应的Channel
	std::map<int,std::shared_ptr<Channel>> ChannelMap;
};

#endif //EPOLLER_H
