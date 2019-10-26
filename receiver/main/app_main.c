/**
 * WSN Receiver
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/adc.h"
#include "driver/uart.h"

#include "display.h"
#include "payload.h"
#include "network.h"

static const char *TAG = "receiver";

#define UART_BUF_SIZE (1024)

char uart_buffer[UART_BUF_SIZE];

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

void setupADC() {
  adc_config_t adc_config;
  adc_config.mode = ADC_READ_TOUT_MODE;
  adc_config.clk_div = 8;  // 80MHz/clk_div = 10MHz
  if (adc_init(&adc_config) != ESP_OK) {
    ESP_LOGE(TAG, "Failed: adc_init");
    esp_restart();
  }
}

void app_main(void) {
  DISPLAY_Init();
  // Configure ADC after the SPI
  setupADC();

  // Configure parameters of an UART driver,
  // communication pins and install the driver
  // uart_config_t uart_config = {
  //     .baud_rate = 115200,
  //     .data_bits = UART_DATA_8_BITS,
  //     .parity    = UART_PARITY_DISABLE,
  //     .stop_bits = UART_STOP_BITS_1,
  //     .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
  // };
  // uart_param_config(UART_NUM_0, &uart_config);
  // uart_driver_install(UART_NUM_0, UART_BUF_SIZE * 2, 0, 0, NULL);

  if (NETWORK_Init() != ESP_OK) {
    esp_restart();
  }

  xTaskCreate(adc_task, "adc_task", 1024, NULL, 5, NULL);
  // xTaskCreate(spi_master_write_slave_task, "spi_master_write_slave_task",
  // 2048, NULL, 10, NULL);

  hdisplay.pixels = 0x1FF;
  DISPLAY_Update(&hdisplay);
}
