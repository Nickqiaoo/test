#include "KafkaProducer.h"
#include "log.h"

KafkaProducer::KafkaProducer(const std::string& broker, const std::string& topic)
    : broker_(broker), topic_(topic), conf_(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL)) {
    std::string errstr;
    if (conf_->set("bootstrap.servers", broker, errstr) != RdKafka::Conf::CONF_OK) {
        LOG_INFO("init broker error: {}", errstr);
    }
    if (conf_->set("dr_cb", &cb_, errstr) != RdKafka::Conf::CONF_OK) {
        LOG_INFO("set callback error:{}",errstr);
    }
    producer_ = std::shared_ptr<RdKafka::Producer>(RdKafka::Producer::create(conf_.get(), errstr));
    if (!producer_.get()) {
        LOG_INFO("create kafka produce error:{}", errstr);
    }
}

KafkaProducer::~KafkaProducer() {}

void KafkaProducer::Produce(const std::string& key, const std::string& value, Callback* cb) {
    //LOG_INFO("kafka produce message:{}", value);
    RdKafka::ErrorCode err = producer_->produce(topic_, RdKafka::Topic::PARTITION_UA, RdKafka::Producer::RK_MSG_COPY,
                                                const_cast<char*>(value.c_str()), value.size(), NULL, 0, 0, NULL, cb);
}