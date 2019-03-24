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
每个loop都有个任务队列
![任务队列](https://github.com/amoscykl98/Catrine/blob/master/image/%E4%BB%BB%E5%8A%A1%E9%98%9F%E5%88%97.png)

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

## 定时器和定时器队列类
Timer类是一个定时器的抽象;
```C++
   void Run() const { callback_(); }	//定时器到期后调用回调
   Timestamp GetExpTime() const { return when_; } //获取定时器到期时间
   
   const std::function<void()> callback_; //定时器超时回调函数
   Timestamp when_;	//定时器到期时间
```

Timerqueue定时器队列
![定时器队列](https://github.com/amoscykl98/Catrine/blob/master/image/%E5%AE%9A%E6%97%B6%E5%99%A8.png)
```C++
   void AddTimer(const std::function<void()>& cb, Timestamp when);	//添加定时器

   void ResetTimerfd(int timerfd, Timestamp when);	//更新定时器到期时间(即最早到期的定时器时间)

   void HandleTimerExpired();		//定时器超时事件处理

   Eventloop* loop_;

   const int timerfd_;	  //定时器描述符
   std::shared_ptr<Channel> timer_channel;	//构造定时器分发器

   //小顶堆实现的定时器队列
   using TimerPtr = std::shared_ptr<Timer>;
   std::priority_queue<TimerPtr, std::vector<TimerPtr>, cmp> timer_queue_;   //定义一个比较器实现一个小顶堆,堆顶为最早到期的定时器
```

其中HandleTimerExpired实现
```C++
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
```

## 事件循环线程类
EventloopThread是事件循环线程构造类,由线程池调用生产事件循环线程; <br>
主要成员:	<br>
```C++
   Eventloop* GetLoop();
   //新线程所运行的函数，即执行Eventloop
   void ThreadFunc();

   Eventloop* loop_;	//
   std::thread thread_;	

   //互斥锁与条件变量配合使用
   std::mutex mutex_;
   std::condition_variable condition_;
```

主要函数:
```C++
   Eventloop* EventloopThread::GetLoop()
   {
	//互斥锁与条件变量配合使用，当新线程运行起来后才能够得到loop的指针
	{	
		std::unique_lock<std::mutex> lock(mutex_);	//锁退出此作用域就失效
		while (loop_ == nullptr)
		{
			condition_.wait(lock);
		}
	}

	return loop_;
   }

   void EventloopThread::ThreadFunc()	//构造函数初始化线程时此线程函数就开始运行
   {
	Eventloop loop;
	{
		std::unique_lock<std::mutex> lock(mutex_);	//锁退出此作用域就失效
		loop_ = &loop;
		//得到了loop的指针,通知wait
		condition_.notify_one();
	}

	loop.Start();	//运行新线程
   }
```


## 线程池
ThreadPool包含两个向量,一个保存事件循环线程,另一个保存loop <br>
主要成员： <br>
```C++
   //线程池所在的线程
   Eventloop* base_loop_;

   //线程池中线程数量
   int thread_num_;
   //指向线程池中要取出的下一个线程
   int next_;

   std::vector<std::unique_ptr<EventloopThread>> loop_threads_;
   std::vector<Eventloop*> loopers_;
};
```
主要函数： <br>
```C++
   //启动线程池，即创建相应数量的线程放入池中
   void ThreadPool::Start()
   {
	for(int i = 0; i < thread_num_; ++i)
	{
		std::unique_ptr<EventloopThread> t(new EventloopThread()); //生产事件循环线程
		loopers_.push_back(t->GetLoop());
		loop_threads_.push_back(std::move(t));
	}
   }

   //取出loop消费，简单的循环取用
   Eventloop* ThreadPool::TakeOutLoop()
   {
	Eventloop* loop = base_loop_;
	if(!loopers_.empty())
	{
		loop = loopers_[next_];
		next_ = (next_ + 1) % thread_num_;
	}

	return loop;
   }
```


