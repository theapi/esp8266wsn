/**
 *  Library for defining the payload datastructure for a solar sensor.
 */
#ifndef ESP8266WSN_PAYLOAD_h
#define ESP8266WSN_PAYLOAD_h

#include <stdint.h>

namespace esp8266wsn {

  class Payload {
    public:
      typedef struct {
        uint8_t MessageType;
        uint8_t DeviceId;
        uint8_t MessageId;
        uint16_t Batt;
        uint16_t vals[5];
      } payload_t;

      const static uint8_t SIZE = 15;

      Payload();

      // How big the whole payload is.
      uint8_t size();

      /**
       * The type of message.
       * Each Theapi payload has a message type,
       * so the receiver can read the first byte to know which type is arriving.
       */
      uint8_t getMsgType();
      void setMsgType();

      // The id of the device.
      uint16_t getDeviceId();
      void setDeviceId(uint16_t device_id);

      // The id, not neccessarily unique, of the message.
      uint8_t getMsgId();
      void setMsgId(uint8_t msg_id);

      uint16_t getBatt();
      void setBatt(uint16_t val);

      uint16_t getVal(uint8_t key);
      void setVal(uint8_t key, uint16_t val);

      // Creates a byte array for sending via the radio
      void serialize(uint8_t buffer[Payload::SIZE]);

      // Parse the read byte data from the radio
      void unserialize(uint8_t buffer[Payload::SIZE]);

    private:
      payload_t _payload;

  };

}

#endif

