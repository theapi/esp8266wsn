

#include "esp_now.h"

#ifndef ESPNOW_APP_H
#define ESPNOW_APP_H

#define ESPNOW_QUEUE_SIZE 6

typedef struct {
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    esp_now_send_status_t status;
} app_espnow_event_send_cb_t;

typedef struct {
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    uint8_t *data;
    int data_len;
} app_espnow_event_recv_cb_t;

typedef union {
    app_espnow_event_send_cb_t send_cb;
    app_espnow_event_recv_cb_t recv_cb;
} app_espnow_event_info_t;

/* When ESPNOW receiving callback function is called, post event to ESPNOW task. */
typedef struct {
    app_espnow_event_info_t info;
} app_espnow_event_t;

#endif
