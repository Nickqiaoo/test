#pragma once

#include "Session.h"
#include "HttpContext.h"

class HttpSession : public Session {

  public:
    explicit HttpSession(LoopPtr loop, uint64_t id) : Session(loop, id) {}
    ~HttpSession() {}
    HttpContext* getContext(){
      return &context_;
    }
  private:
    HttpContext context_;
};