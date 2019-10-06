/**
 * WSN Receiver
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp8266/spi_struct.h"
#include "esp_log.h"
#include "esp_system.h"

#include "driver/adc.h"
#include "driver/gpio.h"
#include "driver/spi.h"

#include "display.h"



/* The event bit that signifies there is not enough light to use the LEDs */
#define EV_DARK_BIT ( 1 << 0 )

static const char *TAG = "wsn_receiver";


/* What gets displayed on the LEDs */
uint8_t display = 1;

/* For sharing flags across tasks. */
EventGroupHandle_t xEventGroupHandle;

// // Write an 8-bit data
// static esp_err_t write_byte(uint8_t data)
// {
//     uint32_t buf = data << 24;
//     spi_trans_t trans = {0};
//     trans.mosi = &buf;
//     trans.bits.mosi = 8;
//     spi_trans(HSPI_HOST, trans);
//     return ESP_OK;
// }

static void spi_master_write_slave_task(void *arg)
{
    EventBits_t bits;
    while (1) {
        bits = xEventGroupGetBits(xEventGroupHandle);

        ESP_LOGI(TAG, "bits: %d", (int)bits);
        if( ( bits & EV_DARK_BIT ) != 1 ) {
            if (display == 0) {
              display = 1;
            }
            display_write_byte(display);
            display = display << 1;
        }

        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}

void displayOffTask(void *pvParameters)
{
    EventBits_t bits;
    while(1) {
        bits = xEventGroupWaitBits(
            xEventGroupHandle,
            EV_DARK_BIT,
            pdFALSE,
            pdTRUE,
            portMAX_DELAY );
        if( ( bits & EV_DARK_BIT ) != 0 ) {
            // Turn off the LEDs
            display_write_byte(0);
        }
    }
}

void blinkTask(void *pvParameters)
{
    int cnt = 0;
    while(1) {
        vTaskDelay(1000 / portTICK_RATE_MS);
        gpio_set_level(GPIO_OUTPUT_IO_0, ++cnt % 2);
        ESP_LOGI(TAG, "blink: %d", cnt);
    }
}

static void adc_task()
{
    uint16_t val;

    while (1) {
        if (ESP_OK == adc_read(&val)) {
            if (val > 200) {
              xEventGroupClearBits(xEventGroupHandle, EV_DARK_BIT);
            } else if (val < 150) {
              xEventGroupSetBits(xEventGroupHandle, EV_DARK_BIT);
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

    xTaskCreate(adc_task, "adc_task", 1024, NULL, 5, NULL);
}

void app_main(void)
{
    // Configure the GPIO before the SPI.
    display_setup();
    // Configure ADC after the SPI
    setupADC();

    xEventGroupHandle = xEventGroupCreate();
    if (xEventGroupHandle == NULL) {
      esp_restart();
    }

    xTaskCreate(spi_master_write_slave_task, "spi_master_write_slave_task", 2048, NULL, 10, NULL);
    xTaskCreate(displayOffTask, "displayOffTask", 1024, NULL, 5, NULL);
    xTaskCreate(blinkTask, "blinkTask", 1024, NULL, 2, NULL);
}
