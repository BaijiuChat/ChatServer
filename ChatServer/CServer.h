#pragma once
#include <memory>
#include <map>
#include <mutex>
#include <boost/asio.hpp>
#include "CSession.h"

using boost::asio::ip::tcp;

class CServer
{
public:
	CServer(boost::asio::io_context& io_context, unsigned short& port);  // 上面的namespace
	~CServer();
	void ClearSession();
private:
	void HandleAccept(std::shared_ptr<CSession>, const boost::system::error_code& error);
	void StartAccept();
	boost::asio::io_context& _io_context;
	unsigned short _port;
	tcp::acceptor _acceptor; // 监听器
	std::map<std::string, std::shared_ptr<CSession>> _sessions; // 连接的会话
	std::mutex _session_mutex; // 互斥锁，保护_sessions
};

