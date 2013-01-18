//
// Summary: server core source code.
//
// Author: Tony.
// Email: tonyjobmails@gmail.com.
// Last modify: 2012-08-27 15:47:16.
// File name: tcp_connection.cc
//
// Description: 
// Define class TCPConnection.
//

#include <boost/bind.hpp>

#include "core/configure.h"
#include "core/event_loop.h"
#include "core/tcp_connection.h"
#include "core/tcp_socket.h"
#include "core/channel.h"
#include "core/base/auto_mutex.h"
#include "core/base/time_tick.h"

namespace core {

TCPConnection::TCPConnection(int sockfd, EventLoop *loop,
       const std::string & name, const InetAddress &local,
       const InetAddress &peer)
       : name_(name), state_(CONNECTING), loop_(loop),
         channel_(NULL), socket_(sockfd),
         local_address_(local), peer_address_(peer), 
         send_buffer_(Configure::GetTCPConnection()->buffer_unit_size_),
         receive_buffer_(Configure::GetTCPConnection()->buffer_unit_size_), 
         cache_(Configure::GetTCPConnection()->bound_queue_size_, kBlockingProducerPattern) {
  CoreLog(INFO, "TCPConnection[%p] constructor", this);
}

TCPConnection::~TCPConnection() {
  CoreLog(INFO, "~TCPConnection[%p] destructor", this);
  if(this->channel_) {
    delete this->channel_;
    this->channel_ = NULL;
  }
}

bool TCPConnection::Initialize() {
  assert(this->loop_);

  this->channel_ = new (std::nothrow) Channel(this->loop_, this->socket_.GetSockfd());
  if(this->channel_ == NULL) {
    CoreLog(ERROR, "%s:%d (%s) Failed in allocating Channel",
           __FILE__, __LINE__, __FUNCTION__);
    return false;
  }

  this->channel_->SetReadCallback(boost::bind(&TCPConnection::OnRead, this, _1));
  this->channel_->SetWriteCallback(boost::bind(&TCPConnection::OnWrite, this));
  this->channel_->SetCloseCallback(boost::bind(&TCPConnection::OnClose, this));
  this->channel_->SetErrorCallback(boost::bind(&TCPConnection::OnError, this));

  return true;
}

void TCPConnection::Shutdown() {
  this->RequireShutdown();
}

void TCPConnection::ConnectionEstablished() {
  assert(this->channel_);
  assert(this->state_ == CONNECTING);

  this->SetState(CONNECTED);

  this->send_buffer_.PreWrite(Configure::GetTCPConnection()->buffer_init_size_);
  this->receive_buffer_.PreWrite(Configure::GetTCPConnection()->buffer_init_size_);

  if(this->connection_callback_)
    this->connection_callback_(shared_from_this());

  this->channel_->EnableRead();
}

void TCPConnection::ConnectionDestoryed() {
  if(this->connection_callback_)
    this->connection_callback_(shared_from_this());
}

void TCPConnection::SendMessage(const void* message, size_t size) {
  if(this->state_ == CONNECTED) {
    if(this->loop_->CheckInLoopThread()) {
      if(this->send_buffer_.PreWrite(size) == true) {
        memcpy(this->send_buffer_.WritePointer(), message, size);
        this->send_buffer_.WriteShift(size);
        if(this->channel_->CheckWriteEvent() == false) { this->channel_->EnableWrite(); }
      } else {
        CoreLog(ERROR, "%s:%d (%s) PreWrite error.",
            __FILE__, __LINE__, __FUNCTION__);
      }
    } else {
      this->CacheMessage(message, size);
    }
  }
}

void TCPConnection::CacheMessage(const void *message, size_t size) {
  if(this->cache_.Add((const char *)message, size) != (int)size) {
    CoreLog(ERROR, "%s:%d (%s) Error size[%u], this message will be abandoned",
           __FILE__, __LINE__, __FUNCTION__, size);
  } else {
    // TODO: this is a bottleneck.
    this->loop_->WakeupCallback(boost::bind(&TCPConnection::AbsorbCache,
          this->shared_from_this()));
  }
}

void TCPConnection::AbsorbCache() {
  if(this->state_ == CONNECTED) {
    while(true) {
      this->send_buffer_.PreWrite(Configure::GetTCPConnection()->buffer_unit_size_);
      int ret = this->cache_.Remove(this->send_buffer_.WritePointer(),
          this->send_buffer_.WriteSize());
      if(ret > 0) {
        this->send_buffer_.WriteShift(ret);
      } else if(ret == kErrorCodeEmpty) {
        break;
      } else if(ret == kErrorCodeErrorSize) {
        continue;
      } else if(ret == kErrorCodeUnknown) {
        CoreLog(ERROR, "%s:%d (%s) return value is [%d], process will abort",
            __FILE__, __LINE__, __FUNCTION__, ret);
        abort();
      } else {
        CoreLog(ERROR, "%s:%d (%s) return value is [%d]",
            __FILE__, __LINE__, __FUNCTION__, ret);
      }
    }
    if(this->send_buffer_.ReadSize() > 0 && this->channel_->CheckWriteEvent() == false) {
      this->channel_->EnableWrite();
    }
  }
}

void TCPConnection::OnRead(const TimeTick &time_tick) {
  // Ensure could receive 1 message.
  this->receive_buffer_.PreWrite(Configure::GetTCPConnection()->buffer_unit_size_);
  int ret = this->socket_.Receive(this->receive_buffer_.WritePointer(),
         this->receive_buffer_.WriteSize());
  if(ret > 0) {
    // Every time recieving complete, PreWrite will enlarge the capacity of buffer,
    // this enlarging will reach a balance situation in maybe servral times(this
    // depends on the communication speed).
    this->receive_buffer_.PreWrite(Configure::GetTCPConnection()->buffer_unit_size_);
    // Shift write pointer.
    this->receive_buffer_.WriteShift(ret);
    assert(this->message_callback_);
    this->message_callback_(shared_from_this(), this->receive_buffer_, time_tick);
  }
  else if(ret == 0) {
    this->OnClose();
  } else {
    this->OnError();
  }
}

void TCPConnection::OnWrite() {
  int ret = this->socket_.Send(this->send_buffer_.ReadPointer(),
         this->send_buffer_.ReadSize());
  if(ret > 0) {
    this->send_buffer_.ReadShift(ret);
    if(this->send_buffer_.ReadSize() == 0) {
      this->channel_->DisableWrite();
      if(this->write_complete_callback_)
        this->write_complete_callback_(shared_from_this());
    }
  } else if(ret < 0) {
    this->OnError();
  }
}

void TCPConnection::OnClose() {
  CoreLog(INFO, "%s:%d (%s) [%p] Close occured.", __FILE__, __LINE__, __FUNCTION__, this);
  this->channel_->DisableWrite();
  this->RequireShutdown();
}

void TCPConnection::OnError() {
  CoreLog(INFO, "%s:%d (%s) [%p] Error occured.", __FILE__, __LINE__, __FUNCTION__, this);
  this->channel_->DisableWrite();
  this->RequireShutdown();
}

void TCPConnection::RequireShutdown() {
  if(this->channel_->CheckWriteEvent() == false) {
    AutoMutex auto_mutex(&this->mutex_);
    if(this->state_ == CONNECTED) {
      this->SetState(DISCONNECTING);
      this->loop_->WakeupCallback(boost::bind(&TCPConnection::Disconnect, this), false);
    }
  }
}

void TCPConnection::Disconnect() {
  if(this->state_ == DISCONNECTING) {
    this->SetState(DISCONNECTED);
    this->socket_.Close();
    this->channel_->DisableAll();
    if(this->close_callback_) this->close_callback_(shared_from_this());
  }
}

}  // namespace core

