/**
 * WSN Receiver
 */

#include "freertos/FreeRTOS.h"

#include "driver/uart.h"

#include "display.h"
#include "payload.h"
#include "network.h"
#include "light.h"

//static const char *TAG = "receiver";

#define UART_BUF_SIZE (1024)

char uart_buffer[UART_BUF_SIZE];

void app_main(void) {
  DISPLAY_Init();
  // Configure ADC after the SPI
  LIGHT_Init();

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

  hdisplay.pixels = 0x1FF;
  DISPLAY_Update(&hdisplay);
}
