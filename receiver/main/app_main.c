/**
 * WSN Receiver
*/

#include <stdlib.h>
#include <string.h>

#include "crc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include "esp_event_loop.h"
#include "tcpip_adapter.h"
#include "esp_wifi.h"

#include "esp8266/spi_struct.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_now.h"

#include "driver/adc.h"
#include "driver/gpio.h"
#include "driver/spi.h"
#include "driver/uart.h"

#include "app_main.h"
#include "display.h"
#include "payload.h"

static const char *TAG = "receiver";


#define UART_BUF_SIZE (1024)

char uart_buffer[UART_BUF_SIZE];
static xQueueHandle app_espnow_queue;
uint32_t count = 0;

/* WiFi should start before using ESPNOW */
static esp_err_t app_wifi_init(void)
{
    tcpip_adapter_init();

    if (esp_event_loop_init(NULL, NULL) != ESP_OK) {
      ESP_LOGE(TAG, "Failed: esp_event_loop_init");
      return ESP_FAIL;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&cfg) != ESP_OK) {
      ESP_LOGE(TAG, "Failed: esp_wifi_init");
      return ESP_FAIL;
    }

    if (esp_wifi_set_storage(WIFI_STORAGE_RAM) != ESP_OK) {
      ESP_LOGE(TAG, "Failed: esp_wifi_set_storage");
      return ESP_FAIL;
    }

    if (esp_wifi_set_mode(ESPNOW_WIFI_MODE) != ESP_OK) {
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

/* ESPNOW sending or receiving callback function is called in WiFi task.
 * Users should not do lengthy operations from this task. Instead, post
 * necessary data to a queue and handle it from a lower priority task. */
static void app_espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len)
{
    app_espnow_event_t evt;
    app_espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;

    if (mac_addr == NULL || data == NULL || len <= 0) {
        ESP_LOGE(TAG, "Receive cb arg error");
        return;
    }

    evt.id = APP_ESPNOW_RECV_CB;
    memcpy(recv_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    recv_cb->data = malloc(len);
    if (recv_cb->data == NULL) {
        ESP_LOGE(TAG, "Malloc receive data fail");
        esp_restart();
    }
    memcpy(recv_cb->data, data, len);
    recv_cb->data_len = len;
    if (xQueueSend(app_espnow_queue, &evt, portMAX_DELAY) != pdTRUE) {
        ESP_LOGW(TAG, "Receive queue fail");
        free(recv_cb->data);
        esp_restart();
    }
}

/* Parse received ESPNOW data. */
int app_espnow_data_parse(uint8_t *data, uint16_t data_len, payload_sensor_t *payload)
{
    int i = 0;
    payload_sensor_t *buf = (payload_sensor_t *)data;
    uint16_t crc, crc_cal = 0;

    if (data_len < sizeof(payload_sensor_t)) {
        ESP_LOGE(TAG, "Receive ESPNOW data too short, len:%d", data_len);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "ADC 0:%d", buf->adc[0]);
    ESP_LOGI(TAG, "ADC 1:%d", buf->adc[1]);
    for (i = 0; i < PAYLOAD_ADC_NUM; i++) {
        payload->adc[i] = buf->adc[i];
    }

    crc = buf->crc;
    buf->crc = 0;
    crc_cal = crc16_le(UINT16_MAX, (uint8_t const *)data, data_len);

    if (crc_cal == crc) {
        return ESP_OK;
    }

    return ESP_FAIL;
}

static void app_espnow_task(void *pvParameter)
{
    app_espnow_event_t evt;
    int ret;

    vTaskDelay(500 / portTICK_RATE_MS);
    ESP_LOGI(TAG, "Start listening for broadcast data");

    while (xQueueReceive(app_espnow_queue, &evt, portMAX_DELAY) == pdTRUE) {
        switch (evt.id) {
            case APP_ESPNOW_RECV_CB:
            {
                app_espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;
                payload_sensor_t payload;

                ret = app_espnow_data_parse(recv_cb->data, recv_cb->data_len, &payload);
                free(recv_cb->data);
                if (ret == ESP_OK) {
                    if (count > 0x1FF) {
                      count = 0;
                    }
                    //hdisplay.pixels = ++count;
                    hdisplay.pixels = payload.adc[0];
                    DISPLAY_Update(&hdisplay);

                    ESP_LOGI(TAG, "ADC_1: %d, ADC_2: %d data from: "MACSTR", len: %d",
                      payload.adc[0],
                      payload.adc[1],
                      MAC2STR(recv_cb->mac_addr),
                      recv_cb->data_len
                    );
                    // uint16_t len = sprintf(uart_buffer, "%d - broadcast data from: "MACSTR", len: %d\n", ++count, MAC2STR(recv_cb->mac_addr), recv_cb->data_len);
                    // // Write data back to the UART
                    // uart_write_bytes(UART_NUM_0, (const char *) uart_buffer, len);
                }
                else {
                    ESP_LOGI(TAG, "Receive error data from: "MACSTR"", MAC2STR(recv_cb->mac_addr));
                }
                break;
            }
            default:
                ESP_LOGE(TAG, "Callback type error: %d", evt.id);
                break;
        }
    }
}


static esp_err_t app_espnow_init(void)
{
    app_espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(app_espnow_event_t));
    if (app_espnow_queue == NULL) {
        ESP_LOGE(TAG, "Create mutex fail");
        return ESP_FAIL;
    }

    /* Initialize ESPNOW and register receiving callback function. */
    if (esp_now_init() != ESP_OK) {
      ESP_LOGE(TAG, "Failed: esp_now_init");
      return ESP_FAIL;
    }

    if (esp_now_register_recv_cb(app_espnow_recv_cb) != ESP_OK) {
      ESP_LOGE(TAG, "Failed: esp_now_register_send_cb");
      return ESP_FAIL;
    }

    // /* Set primary master key. */
    // if (esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) != ESP_OK) {
    //   ESP_LOGE(TAG, "Failed: esp_now_set_pmk");
    //   return ESP_FAIL;
    // }

    xTaskCreate(app_espnow_task, "app_espnow_task", 2048, NULL, 4, NULL);

    return ESP_OK;
}

static void adc_task(void *data) {
    uint16_t val;

    while (1) {
        if (ESP_OK == adc_read(&val)) {
            if (val > 200) {
              DISPLAY_On(&hdisplay);
            } else if (val < 150) {
              DISPLAY_Off(&hdisplay);
            }
            //ESP_LOGI(TAG, "adc read: %d", val);
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

    // Initialize NVS
    if (nvs_flash_init() != ESP_OK) {
      ESP_LOGE(TAG, "Failed: nvs_flash_init");
      esp_restart();
    }

    if (app_wifi_init() != ESP_OK) {
      esp_restart();
    }
    if (app_espnow_init() != ESP_OK) {
      esp_restart();
    }

    xTaskCreate(adc_task, "adc_task", 1024, NULL, 5, NULL);
    //xTaskCreate(spi_master_write_slave_task, "spi_master_write_slave_task", 2048, NULL, 10, NULL);

    hdisplay.pixels = 0x1FF;
    DISPLAY_Update(&hdisplay);
}
