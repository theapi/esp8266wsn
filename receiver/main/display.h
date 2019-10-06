#ifndef DISPLAY_H_
#define DISPLAY_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "esp_system.h"

#define GPIO_OUTPUT_IO_0    4
#define GPIO_OUTPUT_PIN_SEL  (1ULL<<GPIO_OUTPUT_IO_0)

void display_setup();
void display_Off ();

esp_err_t display_write_byte(uint8_t data);

#ifdef __cplusplus
}
#endif
#endif /* DISPLAY_H_ */
