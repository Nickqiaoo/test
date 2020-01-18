#include <hiredis/hiredis.h>
#include <string>

class RedisClient {
   public:
    RedisClient(const std::string& ip, const int port, const std::string& passwd = "", const int db = 0) : ip_(ip), port_(port), passwd_(passwd), db_(db) {}
    ~RedisClient() {}

    bool Connect();
    std::shared_ptr<redisReply> Execute(const std::vector<std::string>& cmd);
    void PipeLine(const std::vector<std::string>& cmd);

   private:
    bool Auth();
    bool Ping();
    bool Select();
    void Reset();
    bool Active();

   private:
    redisContext* context_;
    std::string ip_;
    int port_{0};
    std::string passwd_;
    int db_{0};
};