# Trasmit with ESPNOW

## Flash
The standard flash doesn't work when the ESP has deep sleep.
Use the arduino flasher to remove the problem:

NB: first disconnect GPIO16 as is connected to RESET for deep sleep wake up to work.

`~/.arduino15/packages/esp8266/tools/esptool/2.5.0-3-20ed2b9/esptool -vv -cd nodemcu -cb 921600 -cp /dev/ttyUSB0 -ca 0x00000 -cf build/transmitter.bin`

This wont result in a working system, but now regular flashing can be done:

`make -j4 flash monitor`
