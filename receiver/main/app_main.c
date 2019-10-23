/**
 * WSN Receiver
 */

#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "esp8266/spi_struct.h"
//#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_system.h"
//#include "esp_wifi.h"

#include "driver/adc.h"
#include "driver/gpio.h"
#include "driver/spi.h"
#include "driver/uart.h"

#include "crc.h"
#include "tcpip_adapter.h"

#include "app_main.h"
#include "display.h"
#include "payload.h"
#include "network.h"

static const char *TAG = "receiver";

#define UART_BUF_SIZE (1024)

char uart_buffer[UART_BUF_SIZE];
static xQueueHandle app_espnow_queue;
uint32_t count = 0;


/* ESPNOW sending or receiving callback function is called in WiFi task.
 * Users should not do lengthy operations from this task. Instead, post
 * necessary data to a queue and handle it from a lower priority task. */
static void app_espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data,
                               int len) {
  app_espnow_event_t event = {0};

  if (mac_addr == NULL || data == NULL || len <= 0) {
    ESP_LOGE(TAG, "Receive cb arg error");
    return;
  }

  memcpy(event.mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
  memcpy(event.data, data, len);
  event.data_len = len;
  if (xQueueSend(app_espnow_queue, &event, portMAX_DELAY) != pdTRUE) {
    ESP_LOGW(TAG, "Receive queue fail");
    esp_restart();
  }
}

/* Parse received ESPNOW data. */
int app_espnow_data_parse(uint8_t *data, uint16_t data_len,
                          payload_sensor_t *payload) {
  /* Map the data to the struct for ease of manipulation */
  payload_sensor_t *buf = (payload_sensor_t *)data;
  uint16_t crc, crc_cal = 0;

  if (data_len < sizeof(payload_sensor_t)) {
    ESP_LOGE(TAG, "Receive ESPNOW data too short, len:%d", data_len);
    return ESP_FAIL;
  }

  payload->device_id = buf->device_id;
  ESP_LOGI(TAG, "ADC 0:%d", buf->adc[0]);
  ESP_LOGI(TAG, "ADC 1:%d", buf->adc[1]);
  memcpy(payload->adc, buf->adc, sizeof(payload->adc));

  crc = buf->crc;
  buf->crc = 0;
  crc_cal = crc16_le(UINT16_MAX, (uint8_t const *)buf, data_len);

  if (crc_cal == crc) {
    return ESP_OK;
  }

  return ESP_FAIL;
}

static void app_espnow_task(void *pvParameter) {
  app_espnow_event_t event;
  int ret;

  vTaskDelay(500 / portTICK_RATE_MS);
  ESP_LOGI(TAG, "Start listening for broadcast data");

  while (xQueueReceive(app_espnow_queue, &event, portMAX_DELAY) == pdTRUE) {
    payload_sensor_t payload;

    ret = app_espnow_data_parse(event.data, event.data_len, &payload);
    //free(event.data);
    ESP_LOGI(TAG, "RAM left %d bytes", esp_get_free_heap_size());
    if (ret == ESP_OK) {
      if (count > 0x1FF) {
        count = 0;
      }
      // hdisplay.pixels = ++count;
      hdisplay.pixels = payload.adc[0];
      DISPLAY_Update(&hdisplay);

      ESP_LOGI(TAG, "Device: %d, ADC_1: %d, ADC_2: %d data from: " MACSTR ", len: %d",
                payload.device_id,
                payload.adc[0], payload.adc[1], MAC2STR(event.mac_addr),
                event.data_len);
      // uint16_t len = sprintf(uart_buffer, "%d - broadcast data from:
      // "MACSTR", len: %d\n", ++count, MAC2STR(recv_cb->mac_addr),
      // recv_cb->data_len);
      // // Write data back to the UART
      // uart_write_bytes(UART_NUM_0, (const char *) uart_buffer, len);

    } else {
      ESP_LOGI(TAG, "Receive error data from: " MACSTR "",
                MAC2STR(event.mac_addr));
    }
  }
}

static void adc_task(void *data) {
  uint16_t val;

  while (1) {
    if (ESP_OK == adc_read(&val)) {
      if (val > 120) {
        DISPLAY_On(&hdisplay);
      } else if (val < 100) {
        DISPLAY_Off(&hdisplay);
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

  if (esp_now_register_recv_cb(app_espnow_recv_cb) != ESP_OK) {
    ESP_LOGE(TAG, "Failed: esp_now_register_send_cb");
    esp_restart();
  }

  app_espnow_queue =
      xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(app_espnow_event_t));
  if (app_espnow_queue == NULL) {
    ESP_LOGE(TAG, "Create mutex fail");
    esp_restart();
  }

  xTaskCreate(app_espnow_task, "app_espnow_task", 2048, NULL, 4, NULL);

  xTaskCreate(adc_task, "adc_task", 1024, NULL, 5, NULL);
  // xTaskCreate(spi_master_write_slave_task, "spi_master_write_slave_task",
  // 2048, NULL, 10, NULL);

  hdisplay.pixels = 0x1FF;
  DISPLAY_Update(&hdisplay);
}
