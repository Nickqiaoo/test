#pragma once

#include "Loop.h"
#include "RpcChannel.h"
#include "log.h"
#include "logic.pb.h"

#include <stdio.h>
#include <unistd.h>
#include <functional>

class GateServer;
using namespace std::placeholders;

class LogicRpcClient {
   public:
    LogicRpcClient(const std::string& ip, int port, GateServer* server, Loop* loop)
        : ip_(ip), port_(port), gateserver_(server), session_(std::make_shared<Session>(loop, 0)), channel_(new RpcChannel), stub_(channel_.get()) {
        session_->setConnectionCallback(std::bind(&LogicRpcClient::onConnection, this, _1));
        session_->setMessageCallback(std::bind(&RpcChannel::onMessage, channel_.get(), _1, _2));
        // client_.enableRetry();
    }
    ~LogicRpcClient() {
        LOG_INFO("rpcclient destroy");
    }

    void connect() { session_->connect(ip_, port_); }

    void Connect(logic::ConnectReq* request, logic::ConnectReply* response, google::protobuf::Closure* done);

    void HeartBeat(logic::HeartbeatReq* request, logic::HeartbeatReply* response, google::protobuf::Closure* done);

    void Disconnect(logic::DisconnectReq* request, logic::DisconnectReply* response, google::protobuf::Closure* done);

    bool Connected() { return isconnected_; }

   private:
    void onConnection(const SessionPtr& conn) {
        isconnected_ = true;
        LOG_INFO("client onConnection");
        channel_->setConnection(conn);
    }

   private:
    std::string ip_;
    int port_;
    bool isconnected_{false};
    GateServer* gateserver_;
    SessionPtr session_;
    RpcChannelPtr channel_;
    logic::Logic_Stub stub_;
};