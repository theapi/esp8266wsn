#ifndef DEVICE_H
#define DEVICE_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define DEVICE_ID_PIN0 14 // D5
#define DEVICE_ID_PIN1 12 // D6
#define DEVICE_ID_PIN2 13 // D7
#define DEVICE_ID_PIN3 15 // D8

uint8_t Device_id();

#ifdef __cplusplus
}
#endif
#endif
