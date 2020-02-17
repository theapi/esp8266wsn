#ifndef DISPLAY_PAYLOAD_H_
#define DISPLAY_PAYLOAD_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "payload.h"

#define DP_DELAY_EXTRA 5 // Number of seconds extra to wait to hear from the sensor again.
#define DP_WATER_THRESHOLD 900
#define DP_BATTERY_THRESHOLD 3600
#define DP_NUM_SENSORS 5

#define DP_RED 0x01
#define DP_GREEN 0x02
#define DP_BLUE 0x04

void DisplayPayload_show(PAYLOAD_sensor_t *payload);
esp_err_t DisplayPayload_start();

#ifdef __cplusplus
}
#endif
#endif /* DISPLAY_PAYLOAD_H_ */
