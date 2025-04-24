#include "RedisMgr.h"

RedisMgr::RedisMgr()
{
    auto& gCfgMgr = ConfigMgr::Inst();
    auto host = gCfgMgr["Redis"]["Host"];
    auto port = gCfgMgr["Redis"]["Port"];
    auto pwd = gCfgMgr["Redis"]["Passwd"];
    _con_pool.reset(new RedisConPool(5, host.c_str(), atoi(port.c_str()), pwd.c_str()));
    std::cout << "RedisMgr 初始化成功" << std::endl;
}

RedisMgr::~RedisMgr()
{
    std::cout << "RedisMgr 开始销毁..." << std::endl;
    Close();
    std::cout << "RedisMgr 销毁完成" << std::endl;
}

bool RedisMgr::Get(const std::string& key, std::string& value)
{
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) {
        return false;
    }

    auto reply = (redisReply*)redisCommand(connect, "GET %s", key.c_str());
    if (reply == NULL) {
        std::cout << "[ GET  " << key << " ] failed" << std::endl;
        _con_pool->returnConnection(connect);
        return false;
    }

    bool success = false;
    if (reply->type != REDIS_REPLY_STRING) {
        std::cout << "[ GET  " << key << " ] failed" << std::endl;
    }
    else {
        value = reply->str;
        std::cout << "Succeed to execute command [ GET " << key << "  ]" << std::endl;
        success = true;
    }

    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return success;
}

bool RedisMgr::Set(const std::string& key, const std::string& value) {
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) {
        return false;
    }
    //执行redis命令行
    auto reply = (redisReply*)redisCommand(connect, "SET %s %s", key.c_str(), value.c_str());

    //如果返回NULL则说明执行失败
    if (NULL == reply)
    {
        std::cout << "Executing command [ SET " << key << "  " << value << " ] failure ! " << std::endl;
        _con_pool->returnConnection(connect);
        return false;
    }

    bool success = false;
    //如果执行失败则释放连接
    if (!(reply->type == REDIS_REPLY_STATUS && (strcmp(reply->str, "OK") == 0 || strcmp(reply->str, "ok") == 0)))
    {
        std::cout << "Executing command [ SET " << key << "  " << value << " ] failure ! " << std::endl;
    }
    else {
        std::cout << "Executing command [ SET " << key << "  " << value << " ] success ! " << std::endl;
        success = true;
    }

    //执行成功 释放redisCommand执行后返回的redisReply所占用的内存
    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return success;
}

bool RedisMgr::Auth(const std::string& password)
{
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) {
        return false;
    }

    auto reply = (redisReply*)redisCommand(connect, "AUTH %s", password.c_str());
    bool success = false;
    if (reply->type == REDIS_REPLY_ERROR) {
        std::cout << "认证失败" << std::endl;
    }
    else {
        std::cout << "认证成功" << std::endl;
        success = true;
    }

    //执行成功 释放redisCommand执行后返回的redisReply所占用的内存
    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return success;
}

bool RedisMgr::LPush(const std::string& key, const std::string& value)
{
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) {
        return false;
    }

    auto reply = (redisReply*)redisCommand(connect, "LPUSH %s %s", key.c_str(), value.c_str());
    if (NULL == reply)
    {
        std::cout << "Executing command [ LPUSH " << key << "  " << value << " ] failure ! " << std::endl;
        _con_pool->returnConnection(connect);
        return false;
    }

    bool success = false;
    if (reply->type != REDIS_REPLY_INTEGER || reply->integer <= 0) {
        std::cout << "Executing command [ LPUSH " << key << "  " << value << " ] failure ! " << std::endl;
    }
    else {
        std::cout << "Executing command [ LPUSH " << key << "  " << value << " ] success ! " << std::endl;
        success = true;
    }

    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return success;
}

bool RedisMgr::LPop(const std::string& key, std::string& value) {
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) {
        return false;
    }

    auto reply = (redisReply*)redisCommand(connect, "LPOP %s ", key.c_str());
    if (reply == nullptr || reply->type == REDIS_REPLY_NIL) {
        std::cout << "Executing command [ LPOP " << key << " ] failure ! " << std::endl;
        if (reply) freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }

    value = reply->str;
    std::cout << "Executing command [ LPOP " << key << " ] success ! " << std::endl;
    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}

