#pragma once
#include <iostream>
#include <memory>
#include <string>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "Singleton.h"
#include "const.h"  // 仅用于 ErrorCodes 枚举，可以考虑单独提取

using grpc::Channel; // 客户端与服务端之间的通信通道。
using grpc::ClientContext; // 用于配置和管理单个 RPC 调用的上下文。
using grpc::Status; //  RPC 调用的结果状态。

using message::GetVerifyReq;
using message::GetVerifyRsp;
using message::VerifyService;

class RPConPool {
public:
    RPConPool(size_t poolsize, std::string host, std::string port) :
        poolSize(poolsize), host_(host), port_(port), b_stop(false)
    {
        // 创建并配置ChannelArguments
        grpc::ChannelArguments args;
        args.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 10000);       // 设置保持连接的时间
        args.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 5000);     // 设置超时
        args.SetInt(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1); // 允许在没有RPC调用的情况下发送保活包

        for (size_t i = 0; i < poolSize; ++i) {
            auto channel = grpc::CreateCustomChannel(host_ + ":" + port_, grpc::InsecureChannelCredentials(), args);
            std::cout << "创建链接 " << i + 1 << " 至 " << host_ << ":" << port_ << std::endl;
            connections_.push(std::move(VerifyService::NewStub(channel)));
        }
        std::cout << "连接池初始化成功，有 " << poolSize << " 个连接" << std::endl;
    }

    ~RPConPool() {
        Close(); // 先关闭连接池，设置停止标志

        // 再清空连接队列
        std::lock_guard<std::mutex> lock(mutex_);
        while (!connections_.empty()) {
            connections_.pop();
        }
        std::cout << "连接池已销毁" << std::endl;
    }

    void Close() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (b_stop) return; // 如果已经关闭，则直接返回

        b_stop = true; // 设置停止标志
        cond_.notify_all(); // 通知所有等待的线程
        std::cout << "连接池已关闭" << std::endl;
    }

    std::unique_ptr<VerifyService::Stub> GetConnection(int timeout_ms = 5000) {
        std::unique_lock<std::mutex> lock(mutex_);

        // 如果已经关闭，直接返回nullptr
        if (b_stop) {
            return nullptr;
        }

        // 带超时的等待
        if (!cond_.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this]() {
            return b_stop || !connections_.empty();
            })) {
            // 策略1: 返回nullptr（由调用方处理）
            // return nullptr;

            // 策略2: 弹性新建连接（需确保线程安全）
            auto channel = grpc::CreateCustomChannel(
                host_ + ":" + port_,
                grpc::InsecureChannelCredentials(),
                grpc::ChannelArguments()
            );
            return VerifyService::NewStub(channel);
        }

        if (b_stop) return nullptr;
        auto stub = std::move(connections_.front());
        connections_.pop();
        return stub;
    }

    void ReturnConnection(std::unique_ptr<VerifyService::Stub> context) {
        if (!context) return; // 检查空指针

        std::lock_guard<std::mutex> lock(mutex_);
        if (!b_stop) {
            connections_.push(std::move(context));
            cond_.notify_one(); // 通知一个可能正在等待的线程
        }
        // 如果已停止，context会在函数结束时被销毁
    }

private:
    std::atomic<bool> b_stop;
    size_t poolSize;
    std::string host_;
    std::string port_;
    std::queue<std::unique_ptr<VerifyService::Stub>> connections_;
    std::condition_variable cond_;
    std::mutex mutex_;
};

// 这里是一个单例模式的实现，确保在整个程序中只有一个 VerifyGrpcClient 实例。
class VerifyGrpcClient : public Singleton<VerifyGrpcClient>
{
    friend class Singleton<VerifyGrpcClient>;
public:
    // 添加析构函数
    ~VerifyGrpcClient() {
        if (pool_) {
            pool_->Close();
        }
        std::cout << "VerifyGrpcClient 已被销毁" << std::endl;
    }

    // 修改 GetVerifyCode 方法处理连接获取失败的情况
    GetVerifyRsp GetVerifyCode(const std::string& email) {
        GetVerifyReq request;
        GetVerifyRsp reply;
        ClientContext context;

        auto stub = pool_->GetConnection(2000); // 给2秒超时时间
        if (!stub) {
            reply.set_error(ErrorCodes::RPCFailed);
            std::cout << "无法获取RPC连接" << std::endl;
            return reply;
        }

        request.set_email(email);
        Status status = stub->GetVerifyCode(&context, request, &reply);
        pool_->ReturnConnection(std::move(stub));

        if (!status.ok()) {
            reply.set_error(ErrorCodes::RPCFailed);
            std::cout << "RPC调用失败: " << status.error_code() << ": " << status.error_message() << std::endl;
        }

        return reply;
    }
private:
    VerifyGrpcClient();

    std::unique_ptr<RPConPool> pool_;
};