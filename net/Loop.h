#pragma once
#include "net_define.h"
#include <thread>

class Loop {
    typedef std::shared_ptr<asio::io_service> iosptr;
    typedef std::shared_ptr<asio::io_service::work> workptr;

  public:
    explicit Loop();
    ~Loop();

    void start();
    void stop();
    void runInLoop();

  private:
    void run();

  private:
    iosptr ios_;
    workptr work_;
    std::thread thread_;
};