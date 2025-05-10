#pragma once
#include <boost/asio.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <queue>
#include <mutex>
#include <memory>
#include "const.h"
#include "MsgNode.h"
using namespace std;

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio/ip/tcp.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class CServer;
class LogicSystem;

class CSession : public std::enable_shared_from_this<CSession>
{
public:
    CSession(boost::asio::io_context& io_context, CServer* server);
    ~CSession();
    tcp::socket& GetSocket();
    std::string& GetUuid();
    void Start();
    void Send(char* msg, short max_length, short msgid);
    void Send(std::string msg, short msgid);
    void Close();
    std::shared_ptr<CSession> SharedSelf();
    void AsyncReadBody(int length);
    void AsyncReadHead(int total_len);
private:
    void asyncReadFull(std::size_t maxLength, std::function<void(const boost::system::error_code&, std::size_t)> handler);
    void asyncReadLen(std::size_t read_len, std::size_t total_len,
        std::function<void(const boost::system::error_code&, std::size_t)> handler);

    void HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> shared_self);
    tcp::socket _socket;
    std::string _uuid;
    char _data[MAX_LENGTH];
    std::mutex _data_mutex; // 添加互斥锁保护_data
    CServer* _server;
    std::atomic<bool> _b_close;  // 使用atomic替代普通bool
    std::queue<shared_ptr<SendNode>> _send_que;
    std::mutex _send_lock;
    //收到的消息结构
    std::shared_ptr<RecvNode> _recv_msg_node;
    std::atomic<bool> _b_head_parse;  // 使用atomic替代普通bool
    //收到的头部结构
    std::shared_ptr<MsgNode> _recv_head_node;
    std::mutex _recv_mutex; // 保护recv结构相关
};

class LogicNode {
    friend class LogicSystem;
public:
    LogicNode(shared_ptr<CSession>, shared_ptr<RecvNode>);
private:
    shared_ptr<CSession> _session;
    shared_ptr<RecvNode> _recvnode;
};