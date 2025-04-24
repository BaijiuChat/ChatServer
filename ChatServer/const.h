#pragma once  
// C++标准库  
//#include <iostream>  
//#include <vector>
//#include <map>  
//#include <unordered_map>  
//#include <memory>  //实现CRTP奇异递归  
//#include <functional> // 实现std::function  
//#include "Singleton.h"  

// JSON库  
//#include <json/json.h>  // 基础功能  
//#include <json/value.h>  // 节点结构  
//#include <json/reader.h>  // 解析json  

// Boost库  
//#include <boost/beast/http.hpp> // http  
//#include <boost/beast.hpp> // http和websocket  
//#include <boost/asio.hpp> // 异步IO  
//#include <boost/filesystem.hpp> // 文件系统  
//#include <boost/property_tree/ptree.hpp> // 配置文件
//#include <boost/property_tree/ini_parser.hpp> // 解析ini配置文件

// gRPC库  
//#include <grpcpp/grpcpp.h>	// gRPC的头文件  
//#include "message.grpc.pb.h" // message.proto编译生成的头文件

//namespace beast = boost::beast;         // from <boost/beast.hpp>  
//namespace http = beast::http;           // from <boost/beast/http.hpp>  
//namespace net = boost::asio;            // from <boost/asio.hpp>  
//using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>  

// 统一错误码定义（服务器和客户端共用）
enum ErrorCodes {
    // 基础错误码 (0-999)
    SUCCESS = 0,            // 成功
    ERR_JSON = 1,           // JSON解析失败（客户端）
    ERR_NETWORK = 2,        // 网络错误（客户端）

    // 业务逻辑错误码 (1000-1999)
    Error_Json = 1001,      // JSON解析失败（服务器）
    RPCFailed = 1002,       // RPC调用失败（服务器）
    VerifyExpired = 1003,   // 验证码过期
    VerifyCodeErr = 1004,   // 验证码错误
    PasswdErr = 1006,       // 两次密码错误
    UserMailNotMatch = 1007,// 邮箱不匹配
    PasswdUpFailed = 1008,  // 更新密码失败
    PasswdInvalid = 1009,   // 登录密码无效
    RPCGetFailed = 1010,    // RPC无法获取聊天服务器
    ConnectionPoolFailed = 1011, // 无法获取池子中连接

    // 数据相关错误码 (2000-2999)
    UserEmailExists = 2000,     // 用户或邮箱存在
    SQLFailed = 2001,           // SQL异常
    DatabaseConnectionFailed = 2002, // 数据库连接失败
    DatabaseProcedureError = 2003,   // 数据库存储过程错误
    UserEmailNotExists = 2004,     // 用户或邮箱不存在
    UidInvalid = 2005,          // UID无效
    TokenInvalid = 2006,        // TOKEN无效

    // 系统错误码 (3000-3999)
    GeneralException = 3001,    // 一般异常
    UnknownException = 3002,    // 未知异常
    UnknownError = 3003         // 未定义的错误
};

#define CODEPREFIX "code_"

#define MAX_LENGTH 1024*2
#define HEAD_TOTAL_LEN 4
#define HEAD_ID_LEN 2
#define HEAD_DATA_LEN 2
#define MAX_RECVQUE 10000
#define MAX_SENDQUE 1000

enum MSG_IDS {
	MSG_CHAT_LOGIN = 1005,  // 登录
	MSG_CHAT_LOGIN_RSP = 1006, // 登录回包
};