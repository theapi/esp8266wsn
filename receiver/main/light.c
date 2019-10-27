
#include "light.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "driver/adc.h"
//#include "esp_log.h"

#include "display.h"

static void light_task(void *data) {
  uint16_t val;

  while (1) {
    if (ESP_OK == adc_read(&val)) {
      if (val > 120) {
        Display_on(&hdisplay);
      } else if (val < 100) {
        Display_off(&hdisplay);
      }
      // ESP_LOGI(TAG, "adc read: %d", val);
    }
    vTaskDelay(500 / portTICK_RATE_MS);
  }
}

esp_err_t Light_init() {
  adc_config_t adc_config;
  adc_config.mode = ADC_READ_TOUT_MODE;
  adc_config.clk_div = 8;  // 80MHz/clk_div = 10MHz
  if (adc_init(&adc_config) != ESP_OK) {
    return ESP_FAIL;
  }

  return ESP_OK;
}

esp_err_t Light_start() {
  BaseType_t xReturned;
  xReturned = xTaskCreate(light_task, "light_task", 1024, NULL, 5, NULL);
  if( xReturned != pdPASS ) {
    return ESP_FAIL;
  }

  return ESP_OK;
}
