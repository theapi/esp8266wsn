
#include "network.h"

#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_system.h"
#include "esp_wifi.h"

#include "app_main.h"

static const char *TAG = "network";

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

esp_err_t NETWORK_Init() {
  if (wifi_init() != ESP_OK) {
    return ESP_FAIL;
  }
  if (espnow_init() != ESP_OK) {
    return ESP_FAIL;
  }

  return ESP_OK;
}
