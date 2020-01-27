/**
 * To subscribe to the UDP data: listen for broadcasts on 239.0.0.58 port 12345
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "config.h"
#include "payload.h"
#include "sensor.h"

#define UDP_DELAY_EXTRA 2 // Number of seconds extra to wait to hear from the sensor again.

// A UDP instance to let us send and receive packets over UDP
WiFiUDP Udp;

// Multicast declarations
IPAddress ipMulti(239, 0, 0, 58);
unsigned int portMulti = 12345;      // local port to listen on


WiFiClient espClient;

uint8_t rx_buffer[SENSOR_BUFFER_SIZE];
uint8_t rx_buffer_index = 0;
uint8_t payload_length = 0;
const long ping_interval = 2000;
unsigned long ping_last = 0;

enum State {
  ST_WAITING,
  ST_LENGTH,
  ST_HEADER_VALIDATION,
  ST_DATA
};
State payload_state = ST_WAITING;

void setup() {
  Serial.begin(115200);
  Serial.println();

  WiFi.begin(ssid, password);
  Serial.print("\nConnecting to "); Serial.println(ssid);
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20) delay(500);
  if(i == 21){
    Serial.print("Could not connect to"); Serial.println(ssid);
    delay(5000);
    ESP.restart();
  }

  Serial.println(WiFi.localIP().toString());
  Serial.println("Ready! Listen for UDP broadcasts on 239.0.0.58 port 12345");
}

void udpBroadcastPayload(uint8_t num) {
  Sensor_t sensor = SensorGetByNumber(num);
  if (sensor.size != 0) {
    Serial.printf("broadcast: %d, mac: %02X,  msgid: %d\n", sensor.num, sensor.payload.mac[0], sensor.payload.message_id);
    Udp.beginPacketMulticast(ipMulti, portMulti, WiFi.localIP());

    // Send the contents of the original raw buffer.
    Udp.write(sensor.raw, sensor.size);

    Udp.endPacket();
    Udp.stop();
  }
}

void udpPing() {
  unsigned long now = millis();
  for (uint8_t i = 0; i < SENSOR_NUM; i++) {
    Sensor_t sensor = SensorGetByNumber(i);
    if (now - sensor.last > (sensor.payload.delay + UDP_DELAY_EXTRA) * 1000UL) {
      // Old data don't send it.
    } else if (sensor.payload.message_id != sensor.previous.message_id) {
      Serial.printf("Ping: %d\n", i);
      udpBroadcastPayload(i);
    }
  }
}

void loop() {
  unsigned long currentMillis = millis();

  // Read the data from the receiver.
  while (Serial.available()) {
    // get the new byte:
    uint8_t c = (uint8_t) Serial.read();
    //Serial.println(c, HEX);

    switch (payload_state) {
      case ST_WAITING:
        // Wait for the payload header.
        if (c == 0xAA) {
          rx_buffer_index = 0;
          // Clear the rx buffer, ready for new data.
          memset(rx_buffer, 0, SENSOR_BUFFER_SIZE);
          payload_state = ST_LENGTH;
        } else {
          // Passthru other serial messages.
          //Serial.print(char(c));
        }
        break;

      case ST_LENGTH:
        payload_length = c;
        payload_state = ST_HEADER_VALIDATION;
        break;

      case ST_HEADER_VALIDATION:
        // Next byte is the end of the header.
        if (c == 0xAA) {
          payload_state = ST_DATA;
        } else {
          // Not a real header, so start again.
          payload_state = ST_WAITING;
        }
        break;

      case ST_DATA:
        rx_buffer[rx_buffer_index++] = c;
        if (rx_buffer_index == payload_length) {
          Sensor_t sensor = SensorPopulate(rx_buffer, payload_length);
          udpBroadcastPayload(sensor.num);
//          Serial.printf("Sensor: num %d, prev: %d, msg_id %d, last %d, size %d\n",
//            sensor.num, sensor.previous.message_id, sensor.payload.message_id, sensor.last, sensor.size);

          payload_state = ST_WAITING;
        }
        break;
    }
  }


  if ( (payload_state == ST_WAITING) && (currentMillis - ping_last >= ping_interval) ) {
    ping_last = currentMillis;
    udpPing();
  }

}

