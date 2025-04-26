#pragma once
#include "const.h"
#include <thread>
#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/cppconn/exception.h>
#include "data.h"
#include <memory>
#include <queue>
#include <mutex>
#include <chrono>
class SqlConnection {
public:
	SqlConnection(sql::Connection* con, int64_t lasttime):_con(con), _last_oper_time(lasttime){}
	std::unique_ptr<sql::Connection> _con;
	int64_t _last_oper_time;
};

class MySqlPool {
public:
    MySqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int poolSize)
        : url_(url), user_(user), pass_(pass), schema_(schema), poolSize_(poolSize), b_stop_(false) 
    {
        try {
            for (int i = 0; i < poolSize_; ++i) {
                sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
                auto* con = driver->connect(url_, user_, pass_);
                con->setSchema(schema_);
                // 获取当前时间戳
                auto currentTime = std::chrono::system_clock::now().time_since_epoch();
                // 将时间戳转换为秒
                long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
                pool_.push(std::make_unique<SqlConnection>(con, timestamp));
            }

            _check_thread = std::thread([this]() {
                while (!b_stop_) {
                    checkConnection();
                    std::this_thread::sleep_for(std::chrono::seconds(60));
                }
                });
        }
        catch (sql::SQLException& e) {
            // 处理异常
            std::cout << "mysql pool init failed, error is " << e.what() << std::endl;
        }
    }

    void checkConnection() {
        std::lock_guard<std::mutex> guard(mutex_);
        int poolsize = pool_.size();
        // 获取当前时间戳
        auto currentTime = std::chrono::system_clock::now().time_since_epoch();
        // 将时间戳转换为秒
        long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();

        std::queue<std::unique_ptr<SqlConnection>> temp_queue;

        // 处理所有连接
        for (int i = 0; i < poolsize; i++) {
            auto con = std::move(pool_.front());
            pool_.pop();

            if (timestamp - con->_last_oper_time < 5) {
                temp_queue.push(std::move(con));
                continue;
            }

            try {
                std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
                stmt->executeQuery("SELECT 1");
                con->_last_oper_time = timestamp;
                temp_queue.push(std::move(con));
            }
            catch (sql::SQLException& e) {
                std::cout << "Error keeping connection alive: " << e.what() << std::endl;
                // 重新创建连接并替换旧的连接
                try {
                    sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
                    auto* newcon = driver->connect(url_, user_, pass_);
                    newcon->setSchema(schema_);
                    con->_con.reset(newcon);
                    con->_last_oper_time = timestamp;
                    temp_queue.push(std::move(con));
                }
                catch (sql::SQLException& e) {
                    std::cout << "Failed to create new connection: " << e.what() << std::endl;
                    // If we can't create a new connection, don't push it back
                }
            }
        }

        // 恢复队列
        while (!temp_queue.empty()) {
            pool_.push(std::move(temp_queue.front()));
            temp_queue.pop();
        }
    }

    std::unique_ptr<SqlConnection> getConnection() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] {
            if (b_stop_) {
                return true;
            }
            return !pool_.empty();
            });

        if (b_stop_) {
            return nullptr;
        }

        std::unique_ptr<SqlConnection> con(std::move(pool_.front()));
        pool_.pop();
        return con;
    }

    void returnConnection(std::unique_ptr<SqlConnection> con) {
        if (con == nullptr) {
            return;
        }

        std::unique_lock<std::mutex> lock(mutex_);
        if (b_stop_) {
            return;
        }
        pool_.push(std::move(con));
        lock.unlock(); // Unlock before notification
        cond_.notify_one();
    }

    void Close() {
        b_stop_ = true;

        if (_check_thread.joinable()) {
            _check_thread.join();
        }

        cond_.notify_all();
    }

    ~MySqlPool() {
        Close();
        std::unique_lock<std::mutex> lock(mutex_);
        while (!pool_.empty()) {
            pool_.pop();
        }
    }

private:
    std::string url_;
    std::string user_;
    std::string pass_;
    std::string schema_;
    int poolSize_;
    std::queue<std::unique_ptr<SqlConnection>> pool_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::atomic<bool> b_stop_;
    std::thread _check_thread;
};



class MysqlDao
{
public:
	MysqlDao();
	~MysqlDao();
	int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
	bool CheckEmail(const std::string& name, const std::string & email);
	bool UpdatePwd(const std::string& name, const std::string& newpwd);
	bool CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo);
	std::shared_ptr<UserInfo> GetUser(int uid);
private:
	std::unique_ptr<MySqlPool> pool_;
};


