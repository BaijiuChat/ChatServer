#include <iostream>
#include <sstream>
#include <json/json.h>
#include <json/reader.h>
#include <json/value.h>
#include "CSession.h"
#include "CServer.h"
#include "LogicSystem.h"
#include "MsgNode.h"

CSession::CSession(boost::asio::io_context& io_context, CServer* server) :
	_socket(io_context), _server(server), _b_close(false), _b_head_parse(false) 
{
	boost::uuids::uuid uuid = boost::uuids::random_generator()();
	_recv_head_node = std::make_shared<MsgNode>(HEAD_TOTAL_LEN);
}

CSession::~CSession() {
	std::cout << "析构会话" << std::endl;
}

tcp::socket& CSession::GetSocket() {
	return _socket;
}

std::string CSession::GetUuid() {
	return _uuid;
}

void CSession::Start() {
	AsyncReadHead(HEAD_TOTAL_LEN);
}

void CSession::Send(std::string msg, short msgid) {
	std::lock_guard<std::mutex> lock(_send_lock);
	int send_que_size = _send_que.size();
	if (send_que_size > MAX_SENDQUE) {
		std::cout << "发送队列已满，丢弃消息" << std::endl;
		return;
	}

	_send_que.push(std::make_shared<SendNode>(msg.c_str(), msg.length(), msgid));
	// 如果队列中有元素了，则开始发送
	if (send_que_size > 0) {
		return;
	}
	auto& msgnode = _send_que.front();
	boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len)),
		std::bind(&CSession::HandleWrite, this, std::placeholders::_1, shared_from_this());
}

void CSession::Send(char* msg, short max_length, short msgid) {
	std::lock_guard<std::mutex> lock(_send_lock);
	int send_que_size = _send_que.size();
	if (send_que_size > MAX_SENDQUE) {
		std::cout << "发送队列已满，丢弃消息" << std::endl;
		return;
	}

	_send_que.push(std::make_shared<SendNode>(msg, max_length, msgid));
	// 如果队列中有元素了，则开始发送
	if (send_que_size > 0) {
		return;
	}
	auto& msgnode = _send_que.front();
	boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len)),
		std::bind(&CSession::HandleWrite, this, std::placeholders::_1, shared_from_this());
}

void CSession::Close() {
	_socket.close();
	_b_close = true;
}

std::shared_ptr<CSession>CSession::SharedSelf() {
	return shared_from_this();
}

void CSession::AsyncReadBody(int total_len)
{
	auto self = shared_from_this();
	asyncReadFull(total_len, [self, this, total_len](const boost::system::error_code& ec, std::size_t bytes_transfered) {
		try {
			if (ec) {
				std::cout << "读取数据失败，错误是" << ec.message() << std::endl;
				Close();
				_server->ClearSession(_uuid);
				return;
			}
			if (bytes_transfered < total_len) {
				std::cout << "读取数据失败，读取的字节数小于" << total_len << std::endl;
				Close();
				_server->ClearSession(_uuid);
				return;
			}
			memcpy(_recv_msg_node->_data, _data, bytes_transfered);
			_recv_msg_node->_cur_len += bytes_transfered;
			_recv_msg_node->_data[_recv_msg_node->_total_len] = '\0'; // 添加结束符，方便处理
			std::wcout << L"读取数据成功，数据是" << _recv_msg_node->_data << std::endl;
			// 将消息投递到逻辑队列中
			LogicSystem::GetInstance()->PostMsgToQue(make_shared<LogicNode>(shared_from_this(), _recv_msg_node));
			// 继续监听头部接受事件
			AsyncReadHead(HEAD_TOTAL_LEN);
		}
		catch (std::exception& e) {
			std::cout << "读取数据失败，异常是" << e.what() << std::endl;
		}
	});
}

void CSession::AsyncReadHead(int total_len) {
	auto self = shared_from_this();
	// 读完头部后，开始读数据
	asyncReadFull(HEAD_TOTAL_LEN, [self, this](const boost::system::error_code& ec, std::size_t bytes_transfered) {
		try {
			if (ec) {
				std::cout << "读取头部失败，错误是" << ec.message() << std::endl;
				Close();
				_server->ClearSession(_uuid);
				return;
			}

			if (bytes_transfered < HEAD_TOTAL_LEN) {
				std::cout << "读取头部失败，读取的字节数小于" << HEAD_TOTAL_LEN << std::endl;
				Close();
				_server->ClearSession(_uuid);
				return;
			}
			_recv_head_node->Clear(); // 清空数据
			memcpy(_recv_head_node->_data, _data, bytes_transfered);

			// 解析头部
			short msg_id = 0;
			memcpy(_recv_head_node->_data, _data, bytes_transfered);

			// 网络字节序转化为本地字节序
			msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
			std::cout << "读取头部成功，消息ID是" << msg_id << std::endl;
			// id非法
			if (msg_id < 0 || msg_id > MAX_LENGTH) {
				std::cout << "读取头部失败，消息ID非法" << std::endl;
				_server->ClearSession(_uuid);
				return;
			}
			short msg_len = 0;
			memcpy(&msg_len, _recv_head_node->_data + HEAD_ID_LEN, HEAD_DATA_LEN);
			_recv_msg_node = make_shared<RecvNode>(msg_len, msg_id);
			AsyncReadBody(msg_len);
		}
		catch (std::exception& e) {
			std::cout << "读取头部失败，异常是" << e.what() << std::endl;
		}
	});
}

void CSession::HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> shared_self) {
	// 异常处理
	try {
		if (!error) {
			std::lock_guard<std::mutex> lock(_send_lock);
			_send_que.pop();
			if (!_send_que.empty()) {
				auto& msgnode = _send_que.front();
				boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len),
					[this, shared_self](const boost::system::error_code& ec, std::size_t bytes_transfered) {
						HandleWrite(ec, shared_self);
					});
			}
		}
		else {
			std::cout << "发送失败，错误是" << error.message() << std::endl;
			Close();
			_server->ClearSession(_uuid);
		}
	}
	catch (const std::exception& e) {
		std::cout << "发送失败，异常是" << e.what() << std::endl;
		Close();
		_server->ClearSession(_uuid);
	}
}

void CSession::asyncReadFull(std::size_t maxLength, 
	std::function<void(const boost::system::error_code&, std::size_t)> handler) 
{
	::memset(_data, 0, MAX_LENGTH);
	asyncReadLen(0, maxLength, handler);
}

void CSession::asyncReadLen(std::size_t read_len, std::size_t total_len, 
	std::function<void(const boost::system::error_code&, std::size_t)> handler) 
{
	auto self = shared_from_this();
	_socket.async_read_some(boost::asio::buffer(_data + read_len, total_len - read_len),
		[self, read_len, total_len, handler](const boost::system::error_code& ec, std::size_t bytes_transfered) {
			if (ec) {
				// 读取失败
				handler(ec, read_len + bytes_transfered);
				return;
			}
			if (read_len + bytes_transfered >= total_len) {
				// 读取完成
				handler(ec, read_len + bytes_transfered);
				return;
			}
			// 继续读取
			self->asyncReadLen(read_len + bytes_transfered, total_len, handler);
		});
}

LogicNode::LogicNode(std::shared_ptr<CSession> session, std::shared_ptr<RecvNode> recvnode) :
	_session(session), _recvnode(recvnode) 
{

}