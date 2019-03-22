	/*************************************************************************
		> File Name: Connection.cpp
		> Author: amoscykl
		> Mail: amoscykl@163.com 
	 ************************************************************************/

#include "Connection.h"
#include "Channel.h"
#include "Eventloop.h"
#include "Logger.h"
#include "Util.h"
#include <unistd.h>

Connection::Connection(Eventloop* loop, int conn_sockfd, const struct sockaddr_in& localAddr, const struct sockaddr_in& peerAddr)
	: loop_(loop),
	  conn_sockfd_(conn_sockfd),
	  conn_channel(new Channel(conn_sockfd_)),	//连接事件分发器
	  localAddr_(localAddr),
	  peerAddr_(peerAddr),
	  context_(nullptr)
{
	//设置连接各种事件回调
	conn_channel->SetReadCallback(std::bind(&Connection::HandleRead, this, std::placeholders::_1));
	conn_channel->SetWriteCallback(std::bind(&Connection::HandleWrite,this));
	conn_channel->SetCloseCallback(std::bind(&Connection::HandleClose,this));
	conn_channel->EnableReadEvents();
}

Connection::~Connection()
{
	Util::Close(conn_sockfd_);
}

//连接建立后在分配的线程上注册事件
void Connection::Register()
{
	loop_->AddChannel(conn_channel);	//添加事件分发器即在epoller_注册相应事件
	if(connectionCallback_)
		connectionCallback_(shared_from_this());
}


//往对端发送消息
void Connection::Send(const void* message, size_t len)
{
	ssize_t n_wrote = 0;
	//当output_buffer为空时直接write而不经过缓冲区
	if(!conn_channel->IsWriting() && output_buffer.GetReadableSize() == 0)
	{
		n_wrote = write(conn_channel->GetFd(), message, len);
		if(n_wrote >= 0)
		{
			//数据已写完
			if((size_t)n_wrote == len && writeCompleteCallback_)
				writeCompleteCallback_(shared_from_this());
		}
		else 
		{
			n_wrote = 0;
			if(errno != EWOULDBLOCK)
			{
				LOG_ERROR << "write error";
			}
		}
	}

	//数据未能一次性写完或者缓冲区不为空
	if((ssize_t)n_wrote < len)
	{
		output_buffer.Append(static_cast<const char*>(message) + n_wrote, len - n_wrote);	//剩下的数据加入到输出缓冲区
		if(!conn_channel->IsWriting())
		{
			conn_channel->EnableWriteEvents();	//关注可写事件
			loop_->ModChannel(conn_channel);
		}
	}
}

void Connection::Send(const std::string& message)
{
	Send(message.data(), message.size());
}

void Connection::Send(IOBuffer& buffer)
{
	Send(buffer.GetReadablePtr(), buffer.GetReadableSize());
	buffer.RetrieveAll();
}

void Connection::Shutdown()
{
	if(!conn_channel->IsWriting())
	{
		Util::ShutdownWrite(conn_sockfd_);
	}
}

//处理可读事件
void Connection::HandleRead(Timestamp t)
{
	int saved_errno = 0;
	ssize_t n = 0;

	n = input_buffer.ReadFd(conn_channel->GetFd(), &saved_errno);
	if(n > 0)
	{
		if(messageCallback_)
			messageCallback_(shared_from_this(), &input_buffer, t);
	}
	else 
	{
		HandleClose();
	}
}

//处理可写事件
void Connection::HandleWrite()
{
	if(conn_channel->IsWriting())
	{
		//GetReadable**获得输出缓冲区可写的位置和大小 
		ssize_t n = write(conn_channel->GetFd(), output_buffer.GetReadablePtr(), output_buffer.GetReadableSize());
		if(n > 0)
		{
			output_buffer.Retrieve(n);
			if(output_buffer.GetReadableSize() == 0)
			{
				conn_channel->DisableWriteEvents();
				loop_->ModChannel(conn_channel);
				if(writeCompleteCallback_)
				{
					writeCompleteCallback_(shared_from_this());
				}
			}
		}
		else 
		{
			LOG_ERROR << "write error";
		}
	}
}


//关闭连接
void Connection::HandleClose()
{
	loop_->DelChannel(conn_channel);
	if(closeCallback_)
		closeCallback_(shared_from_this());
	if(suicideCallback_)
		suicideCallback_(shared_from_this());
}

