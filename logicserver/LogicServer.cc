#include "LogicServer.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "logic.pb.h"
#include "util.hpp"

using namespace std::placeholders;

LogicServer::LogicServer(int thrnum, const std::string& httpip, int httpport, const std::string& rpcip, int rpcport, const std::string& redisip, int redisport,
                         const std::string& brokers, const std::string& topic, int serverid)
    : serverid_(serverid),
      httpserver_(thrnum, httpip, httpport),
      rpcserver_(thrnum, rpcip, rpcport),
      redisclient_(redisip, redisport),
      kafkaproducer_(brokers, topic),
      logicservice_(this, redisip, redisport) {
    if (!redisclient_.Connect()) {
        LOG_ERROR("redis connect error");
    }
    loop_.runInLoop([=] { kafkaproducer_.Poll(); });
}
LogicServer::~LogicServer() {}

void LogicServer::Start() {
    httpserver_.RegHandler("/push/keys", std::bind(&LogicServer::PushMsgByKeysHandler, this, _1, _2));
    httpserver_.RegHandler("/push/mids", std::bind(&LogicServer::PushMsgByMidsHandler, this, _1, _2));
    httpserver_.RegHandler("/push/room", std::bind(&LogicServer::PushMsgByRoomHandler, this, _1, _2));
    httpserver_.RegHandler("/push/all", std::bind(&LogicServer::PushMsgToAllHandler, this, _1, _2));
    httpserver_.start();
    rpcserver_.registerService(&logicservice_);
    rpcserver_.start();
    loop_.start();
}

void LogicServer::Stop() {
    httpserver_.stop();
    rpcserver_.stop();
}

void LogicServer::PushMsgByKeysHandler(const HttpRequest& request, HttpResponsePtr response) {
    std::vector<std::string> keys;
    int operation = stoi(request.getQuery("operation"));
    common::split(request.getQuery("keys"), ",", keys);

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

void LogicServer::PushMsgByMidsHandler(const HttpRequest& request, HttpResponsePtr response) {
    std::vector<std::string> mids;
    std::vector<int32_t> midsvec;
    int operation = stoi(request.getQuery("operation"));
    common::split(request.getQuery("mids"), ",", mids);
    for (auto elem : mids) {
        midsvec.push_back(stoi(elem));
    }
    if (!mids.empty() && operation != 0) {
        response->delay();
        LOG_INFO("http request key: {} operation: {}", mids[0], operation);
        Callback* cb = (Callback*)(response->getSessionPtr()->GetContext());
        PushMsgByMids(midsvec, operation, request.body(), cb);
    } else {
        response->setStatusCode(HttpResponse::k404NotFound);
        response->setStatusMessage("NotFound");
        response->send();
    }
}

void LogicServer::PushMsgByRoomHandler(const HttpRequest& request, HttpResponsePtr response) {
    int operation = stoi(request.getQuery("operation"));
    std::string type = request.getQuery("type");
    std::string room = request.getQuery("room");

    std::string roomstr = fmt::format("{}://{}", type, room);
    if (!type.empty() && operation != 0 && !room.empty()) {
        // response->delay();
        LOG_INFO("http request room: {} operation: {}", roomstr, operation);
        PushMsgByRoom(roomstr, operation, request.body());
    } else {
        response->setStatusCode(HttpResponse::k404NotFound);
        response->setStatusMessage("NotFound");
        response->send();
    }
}

void LogicServer::PushMsgToAllHandler(const HttpRequest& request, HttpResponsePtr response) {
    int operation = stoi(request.getQuery("operation"));
    int speed = stoi(request.getQuery("speed"));
    if (operation != 0 && speed != 0) {
        // response->delay();
        LOG_INFO("http request all speed: {} operation: {}", speed, operation);
        PushMsgToAll(speed, operation, request.body());
    } else {
        response->setStatusCode(HttpResponse::k404NotFound);
        response->setStatusMessage("NotFound");
        response->send();
    }
}

void LogicServer::PushMsgByKeys(const std::vector<std::string>& keys, int op, const string& msg) {
    std::vector<std::string> cmd;
    for (auto it : keys) {
        cmd.emplace_back(logicservice_.keyKeyServer(it));
    }
    auto reply = redisclient_.MGet(cmd);
    std::unordered_map<std::string, std::vector<std::string>> serverKeys;
    for (size_t i = 0; i < reply.size(); i++) {
        serverKeys[reply[i]].push_back(cmd[i]);
    }

    for (auto elem : serverKeys) {
        PushMsg(elem.second, op, elem.first, msg, nullptr);
    }
}

void LogicServer::PushMsgByMids(const std::vector<int32_t>& mids, int op, const string& msg, Callback* cb) {
    std::unordered_map<std::string, std::vector<std::string>> serverKeys;
    std::unordered_map<std::string, std::string> keyServer;
    for (auto it : mids) {
        auto reply = redisclient_.HGetAll(logicservice_.keyMidServer(it));  // key -> server
        for (auto& elem : reply) {
            keyServer[elem.first] = elem.second;
        }
    }
    for (auto& elem : keyServer) {
        serverKeys[elem.second].push_back(elem.first);  // server -> key[]
    }

    for (auto elem : serverKeys) {
        PushMsg(elem.second, op, elem.first, msg, cb);
    }
}

void LogicServer::PushMsgToAll(int speed, int op, const std::string& msg) {
    logic::PushMsg pushmsg;
    pushmsg.set_type(logic::PushMsg::BROADCAST);
    pushmsg.set_operation(op);
    pushmsg.set_speed(speed);
    pushmsg.set_msg(msg);

    kafkaproducer_.Produce(std::to_string(op), pushmsg.SerializeAsString(), nullptr);
}

void LogicServer::PushMsgByRoom(const std::string& room, int op, const std::string& msg) {
    logic::PushMsg pushmsg;
    pushmsg.set_type(logic::PushMsg::ROOM);
    pushmsg.set_room(room);
    pushmsg.set_operation(op);
    pushmsg.set_msg(msg);

    kafkaproducer_.Produce(room, pushmsg.SerializeAsString(), nullptr);
}

void LogicServer::PushMsg(const vector<std::string>& keys, int op, const std::string& server, const string& msg, Callback* cb) {
    logic::PushMsg pushmsg;
    pushmsg.set_type(logic::PushMsg::PUSH);
    pushmsg.set_operation(op);
    pushmsg.set_server(server);
    for (auto& it : keys) {
        pushmsg.add_keys(it);
    }
    pushmsg.set_msg(msg);

    kafkaproducer_.Produce(keys[0], pushmsg.SerializeAsString(), cb);
}

int64_t LogicServer::getMilliSecond() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string LogicServer::generateKey() {
    static uint64_t lastTimestamp = getMilliSecond();
    static uint16_t seqID = 0;
    uint64_t timestamp = getMilliSecond();
    if (lastTimestamp == timestamp) {
        seqID = (seqID + 1) & 0xFFF;
        if (seqID == 0) {
            while (lastTimestamp == timestamp) timestamp = getMilliSecond();
        };
    } else {
        seqID = 0;
    }
    lastTimestamp = timestamp;
    uint64_t id = (timestamp & 0x3FFFFFFFFFF) << 22 | (serverid_ & 0x3FF) << 12 | seqID;
    return std::to_string(id);
}