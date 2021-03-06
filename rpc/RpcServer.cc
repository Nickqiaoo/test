#include "RpcServer.h"
#include "RpcChannel.h"
#include "RpcSession.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/service.h>

using namespace std::placeholders;

RpcServer::RpcServer(int thrnum, const std::string& ip, int port) : server_(thrnum, ip, port) {
    server_.setConnectionCallback(std::bind(&RpcServer::onConnection, this, _1));
}

void RpcServer::registerService(google::protobuf::Service* service) {
    const google::protobuf::ServiceDescriptor* desc = service->GetDescriptor();
    services_[desc->full_name()] = service;
}

void RpcServer::start() {
    server_.setNewSessionCallback<RpcSession>();
    server_.start();
}

void RpcServer::stop() { server_.stop(); }

void RpcServer::onConnection(const SessionPtr& conn) {
    RpcChannel* channel = new RpcChannel(conn);
    channel->setServices(&services_);
    conn->setMessageCallback(std::bind(&RpcChannel::onMessage, channel, _1, _2));
    auto rpcsessionptr = static_pointer_cast<RpcSession>(conn);
    rpcsessionptr->setChannel(channel);
}
