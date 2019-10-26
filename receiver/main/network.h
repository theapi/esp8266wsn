#ifndef NETWORK_H_
#define NETWORK_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "esp_now.h"
#include "esp_system.h"
#include "payload.h"

#define ESPNOW_QUEUE_SIZE 6

/* When ESPNOW receiving callback function is called. */
typedef struct {
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    //uint8_t *data;
    uint8_t data[sizeof(payload_sensor_t)];
    int data_len;
} NETWORK_event_t;

/* Boilerplate wifi & espnow initialisation. */
esp_err_t NETWORK_Init();

#ifdef __cplusplus
}
#endif
#endif /* NETWORK_H_ */
