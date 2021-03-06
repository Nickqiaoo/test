#pragma once

#include <unordered_map>
#include "TcpServer.h"
#include "Limiter.h"

class HttpRequest;
class HttpResponse;

class HttpServer {
    typedef std::function<void(const HttpRequest&, HttpResponsePtr)> HttpCallback;

   public:
    HttpServer(int thrnum, const std::string& ip, int port);
    ~HttpServer();
    void start();
    void stop();
    void RegHandler(const std::string& url, const HttpCallback& cb) { handler_.insert({url, cb}); }
    void doResponse(HttpResponsePtr response);

   private:
    void onConnection(const SessionPtr& conn);
    void onMessage(const SessionPtr, Buffer*);
    void onRequest(const HttpSessionPtr& conn, const HttpRequest& req);

   private:
    TcpServer tcpserver_;
    Limiter limiter_;
    std::unordered_map<std::string, HttpCallback> handler_;
};