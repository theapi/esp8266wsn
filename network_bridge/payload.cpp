
#include "payload.h"

namespace esp8266wsn {

  Payload::Payload() {
    setMsgType();
    _payload.DeviceId = 0;
    _payload.MessageId = 0;
    _payload.Batt = 0;
    _payload.vals[5] = {0};
  }

  uint8_t Payload::size() {
    return SIZE;
  }

  void Payload::setMsgType() {
    _payload.MessageType = 200;
  }

  uint8_t Payload::getMsgType() {
    return _payload.MessageType;
  }

  uint16_t Payload::getDeviceId() {
    return _payload.DeviceId;
  }

  void Payload::setDeviceId(uint16_t val) {
    _payload.DeviceId = val;
  }

  // The id, not neccessarily unique, of the message.
  uint8_t Payload::getMsgId() {
    return _payload.MessageId;
  }

  void Payload::setMsgId(uint8_t id) {
    _payload.MessageId = id;
  }


  uint16_t Payload::getBatt() {
    return _payload.Batt;
  }

  void Payload::setBatt(uint16_t val) {
    _payload.Batt = val;
  }

  uint16_t Payload::getVal(uint8_t key) {
    return _payload.vals[key];
  }
  
  void Payload::setVal(uint8_t key, uint16_t val) {
    // Prevent buffer overflow.
    if (key >= 5) {
      key = 1;
    }
    _payload.vals[key] = val;
  }

  // Populates the given buffer with the payload data.
  void Payload::serialize(uint8_t buffer[Payload::SIZE]) {
    buffer[0] = _payload.MessageType;
    buffer[1] = _payload.DeviceId;
    buffer[2] = _payload.MessageId;
    buffer[3] = (_payload.Batt >> 8);
    buffer[4] = _payload.Batt;
  }

  // Parse the byte data from the buffer.
  void Payload::unserialize(uint8_t buffer[Payload::SIZE]) {
    _payload.MessageType = buffer[0];
    _payload.DeviceId = buffer[1];
    _payload.MessageId = buffer[2];
    _payload.Batt = (buffer[3] << 8) | buffer[4];
  }

}

