#pragma once
#include <map>
#include <memory>      // 用于 std::shared_ptr
#include <functional>  // 用于 std::function
#include <queue>
#include <thread>
#include <unordered_map>
#include <json/json.h>
#include <json/reader.h>
#include <json/value.h>
#include "Defer.h"
#include "StatusGrpcClient.h"
#include "MySqlMgr.h"
#include "data.h"
#include "const.h"
#include "CSession.h"
#include "Singleton.h"

class HttpConnection; //前置声明HttpConnection，避免头文件的循环引用，一定不要在头文件include HttpConnection
typedef std::function<void(std::shared_ptr<HttpConnection>)>HttpHandler; // 声明HttpHandler函数
class LogicSystem : public Singleton<LogicSystem> {
	friend class Singleton<LogicSystem>;
public:
	~LogicSystem();
	void PostMsgToQue(std::shared_ptr<HttpConnection> msg);
private:
	LogicSystem();
	void DealMsg();
	void RegisterCallBacks();
	void LoginHandler(std::shared_ptr<LogicNode >> _msg_que;
	std::mutex _mutex;
	std::condition_variable _consume;
	bool _b_stop;
	std::map<short, FunCallBack> _fun_callbacks; // 业务逻辑回调函数
	std::unordered_map<int, std::shared_ptr<UserInfo>> _users;
};

