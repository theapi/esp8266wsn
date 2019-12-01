
#include "payload.h"

void PAYLOAD_serialize(PAYLOAD_sensor_t *payload, uint8_t buffer[sizeof(PAYLOAD_sensor_t)]) {
  buffer[0] = payload->message_type;
  buffer[1] = payload->message_id;

  /* Do the mac address */
  int b = 1;
  for (int i = 0; i < 6; i++) {
    buffer[++b] = payload->mac[i];
  }

  /* Do the adc values. */
  for (int i = 0; i < PAYLOAD_ADC_NUM; i++) {
    if (b > sizeof(payload)) {
      return;
    }
    buffer[++b] = (payload->adc[i] >> 8);
    buffer[++b] = payload->adc[i];
  }
}

void PAYLOAD_unserialize(PAYLOAD_sensor_t *payload, uint8_t buffer[sizeof(PAYLOAD_sensor_t)]) {
  payload->message_type = buffer[0];
  payload->message_id = buffer[1];

  /* Do the mac address */
  int b = 1;
  for (int i = 0; i < 6; i++) {
    payload->mac[i] = buffer[++b];
  }

  /* Do the adc values. */
  for (int i = 0; i < PAYLOAD_ADC_NUM; i++) {
    if (b > sizeof(payload)) {
      return;
    }
    payload->adc[i] = (buffer[++b] << 8) | buffer[++b];
  }
}
