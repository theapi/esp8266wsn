#ifndef DISPLAY_H_
#define DISPLAY_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "esp_system.h"
#include "payload.h"

#define DISPLAY_PIN_BLUE 4 // D2 on NodeMCU

/**
  * @brief  DISPLAY State structures definition
  */
typedef enum {
  DISPLAY_STATE_ON,
  DISPLAY_STATE_OFF,
} DISPLAY_StateTypeDef;

/**
  * @brief  DISPLAY Handle Structure definition
  */
typedef struct __DISPLAY_HandleTypeDef {
  uint16_t pixels; /* Three rgb leds, either on or off, one bit each colour xxxxxxxRGBRGBRGB */
  DISPLAY_StateTypeDef state;  /* On/off */
} DISPLAY_HandleTypeDef;

extern DISPLAY_HandleTypeDef hdisplay;

void Display_init();
void Display_update(DISPLAY_HandleTypeDef *hdisplay);
void Display_on(DISPLAY_HandleTypeDef *hdisplay);
void Display_off(DISPLAY_HandleTypeDef *hdisplay);

#ifdef __cplusplus
}
#endif
#endif /* DISPLAY_H_ */
