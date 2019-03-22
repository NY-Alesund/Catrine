/*************************************************************************
	> File Name: Server.cpp
	> Author: amoscykl
	> Mail: amoscykl@163.com 
 ************************************************************************/

#include "Server.h"
#include "Util.h"
#include "ThreadPool.h"
#include "Channel.h"
#include <string.h>
#include <netinet/in.h>

Server::Server(Eventloop* loop, int port, int thread_num)
	: loop_(loop),
	  thread_pool(new ThreadPool(loop_, thread_num)),
	  accept_sockfd(Util::Create())
{
	//创建服务器地址结构
	bzero(&addr_,sizeof(addr_));
	addr_.sin_family = AF_INET;
	addr_.sin_addr.s_addr = htonl(INADDR_ANY);
	addr_.sin_port = htons(port);

	//设置可重用及绑定地址结构
	Util::SetReuseAddr(accept_sockfd);
	Util::Bind(accept_sockfd, addr_);

	//初始化事件
	accept_channel = std::make_shared<Channel>(accept_sockfd);
	accept_channel->SetReadCallback(std::bind(&Server::HandleNewConnection, this, std::placeholders::_1));
	accept_channel->EnableReadEvents();
}

Server::~Server() 
{
	Util::Close(accept_sockfd);
}

//启动
void Server::Start()
{
	Util::Listen(accept_sockfd);
	loop_->AddChannel(accept_channel);
	thread_pool->Start();		//启动线程池，创造一定数量的线程
}

//处理新连接
void Server::HandleNewConnection(Timestamp t)
{
	//客户端地址结构
	struct sockaddr_in peer_addr;
	bzero(&peer_addr, sizeof(peer_addr));
	int conn_fd = Util::Accept(accept_sockfd, &peer_addr);

	//从线程池中取用线程给连接使用
	Eventloop* io_loop = thread_pool->TakeOutLoop();
	std::shared_ptr<Connection> conn = std::make_shared<Connection>(io_loop, conn_fd, addr_, peer_addr);
	conn->SetConnectionCallback(connectionCallback_);
	conn->SetMessageCallback(messageCallback_);
	conn->SetWriteCompleteCallback(writeCompleteCallback_);
	conn->SetCloseCallback(closeCallback_);

	Connection_map[conn_fd] = conn;

	//在分配到的线程上注册事件
	io_loop->RunTask(std::bind(&Connection::Register, conn));
}

//移除连接，由线程池中线程调用
void Server::RemoveConnection4CloseCB(const std::shared_ptr<Connection>& conn)
{
	loop_->AddTask(std::bind(&Server::RemoveConnection, this, conn->GetFd()));
}

//移除连接,在此线程调用
void Server::RemoveConnection(int conn_fd)
{
	Connection_map.erase(conn_fd);
}

