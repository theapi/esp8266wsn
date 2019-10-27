#ifndef LIGHT_H_
#define LIGHT_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "esp_system.h"

esp_err_t Light_init();
esp_err_t Light_start();

#ifdef __cplusplus
}
#endif
#endif /* LIGHT_H_ */
