

#include "esp_now.h"

#ifndef ESPNOW_APP_H
#define ESPNOW_APP_H

#include "payload.h"

#define ESPNOW_QUEUE_SIZE 6

/* When ESPNOW receiving callback function is called. */
typedef struct {
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    //uint8_t *data;
    uint8_t data[sizeof(payload_sensor_t)];
    int data_len;
} app_espnow_event_t;

#endif
