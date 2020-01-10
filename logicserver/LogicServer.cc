#include "LogicServer.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "logic.pb.h"
#include "util.hpp"

using namespace std::placeholders;

LogicServer::LogicServer(int thrnum, const std::string& httpip, int httpport, const std::string& rpcip, int rpcport, const std::string& brokers,
                         const std::string& topic)
    : httpserver_(thrnum, httpip, httpport), rpcserver_(thrnum, rpcip, rpcport), kafkaproducer_(brokers, topic), logicservice_(this) {}
LogicServer::~LogicServer() {}

void LogicServer::Start() {
    httpserver_.RegHandler("/push", std::bind(&LogicServer::PushMsgByKeysHandler, this, _1, _2));
    httpserver_.start();
    rpcserver_.registerService(&logicservice_);
    rpcserver_.start();
}

void LogicServer::PushMsgByKeysHandler(const HttpRequest& request, HttpResponsePtr response) {
    std::string query = request.query();
    std::vector<std::string> parmlist;
    common::split(query, "&", parmlist);
    std::vector<std::string> keys;
    int operation = 0;
    for (auto it : parmlist) {
        auto pos = it.find('=');
        if (pos != std::string::npos) {
            std::string op = it.substr(0, pos);
            std::string parm = it.substr(pos + 1);
            if (op == "mids") {
                common::split(parm, ",", keys);
            } else if (op == "operation") {
                operation = stoi(parm);
            }
        }
    }
    if (!keys.empty() && operation != 0) {
        // response->delay();
        LOG_INFO("http request key: {} operation: {}", keys[0], operation);
        PushMsgByKeys(keys, operation, request.body());
    } else {
        response->setStatusCode(HttpResponse::k404NotFound);
        response->setStatusMessage("NotFound");
        response->send();
    }
}

void LogicServer::PushMsgByKeys(const vector<std::string>& keys, int op, const string& msg) {
    PushMsg(keys, op, "gate1", msg);
}

void LogicServer::PushMsg(const vector<std::string>& keys, int op, const std::string& server, const string& msg) {
    logic::PushMsg pushmsg;
    pushmsg.set_type(logic::PushMsg::PUSH);
    pushmsg.set_operation(op);
    pushmsg.set_server("gate1");
    pushmsg.set_room("1");
    for (auto& it : keys) {
        pushmsg.add_keys(it);
    }
    pushmsg.set_msg(msg);

    kafkaproducer_.Produce(keys[0], pushmsg.SerializeAsString());
}