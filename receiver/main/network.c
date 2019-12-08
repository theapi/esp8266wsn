#include <stdlib.h>
#include <string.h>

#include "network.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/timers.h"


#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_system.h"
#include "esp_wifi.h"

#include "driver/uart.h"

#include "crc.h"
#include "display.h"

static const char *TAG = "network";
static xQueueHandle recv_queue;

char uart_buffer[UART_BUF_SIZE];
uint16_t count = 0;

/* WiFi should start before using ESPNOW */
static esp_err_t wifi_init(void)
{
    tcpip_adapter_init();

    if (esp_event_loop_init(NULL, NULL) != ESP_OK) {
      ESP_LOGE(TAG, "Failed: esp_event_loop_init");
      return ESP_FAIL;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    cfg.nvs_enable = 0;
    if (esp_wifi_init(&cfg) != ESP_OK) {
      ESP_LOGE(TAG, "Failed: esp_wifi_init");
      return ESP_FAIL;
    }

    if (esp_wifi_set_storage(WIFI_STORAGE_RAM) != ESP_OK) {
      ESP_LOGE(TAG, "Failed: esp_wifi_set_storage");
      return ESP_FAIL;
    }

    if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK) {
      ESP_LOGE(TAG, "Failed: esp_wifi_set_mode");
      return ESP_FAIL;
    }

    if (esp_wifi_start() != ESP_OK) {
      ESP_LOGE(TAG, "Failed: esp_wifi_start");
      return ESP_FAIL;
    }

    /* In order to simplify example, channel is set after WiFi started.
     * This is not necessary in real application if the two devices have
     * been already on the same channel.
     */
    if (esp_wifi_set_channel(CONFIG_ESPNOW_CHANNEL, 0) != ESP_OK) {
      ESP_LOGE(TAG, "Failed: esp_wifi_set_channel");
      return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t espnow_init(void) {
  /* Initialize ESPNOW and register receiving callback function. */
  if (esp_now_init() != ESP_OK) {
    ESP_LOGE(TAG, "Failed: esp_now_init");
    return ESP_FAIL;
  }

  return ESP_OK;
}


/* Parse received ESPNOW data. */
static int data_parse(uint8_t *data, uint16_t data_len,
                          PAYLOAD_sensor_t *payload) {
  /* Map the data to the struct for ease of manipulation */
  PAYLOAD_sensor_t *buf = (PAYLOAD_sensor_t *)data;
  uint16_t crc, crc_cal = 0;

  if (data_len < sizeof(PAYLOAD_sensor_t)) {
    ESP_LOGE(TAG, "Receive ESPNOW data too short, len:%d", data_len);
    return ESP_FAIL;
  }

  memcpy(payload->mac, buf->mac, sizeof(payload->mac));
  payload->message_type = buf->message_type;
  payload->message_id = buf->message_id;
  memcpy(payload->adc, buf->adc, sizeof(payload->adc));

  crc = buf->crc;
  buf->crc = 0;
  crc_cal = crc16_le(UINT16_MAX, (uint8_t const *)buf, data_len);

  if (crc_cal == crc) {
    return ESP_OK;
  }

  return ESP_FAIL;
}

static void recv_task(void *pvParameter) {
  NETWORK_event_t event;
  int ret;

  vTaskDelay(100 / portTICK_RATE_MS);
  //ESP_LOGI(TAG, "Start listening for broadcast data");

  while (xQueueReceive(recv_queue, &event, portMAX_DELAY) == pdTRUE) {
    PAYLOAD_sensor_t payload;

    ret = data_parse(event.data, event.data_len, &payload);
    // ESP_LOGI(TAG, "RAM left %d bytes", esp_get_free_heap_size());
    if (ret == ESP_OK) {
      payload.message_id = ++count;
      hdisplay.pixels = payload.adc[0] / 100; // Battery decivolts for now.
      Display_update(&hdisplay);

      // Debug output.
      // uint16_t slen = sprintf(uart_buffer, "%d - from "MACSTR", batt: %d, A: %d\n",
      //   payload.message_id, MAC2STR(event.mac_addr), payload.adc[0], payload.adc[1]);
      // uart_write_bytes(UART_NUM_0, (const char *) uart_buffer, slen);

      // Send the serialized data through the UART.
      size_t len = sizeof(PAYLOAD_sensor_t);
      uint8_t sbuf[len];
      uint8_t tx_start[1] = {0xAA};
      uint8_t tx_len[1] = {len};
      PAYLOAD_serialize(&payload, sbuf);

      // Write the tx header, so the other end of the UART
      // knows a payload is coming and how long it is.
      uart_write_bytes(UART_NUM_0, (const char *) tx_start, 1);
      uart_write_bytes(UART_NUM_0, (const char *) tx_len, 1);
      uart_write_bytes(UART_NUM_0, (const char *) tx_start, 1);
      // Write payload to the UART
      uart_write_bytes(UART_NUM_0, (const char *) sbuf, len);

      // // Debug output, to prove the serialization is correct.
      // PAYLOAD_sensor_t debug;
      // PAYLOAD_unserialize(&debug, sbuf);
      // uint16_t debug_len = sprintf(uart_buffer, "%d - from "MACSTR", batt: %d, A: %d\n",
      //   debug.message_id, MAC2STR(event.mac_addr), debug.adc[0], debug.adc[1]);
      // uart_write_bytes(UART_NUM_0, (const char *) uart_buffer, debug_len);

    } else {
      ESP_LOGI(TAG, "Receive error data from: " MACSTR "",
                MAC2STR(event.mac_addr));
    }
  }
}

/* ESPNOW sending or receiving callback function is called in WiFi task.
 * Users should not do lengthy operations from this task. Instead, post
 * necessary data to a queue and handle it from a lower priority task. */
static void recv_cb(const uint8_t *mac_addr, const uint8_t *data,
                               int len) {
  NETWORK_event_t event = {0};

  if (mac_addr == NULL || data == NULL || len <= 0) {
    ESP_LOGE(TAG, "Receive cb arg error");
    return;
  }

  memcpy(event.mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
  memcpy(event.data, data, len);
  event.data_len = len;
  if (xQueueSend(recv_queue, &event, portMAX_DELAY) != pdTRUE) {
    ESP_LOGW(TAG, "Receive queue fail");
    esp_restart();
  }
}

esp_err_t Network_init() {
  if (wifi_init() != ESP_OK) {
    return ESP_FAIL;
  }
  if (espnow_init() != ESP_OK) {
    return ESP_FAIL;
  }

  if (esp_now_register_recv_cb(recv_cb) != ESP_OK) {
    ESP_LOGE(TAG, "Failed: esp_now_register_send_cb");
    esp_restart();
  }

  recv_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(NETWORK_event_t));
  if (recv_queue == NULL) {
    ESP_LOGE(TAG, "Create mutex fail");
    esp_restart();
  }

  return ESP_OK;
}

esp_err_t Network_start() {
  BaseType_t xReturned;
  xReturned = xTaskCreate(recv_task, "recv_task", 2048, NULL, 4, NULL);
  if( xReturned != pdPASS ) {
    return ESP_FAIL;
  }

  return ESP_OK;
}
