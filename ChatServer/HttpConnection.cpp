#include "HttpConnection.h"
#include "LogicSystem.h"
#include <iostream>
#include <cassert>
#include <boost/beast/core/ostream.hpp>

// 记住socket只能移动构造，不能拷贝构造
HttpConnection::HttpConnection(boost::asio::io_context& ioc)
	: _socket(ioc)
{

}

void HttpConnection::Start()
{
	std::cout << "HttpConnection 已（再次）创建！\n";
	auto self = shared_from_this();
	// async_read传参有socket、缓冲区、请求和回调函数（lambda）
	http::async_read(_socket, _buffer, _request, [self](beast::error_code ec, ::std::size_t bytes_transferred) {
		// try-catch是双刃剑，有时候出错走catch反而出问题
		try {
			if (ec) {
				std::cout << "http读错误" << ec.what() << std::endl;
				return;
			}

			boost::ignore_unused(bytes_transferred);
			self->HandleReq();
			self->CheckDeadline();
		}
		catch (std::exception& exp) {
			std::cout << "发生错误：" << exp.what() << std::endl;
		}
		});
}

void HttpConnection::CheckDeadline()
{
	auto self = shared_from_this();
	_deadline.async_wait([self](beast::error_code ec) {
		// ec为空代表计时器正常结束，关闭socket
		// ec非空（如被主动_deadline.cancel()）则什么也不做
		if (!ec) {
			self->_socket.close();
			std::cout << "连接超时了！" << std::endl;
		}
		});
}

unsigned char ToHex(unsigned char x)
{
	return  x > 9 ? x + 55 : x + 48;
}

unsigned char FromHex(unsigned char x)
{
	unsigned char y;
	if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
	else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
	else if (x >= '0' && x <= '9') y = x - '0';
	else assert(0);
	return y;
}

std::string UrlEncode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		//判断是否仅有数字和字母构成
		if (isalnum((unsigned char)str[i]) ||
			(str[i] == '-') ||
			(str[i] == '_') ||
			(str[i] == '.') ||
			(str[i] == '~'))
			strTemp += str[i];
		else if (str[i] == ' ') //为空字符
			strTemp += "+";
		else
		{
			//其他字符需要提前加%并且高四位和低四位分别转为16进制
			strTemp += '%';
			strTemp += ToHex((unsigned char)str[i] >> 4);
			strTemp += ToHex((unsigned char)str[i] & 0x0F);
		}
	}
	return strTemp;
}

std::string UrlDecode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		//还原+为空
		if (str[i] == '+') strTemp += ' ';
		//遇到%将后面的两个字符从16进制转为char再拼接
		else if (str[i] == '%')
		{
			assert(i + 2 < length);
			unsigned char high = FromHex((unsigned char)str[++i]);
			unsigned char low = FromHex((unsigned char)str[++i]);
			strTemp += high * 16 + low;
		}
		else strTemp += str[i];
	}
	return strTemp;
}

void HttpConnection::PreParseGetParam() {
	// 提取 URI  
	auto uri = _request.target();
	// 查找查询字符串的开始位置（即 '?' 的位置）  
	auto query_pos = uri.find('?');
	if (query_pos == std::string::npos) {
		_get_url = uri;
		return;
	}

	_get_url = uri.substr(0, query_pos);
	std::string query_string = uri.substr(query_pos + 1);
	std::string key;
	std::string value;
	size_t pos = 0;
	while ((pos = query_string.find('&')) != std::string::npos) {
		auto pair = query_string.substr(0, pos);
		size_t eq_pos = pair.find('=');
		if (eq_pos != std::string::npos) {
			key = UrlDecode(pair.substr(0, eq_pos)); // 假设有 url_decode 函数来处理URL解码  
			value = UrlDecode(pair.substr(eq_pos + 1));
			_get_params[key] = value;
		}
		query_string.erase(0, pos + 1);
	}
	// 处理最后一个参数对（如果没有 & 分隔符）  
	if (!query_string.empty()) {
		size_t eq_pos = query_string.find('=');
		if (eq_pos != std::string::npos) {
			key = UrlDecode(query_string.substr(0, eq_pos));
			value = UrlDecode(query_string.substr(eq_pos + 1));
			_get_params[key] = value;
		}
	}
}

void HttpConnection::HandleReq()
{
	std::cout << "已转入 HandleReq 业务，并开始检测是否超时" << std::endl;
	// 设置版本号
	_response.version(_request.version());
	// Http不是长链接，无需保活
	_response.keep_alive(false);
	// 如果处理的是 get 请求
	if (_request.method() == http::verb::get) {
		PreParseGetParam();
		bool success = LogicSystem::GetInstance()->HandleGet(_get_url, shared_from_this());
		if (!success) {
			_response.result(http::status::not_found);
			_response.set(http::field::content_type, "text/plain");
			beast::ostream(_response.body()) << "未找到 URL\r\n";
			WriteResponse(); // 将信息全部写入socket，同时将含有响应数据的socket发出去
			return;
		}
		// 成功了
		_response.set(http::field::content_type, "text/plain");
		_response.result(http::status::ok);
		_response.set(http::field::server, "GateServer");
		WriteResponse();
	}
	else if (_request.method() == http::verb::post) {
		// 判断成不成功，.target()获取的是url
		bool success = LogicSystem::GetInstance()->HandlePost(_request.target(), shared_from_this());
		if (!success) {
			_response.result(http::status::not_found);
			_response.set(http::field::content_type, "text/plain");
			beast::ostream(_response.body()) << "未找到 URL\r\n";
			WriteResponse(); // 将信息全部写入socket，同时将含有响应数据的socket发出去
			return;
		}
		// 成功了
		_response.set(http::field::content_type, "text/plain");
		_response.result(http::status::ok);
		_response.set(http::field::server, "GateServer");
		WriteResponse();
	}
}

void HttpConnection::WriteResponse()
{
	auto self = shared_from_this();
	_response.content_length(_response.body().size()); // 设置长度
	http::async_write(_socket, _response, [self](beast::error_code ec, ::std::size_t bytes_transferred) {
		self->_socket.shutdown(tcp::socket::shutdown_send, ec); // 关闭服务器端的socket连接
		self->_deadline.cancel();
		});
}