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

PAYLOAD_sensor_t payload = {0};
uint8_t serial_data[32];
uint8_t payload_state = 0;
uint8_t current_payload;
uint8_t serial_byte_count = 0;
uint8_t message_id;
const long ping_interval = 3000;
unsigned long ping_last = 0;


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
      serial_data[serial_byte_count] = in;
      ++serial_byte_count;

      // if the the last byte is received, set a flag
      // so the main loop can do something about it:
      if (current_payload == 200) {
        if (serial_byte_count == sizeof(PAYLOAD_sensor_t)) {
            serial_byte_count = 0;
            payload_state = 2;
            PAYLOAD_unserialize(&payload, serial_data);
            // Generate message id as it will always be 0 from the sensor.
            payload.message_id = message_id++;
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
  Serial.print(payload.message_type); Serial.print(", ");
  Serial.print(payload.message_id); Serial.print(", ");
  for (int i = 0; i < 6; i++) {
    Serial.print(payload.mac[i], HEX);
    if (i < 5) {
      Serial.print(":");
    }
  }
  Serial.print(", ");
  for (int i = 0; i < PAYLOAD_ADC_NUM; i++) {
    Serial.print(payload.mac[i]);
    Serial.print(", ");
  }

  Serial.println();
}

void udpBroadcastPayload() {
  Udp.beginPacketMulticast(ipMulti, portMulti, WiFi.localIP());
  Udp.write('\t'); // Payload start byte

  // Send the contents of the buffer.
  size_t len = sizeof(PAYLOAD_sensor_t);
  uint8_t sbuf[len];
  PAYLOAD_serialize(&payload, sbuf);
  Udp.write(sbuf, len);

  Udp.write('\n');
  Udp.endPacket();
  Udp.stop();
}