bool RedisMgr::RPush(const std::string& key, const std::string& value) {
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) {
        return false;
    }

    auto reply = (redisReply*)redisCommand(connect, "RPUSH %s %s", key.c_str(), value.c_str());
    if (NULL == reply)
    {
        std::cout << "Executing command [ RPUSH " << key << "  " << value << " ] failure ! " << std::endl;
        _con_pool->returnConnection(connect);
        return false;
    }

    bool success = false;
    if (reply->type != REDIS_REPLY_INTEGER || reply->integer <= 0) {
        std::cout << "Executing command [ RPUSH " << key << "  " << value << " ] failure ! " << std::endl;
    }
    else {
        std::cout << "Executing command [ RPUSH " << key << "  " << value << " ] success ! " << std::endl;
        success = true;
    }

    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return success;
}

bool RedisMgr::RPop(const std::string& key, std::string& value) {
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) {
        return false;
    }

    auto reply = (redisReply*)redisCommand(connect, "RPOP %s ", key.c_str());
    if (reply == nullptr || reply->type == REDIS_REPLY_NIL) {
        std::cout << "Executing command [ RPOP " << key << " ] failure ! " << std::endl;
        if (reply) freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }

    value = reply->str;
    std::cout << "Executing command [ RPOP " << key << " ] success ! " << std::endl;
    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}

bool RedisMgr::HSet(const std::string& key, const std::string& hkey, const std::string& value) {
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) {
        return false;
    }

    auto reply = (redisReply*)redisCommand(connect, "HSET %s %s %s", key.c_str(), hkey.c_str(), value.c_str());
    if (reply == nullptr || reply->type != REDIS_REPLY_INTEGER) {
        std::cout << "Executing command [ HSet " << key << "  " << hkey << "  " << value << " ] failure ! " << std::endl;
        if (reply) freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }

    std::cout << "Executing command [ HSet " << key << "  " << hkey << "  " << value << " ] success ! " << std::endl;
    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}


bool RedisMgr::HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen)
{
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) {
        return false;
    }

    const char* argv[4];
    size_t argvlen[4];
    argv[0] = "HSET";
    argvlen[0] = 4;
    argv[1] = key;
    argvlen[1] = strlen(key);
    argv[2] = hkey;
    argvlen[2] = strlen(hkey);
    argv[3] = hvalue;
    argvlen[3] = hvaluelen;
    auto reply = (redisReply*)redisCommandArgv(connect, 4, argv, argvlen);
    if (reply == nullptr || reply->type != REDIS_REPLY_INTEGER) {
        std::cout << "Executing command [ HSet " << key << "  " << hkey << "  " << hvalue << " ] failure ! " << std::endl;
        if (reply) freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }

    std::cout << "Executing command [ HSet " << key << "  " << hkey << "  " << hvalue << " ] success ! " << std::endl;
    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}

std::string RedisMgr::HGet(const std::string& key, const std::string& hkey)
{
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) {
        return "";
    }

    const char* argv[3];
    size_t argvlen[3];
    argv[0] = "HGET";
    argvlen[0] = 4;
    argv[1] = key.c_str();
    argvlen[1] = key.length();
    argv[2] = hkey.c_str();
    argvlen[2] = hkey.length();
    auto reply = (redisReply*)redisCommandArgv(connect, 3, argv, argvlen);
    if (reply == nullptr || reply->type == REDIS_REPLY_NIL) {
        if (reply) freeReplyObject(reply);
        std::cout << "Executing command [ HGet " << key << " " << hkey << "  ] failure ! " << std::endl;
        _con_pool->returnConnection(connect);
        return "";
    }

    std::string value = reply->str;
    freeReplyObject(reply);
    std::cout << "Executing command [ HGet " << key << " " << hkey << " ] success ! " << std::endl;
    _con_pool->returnConnection(connect);
    return value;
}

bool RedisMgr::Del(const std::string& key)
{
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) {
        return false;
    }

    auto reply = (redisReply*)redisCommand(connect, "DEL %s", key.c_str());
    if (reply == nullptr || reply->type != REDIS_REPLY_INTEGER) {
        std::cout << "Executing command [ Del " << key << " ] failure ! " << std::endl;
        if (reply) freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }

    std::cout << "Executing command [ Del " << key << " ] success ! " << std::endl;
    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}

bool RedisMgr::ExistsKey(const std::string& key)
{
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) {
        return false;
    }

    auto reply = (redisReply*)redisCommand(connect, "exists %s", key.c_str());
    if (reply == nullptr || reply->type != REDIS_REPLY_INTEGER || reply->integer == 0) {
        std::cout << "Not Found [ Key " << key << " ]  ! " << std::endl;
        if (reply) freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }

    std::cout << " Found [ Key " << key << " ] exists ! " << std::endl;
    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}

