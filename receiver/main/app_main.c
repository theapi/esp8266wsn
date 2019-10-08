/**
 * WSN Receiver
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include "freertos/event_groups.h"

#include "esp8266/spi_struct.h"
#include "esp_log.h"
#include "esp_system.h"

#include "driver/adc.h"
#include "driver/gpio.h"
#include "driver/spi.h"

#include "display.h"

static const char *TAG = "receiver";

static void spi_master_write_slave_task(void *data) {
    while (1) {
        ESP_LOGI(TAG, "hdisplay pixels: %d", (int)hdisplay.pixels);
        ESP_LOGI(TAG, "hdisplay state: %d", (int)hdisplay.state);
        hdisplay.pixels = hdisplay.pixels << 1;
        if (hdisplay.pixels > 256 || hdisplay.pixels == 0) {
          hdisplay.pixels = 1;
        }

        DISPLAY_Update(&hdisplay);
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}

static void adc_task(void *data) {
    uint16_t val;

    while (1) {
        if (ESP_OK == adc_read(&val)) {
            if (val > 200) {
              DISPLAY_On(&hdisplay);
            } else if (val < 150) {
              DISPLAY_Off(&hdisplay);
            }
            ESP_LOGI(TAG, "adc read: %d", val);
        }
        vTaskDelay(500 / portTICK_RATE_MS);
    }
}

void setupADC() {
    adc_config_t adc_config;
    adc_config.mode = ADC_READ_TOUT_MODE;
    adc_config.clk_div = 8; // ADC sample collection clock = 80MHz/clk_div = 10MHz
    if (adc_init(&adc_config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed: adc_init");
        esp_restart();
    }
}

void app_main(void) {
    DISPLAY_Init();
    // Configure ADC after the SPI
    setupADC();

    xTaskCreate(adc_task, "adc_task", 1024, NULL, 5, NULL);
    xTaskCreate(spi_master_write_slave_task, "spi_master_write_slave_task", 2048, NULL, 10, NULL);
}
