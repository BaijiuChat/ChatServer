#pragma once
#include "Singleton.h"
#include <queue>
#include <thread>
#include "CSession.h"
#include <queue>
#include <map>
#include <functional>
#include "const.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include <unordered_map>
#include <shared_mutex>  // 添加共享互斥锁
#include "data.h"

typedef function<void(shared_ptr<CSession>, const short& msg_id, const string& msg_data)> FunCallBack;

class LogicSystem : public Singleton<LogicSystem>
{
    friend class Singleton<LogicSystem>;
public:
    ~LogicSystem();
    void PostMsgToQue(shared_ptr<LogicNode> msg);
private:
    LogicSystem();
    void DealMsg();
    void RegisterCallBacks();
    void LoginHandler(shared_ptr<CSession>, const short& msg_id, const string& msg_data);
    std::thread _worker_thread;
    std::queue<shared_ptr<LogicNode>> _msg_que;
    std::mutex _mutex;
    std::condition_variable _consume;
    std::atomic<bool> _b_stop;  // 使用atomic替代普通bool
    std::map<short, FunCallBack> _fun_callbacks;
    std::shared_mutex _users_mutex;  // 保护_users的互斥锁
    std::unordered_map<int, std::shared_ptr<UserInfo>> _users;
};