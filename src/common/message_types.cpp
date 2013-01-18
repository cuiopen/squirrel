/**
 * Autogenerated by Thrift Compiler (0.8.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#include "message_types.h"

namespace squirrel { namespace common {

int _kMessageTypeValues[] = {
  MessageType::MESSAGE_TYPE_SCHEDULE,
  MessageType::MESSAGE_TYPE_ROUTE,
  MessageType::MESSAGE_TYPE_DATA_PROXY,
  MessageType::MESSAGE_TYPE_GATEWAY
};
const char* _kMessageTypeNames[] = {
  "MESSAGE_TYPE_SCHEDULE",
  "MESSAGE_TYPE_ROUTE",
  "MESSAGE_TYPE_DATA_PROXY",
  "MESSAGE_TYPE_GATEWAY"
};
const std::map<int, const char*> _MessageType_VALUES_TO_NAMES(::apache::thrift::TEnumIterator(4, _kMessageTypeValues, _kMessageTypeNames), ::apache::thrift::TEnumIterator(-1, NULL, NULL));

const char* Message::ascii_fingerprint = "19B5240589E680301A7E32DF3971EFBE";
const uint8_t Message::binary_fingerprint[16] = {0x19,0xB5,0x24,0x05,0x89,0xE6,0x80,0x30,0x1A,0x7E,0x32,0xDF,0x39,0x71,0xEF,0xBE};

uint32_t Message::read(::apache::thrift::protocol::TProtocol* iprot) {

  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;

  bool isset_type_ = false;
  bool isset_message_ = false;

  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    switch (fid)
    {
      case 1:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          int32_t ecast0;
          xfer += iprot->readI32(ecast0);
          this->type_ = (MessageType::type)ecast0;
          isset_type_ = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 2:
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readBinary(this->message_);
          isset_message_ = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      default:
        xfer += iprot->skip(ftype);
        break;
    }
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  if (!isset_type_)
    throw TProtocolException(TProtocolException::INVALID_DATA);
  if (!isset_message_)
    throw TProtocolException(TProtocolException::INVALID_DATA);
  return xfer;
}

uint32_t Message::write(::apache::thrift::protocol::TProtocol* oprot) const {
  uint32_t xfer = 0;
  xfer += oprot->writeStructBegin("Message");
  xfer += oprot->writeFieldBegin("type_", ::apache::thrift::protocol::T_I32, 1);
  xfer += oprot->writeI32((int32_t)this->type_);
  xfer += oprot->writeFieldEnd();
  xfer += oprot->writeFieldBegin("message_", ::apache::thrift::protocol::T_STRING, 2);
  xfer += oprot->writeBinary(this->message_);
  xfer += oprot->writeFieldEnd();
  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}

}} // namespace