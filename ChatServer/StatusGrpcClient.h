#pragma once
#include <queue>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "message.pb.h"
#include "const.h"
#include "ConfigMgr.h"
#include "Singleton.h"
#include "Defer.h"

using grpc::Channel; //通道
using grpc::Status; // 结果
using grpc::ClientContext; // 上下文

using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::LoginReq;
using message::LoginRsp;
using message::StatusService;

class StatusConPool {
public:
    StatusConPool(size_t poolSize, const std::string& host, const std::string& port)
        : poolSize_(poolSize), host_(host), port_(port), stop_(false) {
        initializePool();
    }

    ~StatusConPool() {
        close();
    }

    std::unique_ptr<StatusService::Stub> getConnection() {
        std::unique_lock<std::mutex> lock(mutex_);
        // 等待可用连接或池被关闭
        // 回调函数为true或超时时唤醒进程（其他线程修改了数据并notify）
        // 等待时释放锁lock，被唤醒后上锁
        bool success = cond_.wait_for(lock, std::chrono::seconds(30), [this] {
            return stop_ || !connections_.empty();
            });

        // 超时或池已关闭
        if (!success || stop_) {
            return nullptr;
        }

        // 获取连接
        auto stub = std::move(connections_.front());
        connections_.pop();
        return stub;
    }

    void returnConnection(std::unique_ptr<StatusService::Stub> stub) {
        if (!stub) return;

        std::lock_guard<std::mutex> lock(mutex_);
        if (stop_) return;

        connections_.push(std::move(stub));
        cond_.notify_one();
    }

    void close() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stop_) return;

        stop_ = true;
        while (!connections_.empty()) {
            connections_.pop();
        }
        cond_.notify_all(); // 唤醒所有等待的线程
    }

private:
    void initializePool() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (size_t i = 0; i < poolSize_; ++i) {
            std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(
                host_ + ":" + port_, grpc::InsecureChannelCredentials());
            connections_.push(StatusService::NewStub(channel));
        }
    }

    size_t poolSize_;
    std::string host_;
    std::string port_;
    std::atomic<bool> stop_;
    std::queue<std::unique_ptr<StatusService::Stub>> connections_;
    mutable std::mutex mutex_;
    std::condition_variable cond_;
};

class StatusGrpcClient : public Singleton<StatusGrpcClient>
{
    friend class Singleton<StatusGrpcClient>;
public:
    // 添加完整的析构函数实现
    ~StatusGrpcClient() {
        if (pool_) {
            pool_->close();
        }
        std::cout << "StatusGrpcClient 已被销毁" << std::endl;
    }
    GetChatServerRsp GetChatServer(int uid);
	LoginRsp Login(int uid, std::string token);
private:
    StatusGrpcClient();
    std::unique_ptr<StatusConPool> pool_;
};