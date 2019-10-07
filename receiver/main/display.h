#ifndef DISPLAY_H_
#define DISPLAY_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "esp_system.h"

#define DISPLAY_PIN_BLUE 4

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

void DISPLAY_Init();
void DISPLAY_Update(DISPLAY_HandleTypeDef *hdisplay);
void DISPLAY_On(DISPLAY_HandleTypeDef *hdisplay);
void DISPLAY_Off(DISPLAY_HandleTypeDef *hdisplay);

#ifdef __cplusplus
}
#endif
#endif /* DISPLAY_H_ */
