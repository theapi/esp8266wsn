/**
 * To subscribe to the UDP data: listen for broadcasts on 239.0.0.58 port 12345
 */

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "config.h"

// A UDP instance to let us send and receive packets over UDP
WiFiUDP Udp;

// Multicast declarations
IPAddress ipMulti(239, 0, 0, 58);
unsigned int portMulti = 12345;      // local port to listen on


WiFiClient espClient;

uint8_t rx_buffer[256];
uint8_t rx_buffer_index = 0;
uint8_t payload_length = 0;
const long ping_interval = 3000;
unsigned long ping_last = 0;

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
    while(1) delay(500);
  }

  Serial.println(WiFi.localIP().toString());
  Serial.println("Ready! Listen for UDP broadcasts on 239.0.0.58 port 12345");
}


void serialPrintPayload() {
  Serial.println("Payload: ");
  for (int i = 0; i < payload_length; i++) {
    Serial.print(rx_buffer[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

void udpBroadcastPayload() {
  Udp.beginPacketMulticast(ipMulti, portMulti, WiFi.localIP());
  Udp.write('\t'); // Payload start byte

  // Send the contents of the buffer.
  Udp.write(rx_buffer, payload_length);

  Udp.write('\n');
  Udp.endPacket();
  Udp.stop();
}


void loop() {
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
          payload_state = ST_LENGTH;
        } else {
          // Passthru other serial messages.
          Serial.print(char(c));
        }
        break;
        
      case ST_LENGTH:
        payload_length = c;
        payload_state = ST_HEADER_VALIDATION;
        break;
        
      case ST_HEADER_VALIDATION:
        // Next byte is the end of the header.
        if (c == 0xAA) {
          // Clear the rx buffer, ready for new data.
          memset(rx_buffer, 0, payload_length);
          payload_state = ST_DATA;
        } else {
          // Not a real header, so start again.
          payload_state = ST_WAITING;
        }
        break;
        
      case ST_DATA:
        rx_buffer[rx_buffer_index++] = c;
        if (rx_buffer_index == payload_length) {
          payload_state = ST_READY;
        }
        break;

      case ST_READY:
        // Buffer ready for processing.
        break;
    }
  }

  unsigned long currentMillis = millis();

  // Send payload to listeners when ready.
  if (payload_state == ST_READY) {
    // No need to ping if we're sending real data.
    ping_last = currentMillis;
    udpBroadcastPayload();
    serialPrintPayload();

    // Ready for the next payload.
    payload_state = ST_WAITING;
  }
  // Send the data continually, as its UDP some may get missed.
  // Only send if not currently processing incoming data.
  else if ( (payload_state == ST_WAITING) && (currentMillis - ping_last >= ping_interval) ) {
    ping_last = currentMillis;
    udpBroadcastPayload();
  }
  
}

