#include "VerifyGrpcClient.h"
#include "ConfigMgr.h"

VerifyGrpcClient::VerifyGrpcClient() {
	auto& gCfgMgr = ConfigMgr::Inst();
	std::string host = gCfgMgr["VerifyServer"]["Host"];
	std::string port = gCfgMgr["VerifyServer"]["Port"];
	pool_.reset(new RPConPool(5, host, port));
	// 添加详细日志
	std::cout << "正在初始化 gRPC 连接池至 " << host << ":" << port << std::endl;
}