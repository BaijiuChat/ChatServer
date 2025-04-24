#include "CServer.h"
#include "AsioIOServicePool.h"
#include <mutex> // 添加此行以解决未定义标识符 "mutex" 的问题
#include <iostream>

// _ioc、_acceptor和_socket都是私有变量，而构造函数中没有提供，所以程序会调用默认构造
// 为了让其正常构造，需要使用拷贝构造，也就是 冒号 后面的三个东西
CServer::CServer(boost::asio::io_context& ioc, unsigned short& port) 
	:_io_context(ioc), // 底层异步I/O调度器
	 _acceptor(ioc, tcp::endpoint(tcp::v4(), port))  // 接入ioc，并监听本机所有IPv4:port端口
{
	StartAccept(); // 启动监听
	std::cout << "现在正在监听 0.0.0.0 的 8080 端口" << std::endl;
}

CServer::~CServer() {
	// 清除所有会话
	ClearSession();
	std::cout << "服务器析构" << std::endl;
}

void CServer::HandleAccept(std::shared_ptr<CSession> new_session, const boost::system::error_code& error) {
	if (!error) {
		new_session->Start();
		std::lock_guard<std::mutex> lock(_session_mutex);
		_sessions.insert(std::make_pair(new_session->GetUuid(), new_session)); // 插入会话
	}
	else {
		std::cout << "连接失败，错误是" << error.message() << std::endl;
	}
	StartAccept(); // 继续监听
}

void CServer::StartAccept() {
	auto& io_context = AsioIOServicePool::GetInstance()->GetIOService();
	std::shared_ptr<CSession> new_session = std::make_shared<CSession>(io_context, this);
	_acceptor.async_accept(new_session->GetSocket(),
		[this, new_session](const boost::system::error_code& ec) {
			HandleAccept(new_session, ec);
		});
}

void CServer::ClearSession() {  
   std::lock_guard<std::mutex> lock(_session_mutex);  
   _sessions.erase(uuid);
}


