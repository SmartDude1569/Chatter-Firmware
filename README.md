# Chatter 1/2 Firmware

This is a fork of the Chatter 2.0 firmware with some new features:
- Support for original Chatter
- Cross-communication between original Chatter and Chatter 2.0

## Installation instructions:
- Download [Arduino 1.x](https://www.arduino.cc/en/software) (Arduino 2.x is not compatible with the plugin below)
- Install the [ESP32 FS Uploader plugin](https://github.com/me-no-dev/arduino-esp32fs-plugin) to upload images from the data folder to the device
- Add "https://raw.githubusercontent.com/CircuitMess/Arduino-Packages/master/package_circuitmess.com_esp32_index.json" to Additional Board Manager URLs and pick "Chatter" as the board
- Run Tools -> ESP32 Sketch Data Upload
- Upload sketch

## Planned features:
- Group chats
- More games