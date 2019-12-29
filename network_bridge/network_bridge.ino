/**
 * To subscribe to the UDP data: listen for broadcasts on 239.0.0.58 port 12345
 */

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "config.h"

#define MAX_SENSORS 10
#define RX_BUFFER_SIZE 64
#define UDP_DELAY_EXTRA 2 // Number of seconds extra to wait to hear from the sensor again.

// A UDP instance to let us send and receive packets over UDP
WiFiUDP Udp;

// Multicast declarations
IPAddress ipMulti(239, 0, 0, 58);
unsigned int portMulti = 12345;      // local port to listen on


WiFiClient espClient;

uint8_t rx_buffer[RX_BUFFER_SIZE];
uint8_t rx_buffer_index = 0;
uint8_t payload_length = 0;
const long ping_interval = 2000;
unsigned long ping_last = 0;

// Identified by their mac address bytes: 5c cf 7f xx xx xx
uint8_t sensors[MAX_SENSORS][6] = {0};
uint16_t sensors_delay[MAX_SENSORS] = {0};
unsigned long sensors_last[MAX_SENSORS] = {0};
uint8_t payload_buffer[MAX_SENSORS][RX_BUFFER_SIZE] = {0};

enum State {
  ST_WAITING,
  ST_LENGTH,
  ST_HEADER_VALIDATION,
  ST_DATA,
  ST_READY
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

void serialPrintRxBuffer() {
  Serial.println("Payload: ");
  for (int i = 0; i < RX_BUFFER_SIZE; i++) {
    Serial.print(rx_buffer[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

void udpBroadcastPayload(uint8_t num) {
  // Size of the payload is stored in the last byte of the payload buffer.
  uint8_t len = payload_buffer[num][RX_BUFFER_SIZE -1];
  if (len > RX_BUFFER_SIZE) {
    // Makes no sense so ignore this payload.
    return;
  }
  Serial.printf("broadcast: %d, hex: %02X \n", num, payload_buffer[num][1]);
  Udp.beginPacketMulticast(ipMulti, portMulti, WiFi.localIP());

  // Send the contents of the buffer.
  Udp.write(payload_buffer[num], len);

  Udp.endPacket();
  Udp.stop();
}

void udpPing() {
  //@todo sensors_last is going to get too large eventually
  unsigned long currentMillis = millis();
  for (uint8_t i = 0; i < MAX_SENSORS; i++) {
    if (payload_buffer[i][0] != 0) {
//      Serial.printf("delay: %d last: %d\n", sensors_delay[i], sensors_last[i]);
//      Serial.print(currentMillis - sensors_last[i]);
//      Serial.print(" > ");
//      Serial.print((sensors_delay[i] + UDP_DELAY_EXTRA) * 1000);
//      Serial.print(" = ");
//      Serial.println(currentMillis - sensors_last[i] > (sensors_delay[i] + UDP_DELAY_EXTRA) * 1000);
      if (currentMillis - sensors_last[i] > (sensors_delay[i] + UDP_DELAY_EXTRA) * 1000) {
        // Not heard from the sensor so remove it.
        sensors[i][0] = 0;
        payload_buffer[i][0] = 0;
        sensors_last[i] = 0;
      } else {
        Serial.printf("Ping: %d\n", i);
        udpBroadcastPayload(i);
      }
    }
  }
}

int8_t compareMacs(uint8_t a[6], uint8_t b[6]) {
  for (int i = 0; i < 6; i++) {
    if (a[i] != b[i]) {
      return 1;
    }
  }
  return 0;
}

uint8_t sensorNum(uint8_t mac[6]) {
  for (uint8_t i = 0; i < MAX_SENSORS; i++) {
    if (compareMacs(sensors[i], mac) == 0) {
      return i;
    }
  }

  // Not seen before, so add it to the array.
  for (uint8_t i = 0; i < MAX_SENSORS; i++) {
    if (sensors[i][0] == 0) {
      for (uint8_t x = 0; x < 6; x++) {
        sensors[i][x] = mac[x];
      }
      return i;
    }
  }

  return MAX_SENSORS -1;
}

uint8_t sensorNumFromRxBuffer() {
  // First byte is the payload type, then the next 6 bytes are the mac address.
  uint8_t mac[6];
  for (int i = 1; i < 7; i++) {
    mac[i - 1] = rx_buffer[i];
  }
  return sensorNum(mac);
}

void loop() {
  uint8_t num = 0;

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
          memset(rx_buffer, 0, RX_BUFFER_SIZE);
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
          
          // Update the local copy of the payload.
          num = sensorNumFromRxBuffer();
          for (int i = 0; i < RX_BUFFER_SIZE; i++) {
            payload_buffer[num][i] = rx_buffer[i];
          }
          // update the delay for this sensor.
          sensors_delay[num] = (payload_buffer[num][7] << 8) | (payload_buffer[num][8]);
          sensors_last[num] = currentMillis;
          
          // Store the size in the last byte of the payload buffer.
          payload_buffer[num][RX_BUFFER_SIZE -1] = payload_length;
          
          payload_state = ST_READY;
        }
        break;

      case ST_READY:
        // Buffer ready for processing.
        break;
    }
  }

  
  if (payload_state == ST_READY) {
    payload_state = ST_WAITING;
    serialPrintRxBuffer();
    udpBroadcastPayload(num);
  } else if ( (payload_state == ST_WAITING) && (currentMillis - ping_last >= ping_interval) ) {
    ping_last = currentMillis;
    udpPing();
  }
  
}

