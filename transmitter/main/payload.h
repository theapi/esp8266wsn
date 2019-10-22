#ifndef PAYLOAD_H
#define PAYLOAD_H

#include <stdint.h>

/* Sensor payload. */
typedef struct {
  uint16_t device_id;
  uint16_t adc[8];  // ADC readings
  uint16_t crc;     // CRC16 value of ESPNOW data.
} __attribute__((packed)) payload_sensor_t;

#endif
