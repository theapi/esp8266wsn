
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

#define PIN_SENSOR_PWR  5  // D1 (nodemcu)
#define PIN_MULTIPLEX_A 14 // D5 (nodemcu)
#define PIN_MULTIPLEX_B 12 // D6 (nodemcu)
#define PIN_MULTIPLEX_C 13 // D7 (nodemcu)
#define GPIO_OUTPUT_PIN_SEL ((1ULL<<PIN_MULTIPLEX_A) | (1ULL<<PIN_MULTIPLEX_B) | (1ULL<<PIN_MULTIPLEX_C) | (1ULL<<PIN_SENSOR_PWR) )
#define GPIO_INPUT_IO_0     5
#define GPIO_INPUT_PIN_SEL  (1ULL<<GPIO_INPUT_IO_0)

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

    // io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    // //set as input mode
    // io_conf.mode = GPIO_MODE_INPUT;
    // gpio_config(&io_conf);
}

/* Selects the ADC multiplexer channel. */
static void multiplexer_set_channel(uint8_t chan) {
  // Set the levels of the multiplexer channel
  // from the bit values of the given channel number.
  gpio_set_level(PIN_MULTIPLEX_A, (chan >> 0) & 0x01);
  gpio_set_level(PIN_MULTIPLEX_B, (chan >> 1) & 0x01);
  gpio_set_level(PIN_MULTIPLEX_C, (chan >> 2) & 0x01);
}

/**
 * Mapping for hardware inputs to multplexer channels:
 * chan 0 = input 4
 * chan 1 = input 5
 * chan 2 = input 6
 * chan 3 = input 3
 * chan 4 = input 2
 * chan 5 = battery
 * chan 6 = input 1
 * chan 7 = not connected
 */
static void readings_get(uint16_t adc[PAYLOAD_ADC_NUM]) {
  uint16_t map[8] = {4, 5, 6, 3, 2, 0, 1, 7};
  for (uint16_t i = 0; i < PAYLOAD_ADC_NUM; i++) {
    multiplexer_set_channel(i);
    //ESP_LOGI(TAG, "chan: %d", i);
    if (i < 8) {
      uint16_t j = map[i];
      adc_read(&adc[j]);
      adc_read(&adc[j]);
    }
  }
}

esp_err_t readings_init() {
  adc_config_t adc_config;
  adc_config.mode = ADC_READ_TOUT_MODE;
  adc_config.clk_div = 8;  // 80MHz/clk_div = 10MHz
  if (adc_init(&adc_config) != ESP_OK) {
    return ESP_FAIL;
  }

  gpio_set_level(PIN_SENSOR_PWR, 1);

  return ESP_OK;
}

/* Prepare ESPNOW data to be sent. */
void app_espnow_data_prepare() {
  static uint8_t msg_id = 0;

  /* Map the buffer to the struct for ease of manipulation */
  PAYLOAD_sensor_t *buf = (PAYLOAD_sensor_t *)buffer;
  //esp_read_mac(buf->mac, ESP_MAC_WIFI_STA);
  esp_efuse_mac_get_default(buf->mac);
  buf->message_type = 200;
  buf->message_id = ++msg_id;
  buf->crc = 0;
  readings_get(buf->adc);
  /*
    Battery id adc[0]
    raw 668 = 4680 mV
    so 1 = 7.005988024mV
  */
  //uint16_t batt = buf->adc[0] * 7.0059;
  //buf->adc[0] = batt;

  buf->crc =
      crc16_le(UINT16_MAX, (uint8_t const *)buf, sizeof(PAYLOAD_sensor_t));
  ESP_LOGI(TAG, "Mac: %02X:%02X:%02X:%02X:%02X:%02X",
                buf->mac[0], buf->mac[1], buf->mac[2], buf->mac[3], buf->mac[4], buf->mac[5]);
  ESP_LOGI(TAG, "msg_id: %d, ADC_0: %d, ADC_1: %d, ADC_2: %d, ADC_3: %d, ADC_4: %d, ADC_5: %d, ADC_6: %d",
                buf->message_id,
                buf->adc[0], buf->adc[1], buf->adc[2], buf->adc[3], buf->adc[4], buf->adc[5], buf->adc[6]);
}

static esp_err_t app_transmit() {
  app_espnow_data_prepare();
  ESP_LOGI(TAG, "RAM left %d bytes", esp_get_free_heap_size());

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
