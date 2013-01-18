//
// Summary: server core source code.
//
// Author: Tony.
// Email: tonyjobmails@gmail.com.
// Last modify: 2012-08-27 15:40:31.
// File name: channel.cc
//
// Description: 
// Define class Channel
// A channel describes a I/O event with a fd(file descriptor), this fd can be
// a socket, a timer fd or a signal fd.
//

#include "core/channel.h"

#include <sys/epoll.h>

#include <sstream>

#include "core/event_loop.h"
#include "core/base/time_tick.h"

#ifdef EPOLLRDHUP
const int kEpollReadEventMask = EPOLLIN | EPOLLRDHUP;
#else  // EPOLLRDHUP
const int kEpollReadEventMask = EPOLLIN;
#endif  // EPOLLRDHUP

namespace core {

const int Channel::kReadEvent_ = EPOLLIN;
const int Channel::kWriteEvent_ = EPOLLOUT;
const int Channel::kNoneEvent_ = 0;

const int Channel::kStateInit = 0;
const int Channel::kStateAdded = 1;
const int Channel::kStateDeleted = -1;

Channel::Channel(EventLoop *loop, int fd)
       : fd_(fd), event_(0), happened_event_(0),
         state_(kStateInit), loop_(loop),
         event_handling_(false) {}

Channel::~Channel() {
}

void Channel::HandleEvent(const TimeTick &time_tick) {
  this->event_handling_ = true;

  if(this->happened_event_ & (kEpollReadEventMask)) {
    if(this->read_callback_) {
      this->read_callback_(time_tick);
    }
  }
  if(this->happened_event_ & EPOLLOUT) {
    if(this->write_callback_) {
      this->write_callback_();
    }
  }
  if(this->happened_event_ & EPOLLERR) {
    if(this->error_callback_) {
      this->error_callback_();
    }
  }

  this->event_handling_ = false;
}

std::string Channel::SerializeString() const {
  std::ostringstream oss;
  if(this->happened_event_ & EPOLLIN) {
    oss << "EPOLLIN ";
  } 
#ifdef EPOLLRDHUP
  if(this->happened_event_ & EPOLLRDHUP) {
    oss << "EPOLLRDHUP ";
  }
#endif // EPOLLRDHUP
  if(this->happened_event_ & EPOLLOUT) {
    oss << "EPOLLOUT ";
  }
  if(this->happened_event_ & EPOLLERR) {
    oss << "EPOLLERR ";
  }
  return oss.str();
}

void Channel::UpdateEvent() {
  if(this->CheckStateInit() || this->CheckStateDeleted()) {
    if(this->CheckNoneEvent() == false)
      this->loop_->AddChannel(this);
  } else if(this->CheckStateAdded()) {
    if(this->CheckNoneEvent())
      this->loop_->RemoveChannel(this);
    else
      this->loop_->UpdateChannel(this);
  }
}

}  // namespace core
