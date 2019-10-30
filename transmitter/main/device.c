#include "device.h"
#include "esp_log.h"
#include "driver/gpio.h"


static const char *TAG = "device";

#define bitSet(value, bit) ((value) |= (1UL << (bit)))

uint8_t Device_id() {
  int pins[4];
  pins[0] = gpio_get_level(DEVICE_ID_PIN0);
  pins[1] = gpio_get_level(DEVICE_ID_PIN1);
  pins[2] = gpio_get_level(DEVICE_ID_PIN2);
  pins[3] = gpio_get_level(DEVICE_ID_PIN3);

  int id = ((pins[3] << 3) | (pins[2] << 2)  | (pins[1] << 1)  | (pins[0] << 0));
  ESP_LOGI(TAG, "pin0: %d, 1: %d, 2: %d, 3: %d, id: %d", pins[0], pins[1], pins[2], pins[3], id);

  return id;
}
