
#include "payload.h"
#include <string.h>

void PAYLOAD_serialize(PAYLOAD_sensor_t *payload, uint8_t buffer[sizeof(PAYLOAD_sensor_t)]) {
  memset(buffer, 0, sizeof(PAYLOAD_sensor_t));
  int b = 0;

  /* Do the mac address */
  for (int i = 0; i < 6; i++) {
    buffer[b++] = payload->mac[i];
  }

  buffer[b++] = (payload->delay >> 8);
  buffer[b++] = payload->delay;

  buffer[b++] = payload->message_type;
  buffer[b++] = payload->message_id;

  /* Do the adc values. */
  for (int i = 0; i < PAYLOAD_ADC_NUM; i++) {
    if (b > sizeof(PAYLOAD_sensor_t)) {
      return;
    }
    buffer[b++] = (payload->adc[i] >> 8);
    buffer[b++] = payload->adc[i];
  }

  buffer[b++] = (payload->batt >> 8);
  buffer[b++] = payload->batt;
}

void PAYLOAD_unserialize(PAYLOAD_sensor_t *payload, uint8_t buffer[sizeof(PAYLOAD_sensor_t)]) {
  int b = 0;

  /* Do the mac address */
  for (int i = 0; i < 6; i++) {
    payload->mac[i] = buffer[b++];
  }

  payload->delay = (buffer[b++] << 8);
  payload->delay |= buffer[b++];

  payload->message_type = buffer[b++];
  payload->message_id = buffer[b++];

  /* Do the adc values. */
  for (int i = 0; i < PAYLOAD_ADC_NUM; i++) {
    if (b > sizeof(PAYLOAD_sensor_t)) {
      return;
    }
    payload->adc[i] = (buffer[b++] << 8);
    payload->adc[i] |= buffer[b++];
  }

  payload->batt = (buffer[b++] << 8);
  payload->batt |= buffer[b++];
}
