#ifndef SENSOR_H
#define SENSOR_H
#ifdef __cplusplus
extern "C" {
#endif

#include "payload.h"

#define SENSOR_NUM 10
#define SENSOR_BUFFER_SIZE 64

/* The sensor struct. */
typedef struct {
  PAYLOAD_sensor_t payload;
  PAYLOAD_sensor_t previous;
  unsigned long last;
  int8_t num;
  uint8_t size;
  uint8_t raw[SENSOR_BUFFER_SIZE];
} Sensor_t;

Sensor_t SensorPopulate(uint8_t raw[SENSOR_BUFFER_SIZE], uint8_t size);
Sensor_t SensorGetByNumber(uint8_t num);

#ifdef __cplusplus
}
#endif
#endif

