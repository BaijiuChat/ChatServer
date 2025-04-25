#pragma once
#include "const.h"
#include <hiredis/hiredis.h>
#include <queue>
#include <atomic>
#include <mutex>
#include "Singleton.h"
class RedisConPool {
public:
    RedisConPool(size_t poolSize, const char* host, int port, const char* pwd)
        : poolSize_(poolSize), host_(host), port_(port), b_stop_(false), pwd_(pwd) {
        for (size_t i = 0; i < poolSize_; ++i) {
            auto* context = redisConnect(host, port);
            if (context == nullptr || context->err != 0) {
                if (context != nullptr) {
                    redisFree(context);
                }
                continue;
            }

            auto reply = (redisReply*)redisCommand(context, "AUTH %s", pwd);
            if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
                std::cout << "认证失败" << std::endl;
                if (reply) {
                    freeReplyObject(reply);
                }
                redisFree(context);
                continue;
            }

            freeReplyObject(reply);
            std::cout << "认证成功" << std::endl;
            connections_.push(context);
        }

        check_thread_ = std::thread([this]() {
            while (!b_stop_) {
                std::this_thread::sleep_for(std::chrono::seconds(60)); // 每隔 60 秒发送一次 PING 命令
                if (b_stop_) break;
                checkThread();
            }
            });
    }

    ~RedisConPool() {
        Close();
        std::lock_guard<std::mutex> lock(mutex_);
        while (!connections_.empty()) {
            auto* context = connections_.front();
            redisFree(context);
            connections_.pop();
        }
    }

    redisContext* getConnection() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] {
            if (b_stop_) {
                return true;
            }
            return !connections_.empty();
            });

        if (b_stop_) {
            return nullptr;
        }

        auto* context = connections_.front();
        connections_.pop();
        return context;
    }

    void returnConnection(redisContext* context) {
        if (context == nullptr) {
            return;
        }

        std::unique_lock<std::mutex> lock(mutex_);
        if (b_stop_) {
            redisFree(context);
            return;
        }
        connections_.push(context);
        lock.unlock(); // Unlock before notification
        cond_.notify_one();
    }

    void Close() {
        b_stop_ = true;
        if (check_thread_.joinable()) {
            check_thread_.join();
        }
        cond_.notify_all();
    }

private:
    void checkThread() {
        std::queue<redisContext*> temp_queue;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto pool_size = connections_.size();
            for (size_t i = 0; i < pool_size; i++) {
                auto* context = connections_.front();
                connections_.pop();

                auto reply = (redisReply*)redisCommand(context, "PING");
                if (reply != nullptr) {
                    freeReplyObject(reply);
                    temp_queue.push(context);
                }
                else {
                    // Connection failed, create new one
                    redisFree(context);
                    auto* new_context = redisConnect(host_, port_);
                    if (new_context == nullptr || new_context->err != 0) {
                        if (new_context != nullptr) {
                            redisFree(new_context);
                        }
                        continue;
                    }

                    auto new_reply = (redisReply*)redisCommand(new_context, "AUTH %s", pwd_);
                    if (new_reply == nullptr || new_reply->type == REDIS_REPLY_ERROR) {
                        std::cout << "认证失败" << std::endl;
                        if (new_reply) {
                            freeReplyObject(new_reply);
                        }
                        redisFree(new_context);
                        continue;
                    }

                    freeReplyObject(new_reply);
                    std::cout << "认证成功" << std::endl;
                    temp_queue.push(new_context);
                }
            }

            // Restore connections
            while (!temp_queue.empty()) {
                connections_.push(temp_queue.front());
                temp_queue.pop();
            }
        }
    }

    std::atomic<bool> b_stop_;
    size_t poolSize_;
    const char* host_;
    const char* pwd_;
    int port_;
    std::queue<redisContext*> connections_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::thread check_thread_;
};

class RedisMgr: public Singleton<RedisMgr>, 
	public std::enable_shared_from_this<RedisMgr>
{
	friend class Singleton<RedisMgr>;
public:
	~RedisMgr();
	bool Get(const std::string &key, std::string& value);
	bool Set(const std::string &key, const std::string &value);
	bool LPush(const std::string &key, const std::string &value);
	bool LPop(const std::string &key, std::string& value);
	bool RPush(const std::string& key, const std::string& value);
	bool RPop(const std::string& key, std::string& value);
	bool HSet(const std::string &key, const std::string  &hkey, const std::string &value);
	bool HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen);
	std::string HGet(const std::string &key, const std::string &hkey);
	bool Del(const std::string &key);
	bool ExistsKey(const std::string &key);
	void Close() {
		_con_pool->Close();
	}
private:
	RedisMgr();
	unique_ptr<RedisConPool>  _con_pool;
};

