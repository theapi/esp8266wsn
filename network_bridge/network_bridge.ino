/**
 * To subscribe to the UDP data: listen for broadcasts on 239.0.0.58 port 12345
 */

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "config.h"
#include "payload.h"

// A UDP instance to let us send and receive packets over UDP
WiFiUDP Udp;

// Multicast declarations
IPAddress ipMulti(239, 0, 0, 58);
unsigned int portMulti = 12345;      // local port to listen on


WiFiClient espClient;

esp8266wsn::Payload payload = esp8266wsn::Payload();
uint8_t input_string[32];
uint8_t payload_state = 0;
uint8_t current_payload;
uint8_t serial_byte_count = 0;

unsigned long message_last = 0;
uint8_t last_msg_id = 0;

unsigned long display_last = 0;
const long display_interval = 1000;

const long ping_interval = 3000;
unsigned long ping_last = 0;

String ip_end;


void setup() {
  Serial.begin(115200);
  Serial.println();

  WiFi.begin(ssid, password);
  Serial.print("\nConnecting to "); Serial.println(ssid);
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20) delay(500);
  if(i == 21){
    Serial.print("Could not connect to"); Serial.println(ssid);
    while(1) delay(500);
  }

  Serial.println("Ready! Listen for UDP broadcasts on 239.0.0.58 port 12345");

  String ip = WiFi.localIP().toString();
  ip_end = ip.substring(ip.lastIndexOf('.'));

}

void loop() {
  // Read the data from the receiver.
  while (Serial.available()) {
    // get the new byte:
    uint8_t in = (uint8_t) Serial.read();
    //Serial.println(in, HEX);

    if (payload_state == 0) {
      // Check for the start of the payload
      if (in == '\t') {
        payload_state = 1;
      }
    } else if (payload_state == 1) {

      // Check the first byte for the payload type.
      if (serial_byte_count == 0) {
        current_payload = in;
      }

      //Serial.print(in, HEX);
      // add it to the inputString:
      input_string[serial_byte_count] = in;
      ++serial_byte_count;

      // if the the last byte is received, set a flag
      // so the main loop can do something about it:
      if (current_payload == 200) {
        if (serial_byte_count == payload.size()) {
            serial_byte_count = 0;
            payload_state = 2;
            payload.unserialize(input_string);
          }
      }
    } else {
      // Passthru other serial messages.
      Serial.print(char(in));
    }
  }

  unsigned long currentMillis = millis();

  // Send payload to listeners when ready.
  if (payload_state == 2) {
    payload_state = 0;

    // No need to ping if we're sending real data.
    ping_last = currentMillis;
    udpBroadcastPayload();
    serialPrintPayload();

    current_payload = 0;
  }
  // Send the data continually, as its UDP some may get missed.
  else if (currentMillis - ping_last >= ping_interval) {
    ping_last = currentMillis;
    udpBroadcastPayload();
  }

}

void serialPrintPayload() {
  Serial.print("Payload: ");
  Serial.print(payload.getMsgType()); Serial.print(", ");
  Serial.print(payload.getDeviceId()); Serial.print(", ");
  Serial.print(payload.getMsgId()); Serial.print(", ");
  Serial.print(payload.getBatt()); Serial.print(", ");
  Serial.print(payload.getVal(0)); Serial.print(", ");
  Serial.print(payload.getVal(1)); Serial.print(", ");
  Serial.print(payload.getVal(2)); Serial.print(", ");
  Serial.print(payload.getVal(3)); Serial.print(", ");
  Serial.print(payload.getVal(4)); Serial.print(", ");
  Serial.println();
}

void udpBroadcastPayload() {
  Udp.beginPacketMulticast(ipMulti, portMulti, WiFi.localIP());
  Udp.write('\t'); // Payload start byte

  // Send the contents of the buffer.
  size_t len = sizeof(payload);
  uint8_t sbuf[len];
  payload.serialize(sbuf);
  Udp.write(sbuf, len);

  Udp.write('\n');
  Udp.endPacket();
  Udp.stop();
}

