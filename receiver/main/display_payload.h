#ifndef DISPLAY_PAYLOAD_H_
#define DISPLAY_PAYLOAD_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "payload.h"

#define DP_WATER_THRESHOLD 900
#define DP_BATTERY_THRESHOLD 3200
#define DP_NUM_SENSORS 3

void DisplayPayload_show(PAYLOAD_sensor_t *payload);

#ifdef __cplusplus
}
#endif
#endif /* DISPLAY_PAYLOAD_H_ */
