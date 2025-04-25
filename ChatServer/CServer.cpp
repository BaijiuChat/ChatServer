#include "CServer.h"
#include <iostream>
#include "AsioIOServicePool.h"

CServer::CServer(boost::asio::io_context& io_context, short port) :
    _io_context(io_context), _port(port),
    _acceptor(io_context, tcp::endpoint(tcp::v4(), port))
{
    cout << "Server start success, listen on port : " << _port << endl;
    StartAccept();
}

CServer::~CServer() {
    cout << "Server destruct listen on port : " << _port << endl;

    // 确保所有会话都被清理
    std::unique_lock<std::shared_mutex> lock(_sessions_mutex);
    _sessions.clear();
}

void CServer::HandleAccept(shared_ptr<CSession> new_session, const boost::system::error_code& error) {
    if (!error) {
        new_session->Start();
        {
            std::unique_lock<std::shared_mutex> lock(_sessions_mutex);
            _sessions.insert(make_pair(new_session->GetUuid(), new_session));
        }
    }
    else {
        cout << "session accept failed, error is " << error.what() << endl;
    }

    StartAccept();
}

void CServer::StartAccept() {
    auto& io_context = AsioIOServicePool::GetInstance()->GetIOService();
    shared_ptr<CSession> new_session = make_shared<CSession>(io_context, this);
    _acceptor.async_accept(new_session->GetSocket(),
        std::bind(&CServer::HandleAccept, this, new_session, std::placeholders::_1));
}

void CServer::ClearSession(std::string uuid) {
    std::unique_lock<std::shared_mutex> lock(_sessions_mutex);
    _sessions.erase(uuid);
}