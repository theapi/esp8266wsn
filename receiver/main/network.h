#ifndef NETWORK_H_
#define NETWORK_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "esp_system.h"

/* Boilerplate wifi & espnow initialisation. */
esp_err_t NETWORK_Init();

#ifdef __cplusplus
}
#endif
#endif /* NETWORK_H_ */
