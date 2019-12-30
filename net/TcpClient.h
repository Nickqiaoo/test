#include "Session.h"
#include "net_define.h"

class TcpClient {
    TcpClient(LoopPtr loop, const std::string& ip, int port) : ip_(ip), port_(port), session_(std::make_shared<Session>(loop)) {}
    ~TcpClient();
    void connect() { session_->connect(ip_, port_); }
    void setMessageCallback(const onMessageCallback& cb) { messagecallback_ = cb; }
    void setConnectionCallback(const onConnectionCallback& cb) { connectioncallback_ = cb; }

   private:
    std::string ip_;
    int port_;
    SessionPtr session_;
    onConnectionCallback connectioncallback_;
    onMessageCallback messagecallback_;
};