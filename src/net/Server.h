/*************************************************************************
	> File Name: Server.h
	> Author: amoscykl
	> Mail: amoscykl@163.com 
 ************************************************************************/

#ifndef SERVER_H
#define SERVER_H 

#include "Eventloop.h"
#include "Connection.h"
#include "Iobuffer.h"
#include "Timestamp.h"
#include <netinet/in.h>
#include <memory>
#include <map>

class ThreadPool;
class Channel;

class Server 
{
public:
	Server(Eventloop* loop, int port, int thread_num = 1);
	~Server();

	void Start();

	//设置回调
	void SetConnectionCallback(Connection::Callback&& cb) { connectionCallback_ = cb; }
	void SetMessageCallback(Connection::MessageCallback&& cb) { messageCallback_ = cb; }
	void SetWriteCompleteCallback(Connection::Callback&& cb) { writeCompleteCallback_ = cb; }
	void SetCloseCallback(Connection::Callback&& cb) { closeCallback_ = cb; }

private:
	//处理新连接
	void HandleNewConnection(Timestamp t);

	//移除连接
	void RemoveConnection4CloseCB(const std::shared_ptr<Connection>& conn);
	void RemoveConnection(int conn_fd);

	Eventloop* loop_;
	std::unique_ptr<ThreadPool> thread_pool;

	const int accept_sockfd;
	struct sockaddr_in addr_;
	std::shared_ptr<Channel> accept_channel;

	//描述符到连接的映射,用来保存所有连接
	std::map<int, std::shared_ptr<Connection>> Connection_map;
	
	//连接建立后的回调函数
	Connection::Callback connectionCallback_;
	//新消息到来
	Connection::MessageCallback messageCallback_;
	//写完毕时
	Connection::Callback writeCompleteCallback_;
	//连接关闭
	Connection::Callback closeCallback_;
};

#endif // SERVER_H 

