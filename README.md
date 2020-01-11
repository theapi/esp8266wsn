# esp8266WSN

ESP8266 Wireless Sensor Network 

## Transmitter
Soil moisture sensors that transmit their data to the ESPNOW network.

- [Hardware](./transmitter/hardware)
- [Schematic](./transmitter/hardware/transmitter.pdf)
- [Software](./transmitter/software)


## Receiver
The receiver for the ESPNOW network, which sends the data to the network bridge.

- [Software](./receiver)


## Network Bridge
Connects the ESPNOW network to the WiFi network.

- [Software](./network_bridge)


## Monitor
A application using SDL2 that monitors the UDP messages from the network bridge. 

- [Software](./monitor)
