#pragma once
#include <boost/asio.hpp>
#include "CSession.h"
#include <memory>
#include <map>
#include <mutex>
#include <shared_mutex>  // 添加共享互斥锁
using namespace std;
using boost::asio::ip::tcp;

class CServer
{
public:
    CServer(boost::asio::io_context& io_context, short port);
    ~CServer();
    void ClearSession(std::string uuid);
private:
    void HandleAccept(shared_ptr<CSession>, const boost::system::error_code& error);
    void StartAccept();
    boost::asio::io_context& _io_context;
    short _port;
    tcp::acceptor _acceptor;
    std::map<std::string, shared_ptr<CSession>> _sessions;
    std::shared_mutex _sessions_mutex;  // 使用读写锁代替普通互斥锁
};