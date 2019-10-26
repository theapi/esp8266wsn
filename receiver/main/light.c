
#include "light.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/adc.h"
#include "esp_log.h"

#include "display.h"

static const char *TAG = "adc";

static void adc_task(void *data) {
  uint16_t val;

  while (1) {
    if (ESP_OK == adc_read(&val)) {
      if (val > 120) {
        DISPLAY_On(&hdisplay);
      } else if (val < 100) {
        //DISPLAY_Off(&hdisplay);
      }
      // ESP_LOGI(TAG, "adc read: %d", val);
    }
    vTaskDelay(500 / portTICK_RATE_MS);
  }
}

void LIGHT_Init() {
  adc_config_t adc_config;
  adc_config.mode = ADC_READ_TOUT_MODE;
  adc_config.clk_div = 8;  // 80MHz/clk_div = 10MHz
  if (adc_init(&adc_config) != ESP_OK) {
    ESP_LOGE(TAG, "Failed: adc_init");
    esp_restart();
  }

  xTaskCreate(adc_task, "adc_task", 1024, NULL, 5, NULL);
}
