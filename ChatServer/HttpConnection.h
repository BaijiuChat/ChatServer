#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <chrono>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class LogicSystem; //前置声明LogicSystem，避免头文件的循环引用，一定不要在头文件include LogicSystem
class HttpConnection:public std::enable_shared_from_this<HttpConnection>
{
public:
	friend class LogicSystem; // 因为它是单例
	HttpConnection(net::io_context& ioc);
	void Start();
	tcp::socket& GetSocket() { return _socket; }
private:
	void CheckDeadline();
	void PreParseGetParam();
	// 检测是否超时掉线
	void HandleReq();     // TCP处理请求函数
	void WriteResponse(); // TCP连接需要应答，应答函数

	std::string _get_url;
	std::unordered_map<std::string, std::string> _get_params;

	tcp::socket _socket;
	// 定义​动态缓冲区​​（8KB）用于临时存储接收或发送的HTTP报文数据​​（如请求头、响应体）。
	beast::flat_buffer _buffer{ 8192 }; 
	// 定义一个​​HTTP请求对象​​，使用dynamic_body表示可变长度的请求体​​（如 POST 数据、文件上传）
	http::request<http::dynamic_body> _request; 
	// 定义一个HTTP响应对象​​，同样使用dynamic_body表示可变长度的响应体​​（如 HTML 页面、JSON 数据）
	http::response<http::dynamic_body> _response;
	// 用来做定时器判断请求是否超时, get_executor()是为了让定时器和socket在同一个上下文中运行
	net::steady_timer _deadline{
		_socket.get_executor(), std::chrono::seconds(30)
	};
};

