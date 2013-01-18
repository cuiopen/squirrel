//
// Summary: server core source code.
//
// Author: Tony.
// Email: tonyjobmails@gmail.com.
// Last modify: 2012-08-27 15:53:44.
// File name: noncopyable.h
//
// Description: 
// Noncopyable.
//

#ifndef __CORE__NONCOPYABLE__H
#define __CORE__NONCOPYABLE__H

namespace core {

namespace noncopyable_ {

class Noncopyable {
 protected:
  Noncopyable() {}
  ~Noncopyable() {}

 private:
  Noncopyable(const Noncopyable&);
  const Noncopyable& operator=(const Noncopyable&);
};

}  // namespace noncopyable_

typedef noncopyable_::Noncopyable Noncopyable;

}  // namespace core

#endif  // __CORE__NONCOPYABLE__H

