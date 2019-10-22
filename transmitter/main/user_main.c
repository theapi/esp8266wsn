
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "tcpip_adapter.h"
#include "esp_event_loop.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_sleep.h"
#include "esp_now.h"
#include "crc.h"
#include "payload.h"

static const char *TAG = "transmitter";

uint32_t count = 0;

static uint8_t app_broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
/* Global send buffer. */
uint8_t buffer[sizeof(payload_sensor_t)] = {0};

/* WiFi should start before using ESPNOW */
static esp_err_t app_wifi_init(void)
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

static esp_err_t app_espnow_init(void)
{
    /* Initialize ESPNOW and register sending and receiving callback function. */
    if (esp_now_init() != ESP_OK) {
      ESP_LOGE(TAG, "Failed: esp_now_init");
      return ESP_FAIL;
    }

    // /* Set primary master key. */
    // if (esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) != ESP_OK) {
    //   ESP_LOGE(TAG, "Failed: esp_now_set_pmk");
    //   return ESP_FAIL;
    // }

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
    if (esp_now_add_peer(peer) != ESP_OK) {
      ESP_LOGE(TAG, "Failed: esp_now_add_peer");
      return ESP_FAIL;
    }
    free(peer);

    return ESP_OK;
}

/* Prepare ESPNOW data to be sent. */
void app_espnow_data_prepare()
{
    /* Map the buffer to the struct for ease of manipulation */
    payload_sensor_t *buf = (payload_sensor_t *)buffer;
    buf->crc = 0;
    buf->adc[0] = ++count;
    if (count > 1024) {
      count = 1;
    }
    buf->adc[1] = 3300; // will be the battery reading
    buf->crc = crc16_le(UINT16_MAX, (uint8_t const *)buf, sizeof(payload_sensor_t));
}

static esp_err_t app_transmit() {
  app_espnow_data_prepare();

  /* Start sending broadcast ESPNOW data. */
  if (esp_now_send(app_broadcast_mac, buffer, sizeof(payload_sensor_t)) != ESP_OK) {
    ESP_LOGE(TAG, "Send error");
    return ESP_FAIL;
  } else  {
    ESP_LOGI(TAG, "Sent");
    return ESP_OK;
  }
  return ESP_FAIL;
}

void app_main()
{
    // // Initialize NVS
    // if (nvs_flash_init() != ESP_OK) {
    //   ESP_LOGE(TAG, "Failed: nvs_flash_init");
    //   esp_restart();
    // }

    if (app_wifi_init() != ESP_OK) {
      esp_restart();
    }
    if (app_espnow_init() != ESP_OK) {
      esp_restart();
    }

    app_transmit();

    // Deep sleep and restart after sleep.
    // Connect GPIO16 to RESET (after flashing for this to work)
    // disconnect GPIO16 to flash again.
    //esp_deep_sleep(10e6);

    // Just for dev work as I need to flash with Arduino IDE after sleep :(
    while (1) {
      vTaskDelay(5000 / portTICK_RATE_MS);
      if (app_transmit() != ESP_OK) {
          esp_restart();
      }
    }
}
