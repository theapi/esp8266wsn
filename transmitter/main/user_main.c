
#include <string.h>
#include "crc.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "payload.h"
#include "tcpip_adapter.h"

#include "driver/adc.h"
#include "driver/gpio.h"

#include "device.h"

#define PIN_MULTIPLEX 4
#define GPIO_OUTPUT_PIN_SEL (1ULL<<PIN_MULTIPLEX)
#define GPIO_INPUT_PIN_SEL  ((1ULL<<DEVICE_ID_PIN0) | (1ULL<<DEVICE_ID_PIN1) | (1ULL<<DEVICE_ID_PIN2) | (1ULL<<DEVICE_ID_PIN3))

static const char *TAG = "transmitter";

static uint8_t app_broadcast_mac[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF,
                                                      0xFF, 0xFF, 0xFF};
/* Global send buffer. */
uint8_t buffer[sizeof(PAYLOAD_sensor_t)] = {0};


/* WiFi should start before using ESPNOW */
static esp_err_t app_wifi_init(void) {
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

static esp_err_t app_espnow_init(void) {
  /* Initialize ESPNOW and register sending and receiving callback function. */
  if (esp_now_init() != ESP_OK) {
    ESP_LOGE(TAG, "Failed: esp_now_init");
    return ESP_FAIL;
  }

  /* Add broadcast peer information to peer list. */
  esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
  if (peer == NULL) {
    ESP_LOGE(TAG, "Malloc peer information fail");
    esp_now_deinit();
    return ESP_FAIL;
  }
  memset(peer, 0, sizeof(esp_now_peer_info_t));
  peer->channel = CONFIG_ESPNOW_CHANNEL;
  peer->ifidx = ESP_IF_WIFI_STA;
  peer->encrypt = false;
  memcpy(peer->peer_addr, app_broadcast_mac, ESP_NOW_ETH_ALEN);

  esp_err_t ret = esp_now_add_peer(peer);
  free(peer);
  if (ret != ESP_OK) {
    //ESP_LOGE(TAG, "Failed: esp_now_add_peer");
    return ESP_FAIL;
  }

  return ESP_OK;
}

static void setupGPIO() {
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO15/16
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
}

static void readings_get(uint16_t adc[2]) {
  gpio_set_level(PIN_MULTIPLEX, 0);
  adc_read(&adc[0]);
  // change multiplexer
  gpio_set_level(PIN_MULTIPLEX, 1);
  adc_read(&adc[1]);
  gpio_set_level(PIN_MULTIPLEX, 0);
}

esp_err_t readings_init() {
  adc_config_t adc_config;
  adc_config.mode = ADC_READ_TOUT_MODE;
  adc_config.clk_div = 8;  // 80MHz/clk_div = 10MHz
  if (adc_init(&adc_config) != ESP_OK) {
    return ESP_FAIL;
  }

  return ESP_OK;
}

/* Prepare ESPNOW data to be sent. */
void app_espnow_data_prepare() {
  static uint8_t msg_id = 0;

  /* Map the buffer to the struct for ease of manipulation */
  PAYLOAD_sensor_t *buf = (PAYLOAD_sensor_t *)buffer;
  buf->device_id = Device_id();
  buf->message_id = ++msg_id;
  buf->crc = 0;
  //buf->adc[0] = 3300;   // will be battery reading
  //buf->adc[1] = 1234;  // will be the soil reading
  readings_get(buf->adc);
  buf->crc =
      crc16_le(UINT16_MAX, (uint8_t const *)buf, sizeof(PAYLOAD_sensor_t));
  ESP_LOGI(TAG, "Device: %d, msg_id: %d, ADC_1: %d, ADC_2: %d",
                buf->device_id,
                buf->message_id,
                buf->adc[0], buf->adc[1]);
}

static esp_err_t app_transmit() {
  app_espnow_data_prepare();

  /* Start sending broadcast ESPNOW data. */
  if (esp_now_send(app_broadcast_mac, buffer, sizeof(PAYLOAD_sensor_t)) !=
      ESP_OK) {
    ESP_LOGE(TAG, "Send error");
    return ESP_FAIL;
  } else {
    ESP_LOGI(TAG, "Sent");
    return ESP_OK;
  }
  return ESP_FAIL;
}

void app_main() {
  if (app_wifi_init() != ESP_OK) {
    esp_restart();
  }
  if (app_espnow_init() != ESP_OK) {
    esp_restart();
  }

  setupGPIO();
  if (readings_init() != ESP_OK) {
    esp_restart();
  }

  app_transmit();

  // Deep sleep and restart after sleep.
  // Connect GPIO16 to RESET (after flashing for this to work)
  // disconnect GPIO16 to flash again.
  // esp_deep_sleep(10e6);

  // Just for dev work as I need to flash with Arduino IDE after sleep :(
  while (1) {
    vTaskDelay(5000 / portTICK_RATE_MS);
    if (app_transmit() != ESP_OK) {
      esp_restart();
    }
  }
}
