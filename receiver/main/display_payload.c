
#include <string.h>

#include "display_payload.h"
#include "display.h"
#include "payload.h"

#include "esp_log.h"

static const char *TAG = "DP";

// Identified by their mac address: 5ccf7f80dfd2
static char sensors[DP_NUM_SENSORS][13] = { "", "", "" };

static uint8_t sensorNum(PAYLOAD_sensor_t *payload) {
  char mac[13];
  sprintf(mac, "%02x%02x%02x%02x%02x%02x",
    payload->mac[0], payload->mac[1], payload->mac[2], payload->mac[3], payload->mac[4], payload->mac[5]);
  for (uint8_t i = 0; i < DP_NUM_SENSORS; i++) {
    if (strcmp(mac, sensors[i]) == 0) {
      ESP_LOGI(TAG, "Found: %s", mac);
      return i;
    }
  }

  // Not seen before, so add it to the array.
  for (uint8_t i = 0; i < DP_NUM_SENSORS; i++) {
    if (strcmp("", sensors[i]) == 0) {
      strcpy(sensors[i], mac);
      return i;
    }
  }

  return 0;
}

static char needsBattery(PAYLOAD_sensor_t *payload) {
  if (payload->adc[0] < DP_BATTERY_THRESHOLD) {
    return 1;
  }
  return 0;
}

static char needsWater(PAYLOAD_sensor_t *payload) {
  // Ignore top value as no sensor is open circuit.
  // Start at 1 as the battery is on 0.
  for (int i = 1; i < PAYLOAD_ADC_NUM; i++) {
    if (payload->adc[i] < 1023) {
      if (payload->adc[i] > DP_WATER_THRESHOLD) {
        return 1;
      }
    }
  }
  return 0;
}

void DisplayPayload_show(PAYLOAD_sensor_t *payload) {
  // Red = low battery
  // Blue = needs water
  // Green = all is well

  // Displays for 3 multi input sensors.
  // 1st led is for the first sensor.
  // xxxx xxxR GBRG BRGB


  uint8_t sensor = sensorNum(payload);
  ESP_LOGI(TAG, "Sensor: %d", sensor);

  // @todo handle multiple sensors cleaner
  // @todo clear a sensor's led if not heard from for a while.
  if (sensor == 0) {
    // Clear the bits for sensor 1
    hdisplay.pixels &= ~(0x07);
    if (needsBattery(payload) == 1) {
      // Set the first bit (red) for sensor 1
      hdisplay.pixels |= 0x01;
    } else if (needsWater(payload) == 1) {
      // Set the 3rd bit (blue) for sensor 1
      hdisplay.pixels |= 0x04;
    } else {
      // Set the 2nd bit (green) for sensor 1
      hdisplay.pixels |= 0x02;
    }
    Display_update(&hdisplay);
  }

  if (sensor == 1) {
    // Clear the bits
    hdisplay.pixels &= ~(0x38);
    if (needsBattery(payload) == 1) {
      // Set the first bit (red)
      hdisplay.pixels |= 0x08;
    } else if (needsWater(payload) == 1) {
      // Set the 3rd bit (blue)
      hdisplay.pixels |= 0x20;
    } else {
      // Set the 2nd bit (green)
      hdisplay.pixels |= 0x10;
    }
    Display_update(&hdisplay);
  }
}
