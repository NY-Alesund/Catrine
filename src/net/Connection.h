/*************************************************************************
	> File Name: Connection.h
	> Author: amoscykl
	> Mail: amoscykl@163.com 
 ************************************************************************/

#ifndef CONNECTION_H
#define CONNECTION_H 

#include "Iobuffer.h"
#include "Timestamp.h"
#include "Anyone.h"
#include <functional>
#include <memory>

class Channel;
class Eventloop;

class Connection : public std::enable_shared_from_this<Connection>
{
public:
	using Callback = std::function<void(const std::shared_ptr<Connection>&)>;	//连接建立,写完毕，连接关闭回调
	using MessageCallback = std::function<void(const std::shared_ptr<Connection>&, IOBuffer*, Timestamp)>;	//消息到达回调

	Connection(Eventloop* loop, int conn_sockfd, const struct sockaddr_in& localAddr, const struct sockaddr_in& peerAddr);
	~Connection();

	//在loop上注册事件，连接建立时调用
	void Register();

	void Send(const void* message, size_t len);
	void Send(const std::string& message);
	void Send(IOBuffer& message);

	void Shutdown();

	//处理事件
	void HandleRead(Timestamp t);
	void HandleWrite();
	void HandleClose();

	//设置回调函数
	void SetConnectionCallback(const Callback& cb) { connectionCallback_ = cb; }
	void SetMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
	void SetWriteCompleteCallback(const Callback& cb) { writeCompleteCallback_ = cb; }
	void SetCloseCallback(const Callback& cb) { closeCallback_ = cb; }

	const int GetFd() const { return conn_sockfd_; }
	const IOBuffer& GetInputBuffer() const { return input_buffer; }
	const IOBuffer& GetOutputBuffer() const { return output_buffer; }

	void SetContext(const any& context) { context_ = context; }
	const any& GetContext() const { return context_; }
	any* GetMutableContext() { return &context_; }

private:
	Eventloop* loop_;

	//连接描述符
	const int conn_sockfd_;
	std::shared_ptr<Channel> conn_channel;
	
	//服务器、客户端地址结构
	struct sockaddr_in localAddr_;
	struct sockaddr_in peerAddr_;

	//关注三个半事件(这几个回调函数通过hand**那四个事件处理函数调用)
	//连接建立回调
	Callback connectionCallback_;
	//消息到达回调
	MessageCallback  messageCallback_;
	//写完毕回调
	Callback writeCompleteCallback_;
	//连接关闭回调
	Callback closeCallback_;
	
	//结束自己生命的回调
	Callback suicideCallback_;

	//输入输出缓冲区
	IOBuffer input_buffer;
	IOBuffer output_buffer;
	
	//解析上下文
	any context_;
};

#endif // CONNECTION_H
