#include "LogicSystem.h"
#include "StatusGrpcClient.h"
#include "MysqlMgr.h"
#include "const.h"

using namespace std;

LogicSystem::LogicSystem() : _b_stop(false) {
    RegisterCallBacks();
    _worker_thread = std::thread(&LogicSystem::DealMsg, this);
}

LogicSystem::~LogicSystem() {
    _b_stop = true;
    _consume.notify_one();
    _worker_thread.join();
}

void LogicSystem::PostMsgToQue(shared_ptr<LogicNode> msg) {
    std::unique_lock<std::mutex> unique_lk(_mutex);
    _msg_que.push(msg);
    unique_lk.unlock();  // 提前解锁，避免通知后仍持有锁
    _consume.notify_one();  // 无论队列大小，都发送通知
}

void LogicSystem::DealMsg() {
    for (;;) {
        std::shared_ptr<LogicNode> msg_node;
        {
            std::unique_lock<std::mutex> unique_lk(_mutex);
            //判断队列为空则用条件变量阻塞等待，并释放锁
            while (_msg_que.empty() && !_b_stop) {
                _consume.wait(unique_lk);
            }

            //判断是否为关闭状态，把所有逻辑执行完后则退出循环
            if (_b_stop && _msg_que.empty()) {
                break;
            }

            // 检查队列为空，避免误唤醒
            if (_msg_que.empty()) {
                continue;
            }

            //如果没有停服，且说明队列中有数据
            msg_node = _msg_que.front();
            _msg_que.pop();
        } // 提前释放锁，避免回调函数中可能访问队列

        // 在锁外处理消息
        if (msg_node) {
            auto msg_id = msg_node->_recvnode->_msg_id;
            cout << "recv_msg id is " << msg_id << endl;
            // 复制一份回调函数表，避免回调中修改表
            auto call_back_iter = _fun_callbacks.find(msg_id);
            if (call_back_iter == _fun_callbacks.end()) {
                std::cout << "msg id [" << msg_id << "] handler not found" << std::endl;
                continue;
            }
            call_back_iter->second(msg_node->_session, msg_id,
                std::string(msg_node->_recvnode->_data, msg_node->_recvnode->_cur_len));
        }
    }
}

void LogicSystem::RegisterCallBacks() {
    _fun_callbacks[MSG_CHAT_LOGIN] = std::bind(&LogicSystem::LoginHandler, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

void LogicSystem::LoginHandler(shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    auto uid = root["uid"].asInt();
    std::cout << "user login uid is " << uid << " user token is "
        << root["token"].asString() << endl;
    //从状态服务器获取token匹配是否准确
    auto rsp = StatusGrpcClient::GetInstance()->Login(uid, root["token"].asString());
    Json::Value rtvalue;
    Defer defer([this, &rtvalue, session, msg_id]() {
        std::string return_str = rtvalue.toStyledString();
        session->Send(return_str, msg_id);
        });

    rtvalue["error"] = rsp.error();
    if (rsp.error() != ErrorCodes::Success) {
        return;
    }

    //内存中查询用户信息
    std::shared_ptr<UserInfo> user_info = nullptr;
    {
        std::shared_lock<std::shared_mutex> lock(_users_mutex);
        auto find_iter = _users.find(uid);
        if (find_iter != _users.end()) {
            user_info = find_iter->second;
        }
    }

    if (!user_info) {
        //查询数据库
        user_info = MysqlMgr::GetInstance()->GetUser(uid);
        if (user_info == nullptr) {
            rtvalue["error"] = ErrorCodes::UidInvalid;
            return;
        }

        {
            std::unique_lock<std::shared_mutex> lock(_users_mutex);
            _users[uid] = user_info;
        }
    }

    rtvalue["uid"] = uid;
    rtvalue["token"] = rsp.token();
    rtvalue["email"] = user_info->email;
}