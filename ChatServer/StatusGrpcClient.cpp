#include "StatusGrpcClient.h"

GetChatServerRsp StatusGrpcClient::GetChatServer(int uid)
{
    // 提前准备好回复对象
    GetChatServerRsp reply;

    // 获取连接
    auto stub = pool_->getConnection();
    if (!stub) {
        reply.set_error(ErrorCodes::ConnectionPoolFailed);
        return reply;
    }

    // 准备请求
    ClientContext context;
    GetChatServerReq request;
    request.set_uid(uid);

    // 使用RAII方式确保连接归还
    Defer connectionGuard([this, &stub]() {
        pool_->returnConnection(std::move(stub));
        });

    // 设置RPC超时
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));

    // 执行RPC调用
    Status status = stub->GetChatServer(&context, request, &reply);

    // 处理结果
    if (!status.ok()) {
        reply.set_error(ErrorCodes::RPCFailed);
        // 记录错误信息
        std::cout << "GetChaGetChatServer RPC failed, uid:" << uid << ", error: " << status.error_message() << std::endl;
    }

    return reply;
}

StatusGrpcClient::StatusGrpcClient()
{
    try {
        auto& gCfgMgr = ConfigMgr::Inst();
        std::string host = gCfgMgr["StatusServer"]["Host"];
        std::string port = gCfgMgr["StatusServer"]["Port"];

        // 使用配置中的连接池大小或默认值
        size_t poolSize = std::stoi(gCfgMgr["StatusServer"]["PoolSize"]);

        pool_ = std::make_unique<StatusConPool>(poolSize, host, port);
        std::cout << "StatusGrpcClient 已被初始化，大小为: " << poolSize << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << "Failed to initialize StatusGrpcClient: " << e.what() << std::endl;
        throw; // 重新抛出异常，表明初始化失败
    }
}