#ifndef PAYLOAD_H
#define PAYLOAD_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* The number of adc channels available to the sensor */
#define PAYLOAD_ADC_NUM 8

#define PAYLOAD_TYPE 200
/* Sensor payload. */
typedef struct {
  uint8_t payload_type;
  uint8_t mac[6];
  uint16_t delay;                // Seconds between transmisions
  uint8_t message_id;
  uint16_t adc[PAYLOAD_ADC_NUM]; //ADC readings
  uint16_t batt;                 // Battery in mV
  uint16_t crc;                  //CRC16 value of ESPNOW data.
} __attribute__((packed)) PAYLOAD_sensor_t;

#ifdef __cplusplus
}
#endif
#endif