void RedisMgr::Close()
{
    if (_con_pool) {
        _con_pool->Close();
        _con_pool.reset();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// RedisConPool 实现 /////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

RedisConPool::RedisConPool(size_t poolSize, const char* host, int port, const char* pwd)
    : poolSize_(poolSize), host_(host), port_(port), password_(pwd), b_stop_(false) {
    std::cout << "RedisConPool 开始初始化，连接数: " << poolSize << std::endl;
    for (size_t i = 0; i < poolSize_; ++i) {
        redisContext* context = createConnection();
        if (context) {
            connections_.push(context);
            std::cout << "创建Redis连接 #" << i + 1 << " 成功" << std::endl;
        }
        else {
            std::cout << "创建Redis连接 #" << i + 1 << " 失败" << std::endl;
        }
    }
    std::cout << "RedisConPool 初始化完成" << std::endl;
}

RedisConPool::~RedisConPool() {
    std::cout << "RedisConPool 开始销毁..." << std::endl;
    closeAllConnections();
    std::cout << "RedisConPool 销毁完成" << std::endl;
}

void RedisConPool::closeAllConnections() {
    // 先设置停止标志，阻止新的连接获取
    b_stop_ = true;
    cond_.notify_all();

    // 再清空队列中的所有连接
    std::lock_guard<std::mutex> lock(mutex_);
    while (!connections_.empty()) {
        redisContext* context = connections_.front();
        connections_.pop();
        if (context) {
            redisFree(context);
            std::cout << "释放一个Redis连接" << std::endl;
        }
    }
}

redisContext* RedisConPool::getConnection() {
    std::unique_lock<std::mutex> lock(mutex_);

    // 如果连接池已关闭，直接返回空
    if (b_stop_) {
        return nullptr;
    }

    bool success = cond_.wait_for(lock, std::chrono::seconds(5), [this] {
        return b_stop_ || !connections_.empty();
        });

    // 超时或池已关闭
    if (!success || b_stop_) {
        return nullptr;
    }

    redisContext* context = connections_.front();
    connections_.pop();

    // 检查连接是否还活着
    if (!isConnectionAlive(context)) {
        std::cout << "检测到Redis连接已断开，尝试重新连接" << std::endl;
        redisFree(context);
        context = createConnection();
    }

    return context;
}

void RedisConPool::returnConnection(redisContext* context) {
    if (!context) return;

    std::lock_guard<std::mutex> lock(mutex_);
    if (b_stop_) {
        redisFree(context);
        std::cout << "连接池已关闭，释放一个Redis连接" << std::endl;
        return;
    }

    // 检查连接是否有效
    if (isConnectionAlive(context)) {
        connections_.push(context);
    }
    else {
        std::cout << "返回了一个无效连接，释放并尝试创建新连接" << std::endl;
        redisFree(context);
        // 尝试创建新连接
        context = createConnection();
        if (context) {
            connections_.push(context);
        }
    }
    cond_.notify_one();
}

void RedisConPool::Close() {
    std::cout << "正在关闭Redis连接池..." << std::endl;
    closeAllConnections();
    std::cout << "Redis连接池已关闭" << std::endl;
}

redisContext* RedisConPool::createConnection() {
    redisContext* context = redisConnect(host_, port_);
    if (context == nullptr || context->err != 0) {
        if (context != nullptr) {
            std::cout << "Redis连接失败: " << context->errstr << std::endl;
            redisFree(context);
        }
        return nullptr;
    }

    // 设置超时
    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    redisSetTimeout(context, timeout);

    auto reply = (redisReply*)redisCommand(context, "AUTH %s", password_.c_str());
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        std::cout << "Redis认证失败: " << (reply ? reply->str : "unknown error") << std::endl;
        if (reply) {
            freeReplyObject(reply);
        }
        redisFree(context);
        return nullptr;
    }

    freeReplyObject(reply);
    std::cout << "Redis认证成功" << std::endl;
    return context;
}

// 检查连接是否有效
bool RedisConPool::isConnectionAlive(redisContext* context) {
    if (!context) {
        return false;
    }

    // 发送PING命令检查连接
    auto reply = (redisReply*)redisCommand(context, "PING");
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            freeReplyObject(reply);
        }
        return false;
    }
    freeReplyObject(reply);
    return true;
}