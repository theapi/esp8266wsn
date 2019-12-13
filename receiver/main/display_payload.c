
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "display_payload.h"
#include "display.h"
#include "payload.h"

//#include "esp_log.h"
#include "esp_timer.h"

// static const char *TAG = "DP";

// Identified by their mac address: 5ccf7f80dfd2
static char sensors[DP_NUM_SENSORS][13] = { "", "", "" };
static int32_t sensors_last[DP_NUM_SENSORS] = {};
static int32_t sensors_delay[DP_NUM_SENSORS] = {};

static int8_t sensorNum(PAYLOAD_sensor_t *payload) {
  char mac[13];
  sprintf(mac, "%02x%02x%02x%02x%02x%02x",
    payload->mac[0], payload->mac[1], payload->mac[2], payload->mac[3], payload->mac[4], payload->mac[5]);
  for (uint8_t i = 0; i < DP_NUM_SENSORS; i++) {
    if (strcmp(mac, sensors[i]) == 0) {
      //ESP_LOGI(TAG, "Found: %s", mac);
      // Update the delay in case it changed.
      sensors_delay[i] = payload->delay;
      return i;
    }
  }

  // Not seen before, so add it to the array.
  for (uint8_t i = 0; i < DP_NUM_SENSORS; i++) {
    if (strcmp("", sensors[i]) == 0) {
      strcpy(sensors[i], mac);
      sensors_delay[i] = payload->delay;
      return i;
    }
  }

  return -1;
}

static char needsBattery(PAYLOAD_sensor_t *payload) {
  if (payload->batt < DP_BATTERY_THRESHOLD) {
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

  int8_t sensor = sensorNum(payload);
  if (sensor == -1) {
    // Couldn't assign an led to the sensor, so do notihng.
    return;
  }

  // Remeber when we last heard from this sensor.
  int32_t now = (int32_t) (esp_timer_get_time() / 1000);
  sensors_last[sensor] = now;

  // Displays for 3 multi input sensors.
  // 1st led is for the first sensor.
  // xxxx xxxR GBRG BRGB
  uint16_t red = (DP_RED << sensor * 3);
  uint16_t green = (DP_GREEN << sensor * 3);
  uint16_t blue = (DP_BLUE << sensor * 3);
  uint16_t clear = ~(red | green | blue);

  // Clear the led bits for the sensor.
  hdisplay.pixels &= clear;
  if (needsBattery(payload) == 1) {
    hdisplay.pixels |= red;
  } else if (needsWater(payload) == 1) {
    hdisplay.pixels |= blue;
  } else {
    hdisplay.pixels |= green;
  }
  Display_update(&hdisplay);
}

static void sensorClear(int8_t sensor) {
  uint16_t red = (DP_RED << sensor * 3);
  uint16_t green = (DP_GREEN << sensor * 3);
  uint16_t blue = (DP_BLUE << sensor * 3);
  uint16_t clear = ~(red | green | blue);
  hdisplay.pixels &= clear;
  Display_update(&hdisplay);
}

/**
 * Clear a sensor's led if not heard from for a while.
 */
static void clearTask(void *pvParameters)
{
    while(1) {
        vTaskDelay(500 / portTICK_RATE_MS);
        int32_t now = (int32_t) (esp_timer_get_time() / 1000);
        for (uint8_t i = 0; i < DP_NUM_SENSORS; i++) {
          // If longer than the delay time clear the led for that sensor.
          if (now - sensors_last[i] > (sensors_delay[i] + DP_DELAY_EXTRA) * 1000) {
            sensorClear(i);
          }
        }
    }
}

esp_err_t DisplayPayload_start() {
  BaseType_t xReturned;
  xReturned = xTaskCreate(clearTask, "clearTask", 2048, NULL, 4, NULL);
  if( xReturned != pdPASS ) {
    return ESP_FAIL;
  }

  return ESP_OK;
}