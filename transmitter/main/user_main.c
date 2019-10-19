
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include "esp_event_loop.h"
#include "tcpip_adapter.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_sleep.h"
#include "esp_now.h"
#include "rom/ets_sys.h"
#include "crc.h"
#include "user_main.h"
#include "driver/gpio.h"

static const char *TAG = "transmitter";

uint32_t count = 0;

static uint8_t app_broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static uint16_t s_app_espnow_seq[APP_ESPNOW_DATA_MAX] = { 0, 0 };

static esp_err_t app_event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        ESP_LOGI(TAG, "WiFi started");
        break;
    default:
        break;
    }
    return ESP_OK;
}

/* WiFi should start before using ESPNOW */
static void app_wifi_init(void)
{
    tcpip_adapter_init();

    if (esp_event_loop_init(app_event_handler, NULL) != ESP_OK) {
      ESP_LOGE(TAG, "Failed: esp_event_loop_init");
      abort();
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&cfg) != ESP_OK) {
      ESP_LOGE(TAG, "Failed: esp_wifi_init");
      abort();
    }

    if (esp_wifi_set_storage(WIFI_STORAGE_RAM) != ESP_OK) {
      ESP_LOGE(TAG, "Failed: esp_wifi_set_storage");
      abort();
    }

    if (esp_wifi_set_mode(ESPNOW_WIFI_MODE) != ESP_OK) {
      ESP_LOGE(TAG, "Failed: esp_wifi_set_mode");
      abort();
    }

    if (esp_wifi_start() != ESP_OK) {
      ESP_LOGE(TAG, "Failed: esp_wifi_start");
      abort();
    }

    /* In order to simplify example, channel is set after WiFi started.
     * This is not necessary in real application if the two devices have
     * been already on the same channel.
     */
    if (esp_wifi_set_channel(CONFIG_ESPNOW_CHANNEL, 0) != ESP_OK) {
      ESP_LOGE(TAG, "Failed: esp_wifi_set_channel");
      abort();
    }
}

/* Prepare ESPNOW data to be sent. */
void app_espnow_data_prepare(app_espnow_send_param_t *send_param)
{
    //app_espnow_data_t *buf = (app_espnow_data_t *)send_param->buffer;
    sensor_payload_t *buf = (sensor_payload_t *)send_param->buffer;
    //int i = 0;

    assert(send_param->len >= sizeof(sensor_payload_t));

    //buf->type = IS_BROADCAST_ADDR(send_param->dest_mac) ? APP_ESPNOW_DATA_BROADCAST : APP_ESPNOW_DATA_UNICAST;
    //buf->state = send_param->state;
    //buf->seq_num = s_app_espnow_seq[buf->type]++;
    buf->crc = 0;
    buf->adc = count++;
    if (count > 1024) {
      count = 1;
    }
    // for (i = 0; i < send_param->len - sizeof(app_espnow_data_t); i++) {
    //     buf->payload[i] = (uint8_t)i;
    // }
    // ESP_LOGI(TAG, "PAYLOAD size:%d", i);
    buf->crc = crc16_le(UINT16_MAX, (uint8_t const *)buf, send_param->len);
}

static esp_err_t app_espnow_init(void)
{
    /* Initialize ESPNOW and register sending and receiving callback function. */
    if (esp_now_init() != ESP_OK) {
      ESP_LOGE(TAG, "Failed: esp_now_init");
      abort();
    }

    /* Set primary master key. */
    if (esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) != ESP_OK) {
      ESP_LOGE(TAG, "Failed: esp_now_set_pmk");
      abort();
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
    peer->ifidx = ESPNOW_WIFI_IF;
    peer->encrypt = false;
    memcpy(peer->peer_addr, app_broadcast_mac, ESP_NOW_ETH_ALEN);
    if (esp_now_add_peer(peer) != ESP_OK) {
      ESP_LOGE(TAG, "Failed: esp_now_add_peer");
      abort();
    }
    free(peer);

    return ESP_OK;
}

void app_transmit() {
/* Initialize sending parameters. */
    app_espnow_send_param_t *send_param;
    send_param = malloc(sizeof(app_espnow_send_param_t));
    memset(send_param, 0, sizeof(app_espnow_send_param_t));
    if (send_param == NULL) {
        ESP_LOGE(TAG, "Malloc send parameter fail");
        esp_now_deinit();
    } else {
        send_param->unicast = false;
        send_param->broadcast = true;
        send_param->state = 0;
        send_param->magic = esp_random();
        send_param->count = CONFIG_ESPNOW_SEND_COUNT;
        send_param->delay = CONFIG_ESPNOW_SEND_DELAY;
        send_param->len = CONFIG_ESPNOW_SEND_LEN;
        send_param->buffer = malloc(CONFIG_ESPNOW_SEND_LEN);
        if (send_param->buffer == NULL) {
            ESP_LOGE(TAG, "Malloc send buffer fail");
            free(send_param);
            esp_now_deinit();
        } else {
            memcpy(send_param->dest_mac, app_broadcast_mac, ESP_NOW_ETH_ALEN);
            app_espnow_data_prepare(send_param);

            /* Start sending broadcast ESPNOW data. */
            if (esp_now_send(send_param->dest_mac, send_param->buffer, send_param->len) != ESP_OK) {
                ESP_LOGE(TAG, "Send error");
                //app_espnow_deinit(send_param);
            }
        }
    }
}

void app_main()
{
    // Initialize NVS
    if (nvs_flash_init() != ESP_OK) {
      ESP_LOGE(TAG, "Failed: nvs_flash_init");
      abort();
    }

    app_wifi_init();
    app_espnow_init();

    app_transmit();

    // Deep sleep and restart after sleep.
    // Connect GPIO16 to RESET (after flashing for this to work)
    // disconnect GPIO16 to flash again.
    //esp_deep_sleep(10e6);

    // Just for dev work as I need to flash with Arduino IDE after sleep :(
    while (1) {
      vTaskDelay(5000 / portTICK_RATE_MS);
      app_transmit();
    }
}
