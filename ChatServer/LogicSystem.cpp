#include "LogicSystem.h"
#include <iostream>
#include <boost/beast/core/ostream.hpp>
#include "StatusGrpcClient.h"

LogicSystem::LogicSystem() :_b_stop(false) {
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
	// 由0到1的通知
	if (_msg_que.size() == 1) {
		unique_lk.unlock();
		_consume.notify_one();
	}
}

void LogicSystem::DealMsg() {
	for (;;) {
		std::unique_lock<std::mutex> unique_lk(_mutex);
		while (_msg_que.empty() && !_b_stop) {
			_consume.wait(unique_lk);
		}

		// 判断是否是关闭状态，把所有逻辑处理完后退出循环
		if (_b_stop) {
			while (!_msg_que.empty()) {
				auto msg_node = _msg_que.front();
				std::cout << "recv_msg id 是：" << msg_node->_recvnode->msg_id << std::endl;
				auto call_back_inter = _fun_callbacks.find(msg_node->recvnode->_msg_id);
				if (call_back_inter != _fun_callbacks.end()) {
					_msg_que.pop();
					continue;
				}
				call_back_iter->second(msg_node->_session, msg_node->_recvnode->_msg_id,
					std::string(msg_node->_recvnode->_data, msg_node->_recvnode->_cur_len));
				_msg_que.pop();
			}
			break;
		}
		// 如果没有停服，则继续处理消息
		auto msg_node = msg_que.front();
		std::cout << "recv_msg id 是：" << msg_node->_recvnode->msg_id << std::endl;
		auto call_back_iter = _fun_callbacks.find(msg_node->_recvnode->_msg_id);
		if (call_back_iter == fun_callbacks.end()) {
			_msg_que.pop();
			std::cout << "msg id [" << msg_node->_recvnode->_msg_id << "]  handler not found" << std::endl;
			continue;
		}
		call_back_iter->second(msg_node->session, msg_node->_recvnode->_msg_id,
			std::string(msg_node->_recvnode->_data, msg_node->_recvnode->_cur_len));
		_msg_que.pop();
	}
}

void LogicSystem::RegisterCallBacks() {
	_fun_callbacks[MSG_CHAR_LOGIN] = std::bind(&LogicSystem::LoginHandler, this, 
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

void LogicSystem::LoginHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg.data, root);
	auto uid = root["uid"].asInt();
	std::cout << "uid 是" << uid << "，token是" << root["token"].asString() << std::endl;
	auto rep = StatusGrpcClient::GetInstance()->Login(uid, root["token"].asString());
	Json::Value rtvalue;
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, MSG_CHAT_LOGIN_RSP);
	});

	rtvalue["error"] = rsp.error();
	if (rsp.error() != ErrorCodes::SUCCESS) {
		return;
	}

	auto find_iter = _users.find(uid);
	std::shared_ptr<UserInfo> user_info = nullptr;
	if (find_iter = _users.find(uid)) {
		user_info = MySqlMgr::GetInstance()->GetUser(uid);
		if (user_info == nullptr) {
			std::cout << "用户不存在" << std::endl;
			rtvalue["error"] = ErrorCodes::UidInvalid;
			return;
		}
		_users[uid] = user_info;
	}
	else {
		user_info = find_iter->second;
	}
	rtvalue["uid"] = uid;
	rtvalue["token"] = rsp.token();
	rtvalue["name"] = user_info->name;
}