#pragma once
#include "GateRpcClient.h"
#include "KafkaConsumer.h"
#include "logic.pb.h"

class JobServer {
   public:
    JobServer(const std::string& brokers, const std::string& topic, const std::string& clientip, int clientport)
        : rpcclient_(clientip, clientport, this, &loop_), kafkaconsumer_(brokers, topic) {
        kafkaconsumer_.setMseeageCallback(std::bind(&JobServer::HandleKafkaMessage, this, _1, _2));
    }
    ~JobServer(){};

    void Start() {
        rpcclient_.connect();
        loop_.start();
    }
    void Stop() {
        kafkaconsumer_.Stop();
        loop_.stop();
    }

    void StartConsum() { kafkaconsumer_.Start(); }

    void HandleKafkaMessage(RdKafka::Message* message, void* opaque);

   private:
    void push(const logic::PushMsg& msg);
    void broadcastRoom(const std::string& room, const std::string& msg);
    void broadcast(int32_t operation, const std::string& msg, int32_t speed);
    void pushKeys(int32_t operation, const string& server, const google::protobuf::RepeatedPtrField<std::string>& keys, const std::string& msg);
    void HandlePushMsg(gate::PushMsgReply* response);

   private:
    Loop loop_;
    GateRpcClient rpcclient_;
    KafkaConsumer kafkaconsumer_;
};