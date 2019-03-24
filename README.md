# Catrine
 A C++ High Performance Network Library
## Introduction
Catrine是一个基于Reactor模式的多线程网络库，附有异步日志，要求Linux 2.6以上内核版本。同时内嵌一个简洁HTTP服务器，可实现路由分发及静态资源访问。
## Feature
* 底层使用Epoll LT模式实现I/O复用，非阻塞I/O
* 多线程、定时器依赖于c++11提供的std::thread库
* Reactor模式，主线程accept请求后，使用Round Robin分发给线程池中线程处理
* 基于小根堆的定时器管理队列
* 双缓冲技术实现的异步日志
* 使用智能指针及RAII机制管理对象生命期
* 使用状态机解析HTTP请求，支持HTTP长连接
* HTTP服务器支持URL路由分发及访问静态资源，可实现RESTful架构
## Envoirment
* OS： Ubuntu 18.04
* Complier： g++ 7.3.0
* Build： CMake

## Build
    
需先安装Cmake：

    $ sudo apt-get update
    $ sudo apt-get install cmake

开始构建

    $ git clone git@github.com:amoscykl98/Catrine.git
    $ cd Catrine
    $ ./build.sh
执行完上述脚本后编译结果在新生成的build文件夹内，示例程序在build/bin下。

库和头文件分别安装在/usr/local/lib和/usr/local/include，该库依赖c++11及pthread库，使用方法如下：

    $ g++ main.cpp -std=c++11 -lCatrine -lpthread

在执行生成的可执行文件时可能会报找不到动态库文件，需要添加动态库查找路径，首先打开配置文件：

    $ sudo vim /etc/ld.so.conf

在打开的文件末尾添加这句，保存退出：

    include /usr/local/lib

使修改生效：

    $ sudo /sbin/ldconfig
    
    
    
# 源码剖析

## Channel类
channel是事件分发器类; <br>
主要成员：<br>
```C++
   //channel负责的文件描述符
   int fd_;	
   //关注的事件
   int events_;
   //epoller返回的活跃事件
   int revents_;
```
主要作用是： <br>
  * 首先绑定Channel要处理的fd <br>
  * 注册fd上需要监听的事件，如果是常用事件(读写等)的话，直接调用接口 enable*** 来注册对应fd上的事件，与之对应的是 disable*** 用来销毁特定事件 <br>
  * 通过 set_callback来设置事件发生时的回调函数 <br>
  * 分发事件处理HandleEvent函数，根据revents_返回的活跃事件调用相应事件处理函数，由Eventloop调用 <br>
 

## Eventloop类
Eventloop是事件循环类; <br>
![事件循环类](https://github.com/amoscykl98/Catrine/blob/master/image/Eventloop.png)

```C++
	  
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
   
   //添加更改删除事件分发器,并在epoller_注册相应事件
   void AddChannel(std::shared_ptr<Channel> channel) { epoller_->Add(channel); }
   void ModChannel(std::shared_ptr<Channel> channel) { epoller_->Mod(channel); }
   void DelChannel(std::shared_ptr<Channel> channel) { epoller_->Del(channel); }


   //定时器队列
   std::unique_ptr<TimerQueue> timerQueue_;
   
   //任务队列
   moodycamel::ConcurrentQueue<Task> task_queue_;

   //添加任务到任务队列
   void Eventloop::AddTask(Task&& task);

     
   //执行任务队列
   void Eventloop::HandleTask()
```

## Epoller类
Epoller类是IO复用类;
```C++
   //往epoll的事件表上注册事件,内部调用epoll_ctl
   void Add(std::shared_ptr<Channel> channel);
   //修改注册的事件
   void Mod(std::shared_ptr<Channel> channel);
   //删除注册的事件
   void Del(std::shared_ptr<Channel> channel);

   //等待事件发生
   std::vector<std::shared_ptr<Channel>> Poll();
	
   //epoll的文件描述符，用来唯一标识内核中的epoll事件表:epoll例程
   int epollfd_;

   //传给epoll_wait的参数，epoll_wait将会把发生的事件写入到这个列表中返回给用户
   std::vector<epoll_event> active_events;

   //fd到Channel事件分发器的映射，可以根据fd来得到对应的Channel
   std::map<int,std::shared_ptr<Channel>> ChannelMap;
```

重点是Poll函数

![Poll](https://github.com/amoscykl98/Catrine/blob/master/image/poll.png)
![Poll](https://github.com/amoscykl98/Catrine/blob/master/image/epoll.png)


```C++
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
```

