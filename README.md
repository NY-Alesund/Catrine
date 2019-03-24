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
  

