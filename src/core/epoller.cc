//
// Summary: server core source code.
//
// Author: Tony.
// Email: tonyjobmails@gmail.com.
// Last modify: 2012-08-27 15:42:32.
// File name: epoller.cc
//
// Description: 
// Define class Epoller.
// Simple wrapper of epoll.
//

#include "core/epoller.h"

#include <errno.h>

#include "core/channel.h"
#include "core/event_loop.h"
#include "core/base/logging.h"

namespace core {


Epoller::Epoller(EventLoop *loop)
       : loop_(loop),
         epoll_fd_(::epoll_create(1024)) {
  if(this->epoll_fd_ < 0) {
    CoreLog(ERROR, "%s:%d (%s) Failed in epoll_create.", __FILE__, __LINE__, __FUNCTION__);
  }
}

Epoller::~Epoller() {
  ::close(this->epoll_fd_);
}

int Epoller::Poll(struct epoll_event *events,
       int max_events, int timeout_milliseconds) {
  return ::epoll_wait(this->epoll_fd_, events,
         max_events, timeout_milliseconds);
}

void Epoller::AddChannel(Channel *channel) {
  if(channel->CheckStateInit() == true || channel->CheckStateDeleted() == true) {
    this->Update(EPOLL_CTL_ADD, channel);
    channel->SetStateAdded();
  } else {
    CoreLog(ERROR, "%s:%d (%s) Failed, channel state is not initialization or deleted.", __FILE__, __LINE__, __FUNCTION__);
  }
}

void Epoller::RemoveChannel(Channel *channel) {
  if(channel->CheckStateAdded() == true) {
    this->Update(EPOLL_CTL_DEL, channel);
    channel->SetStateDeleted();
  } else {
    CoreLog(ERROR, "%s:%d (%s) Failed, channel state is not Add.", __FILE__, __LINE__, __FUNCTION__);
  }
}

void Epoller::UpdateChannel(Channel *channel) {
  if(channel->CheckStateAdded() == true) {
    this->Update(EPOLL_CTL_MOD, channel);
  } else {
    CoreLog(ERROR, "%s:%d (%s) Failed, channel state is not Add.", __FILE__, __LINE__, __FUNCTION__);
  }
}

void Epoller::Update(int operation, Channel *channel) {
  struct epoll_event event;
  memset(&event, 0, sizeof(event));
  event.events = channel->GetEvent();
  event.data.ptr = channel;
  if(::epoll_ctl(this->epoll_fd_, operation, channel->GetFd(), &event) < 0) {
    CoreLog(ERROR, "%s:%d (%s) Failed to epoll_ctl.", __FILE__, __LINE__, __FUNCTION__);
  }
}


}  // namespace core

