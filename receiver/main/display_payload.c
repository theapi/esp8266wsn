
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "display_payload.h"
#include "display.h"
#include "payload.h"

#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "DP";

// Identified by their mac address: 5ccf7f80dfd2
static uint8_t sensors[DP_NUM_SENSORS][6] = {};
static int32_t sensors_last[DP_NUM_SENSORS] = {};
static int32_t sensors_delay[DP_NUM_SENSORS] = {};

/**
 * Compares mac addresses.
 *
 * returns zero if they are “equal”
 * see https://www.gnu.org/software/libc/manual/html_node/Comparison-Functions.html
 *
 */
static int8_t compareMacs(uint8_t a[6], uint8_t b[6]) {
  for (int i = 0; i < 6; i++) {
    if (a[i] != b[i]) {
      return 1;
    }
  }
  return 0;
}

static int8_t sensorNum(PAYLOAD_sensor_t *payload) {
  for (uint8_t i = 0; i < DP_NUM_SENSORS; i++) {
    if (compareMacs(sensors[i], payload->mac) == 0) {
      // Update the delay in case it changed.
      sensors_delay[i] = payload->delay;
      return i;
    }
  }

  // Not seen before, so add it to the array.
  for (uint8_t i = 0; i < DP_NUM_SENSORS; i++) {
    if (sensors[i][0] == 0) {
      //memcpy(payload->mac, sensors[i], 6); // doesn't work here
      for (uint8_t x = 0; x < 6; x++) {
        sensors[i][x] = payload->mac[x];
      }
      sensors_delay[i] = payload->delay;
      ESP_LOGI(TAG, "Added: %d - %02x %02x %02x %02x %02x %02x", i,
               sensors[i][0], sensors[i][1], sensors[i][2], sensors[i][3],
               sensors[i][4], sensors[i][5]);
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