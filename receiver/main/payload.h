#ifndef PAYLOAD_H
#define PAYLOAD_H

#include <stdint.h>

/* The number of adc channels available to the sensor */
#define PAYLOAD_ADC_NUM 8

/* Sensor payload. */
typedef struct {
    uint16_t device_id;
    uint16_t adc[PAYLOAD_ADC_NUM];                      //ADC readings
    uint16_t crc;                         //CRC16 value of ESPNOW data.
} __attribute__((packed)) payload_sensor_t;

#endif
