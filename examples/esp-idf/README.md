# PsychicHttp - ESP IDF Example
*  Install ESP IDF 4.4.6 follow the guide: [ESP-IDF Guide](https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32/get-started/index.html#step-2-get-esp-idf)
*  Clone the project with command --recusive to include all sub module
*  Run build command: idf.py build in the examples/esp-idf folder
*  Flash the spiffs with data folder with command: esptool.py write_flash --flash_mode dio --flash_freq 40m --flash_size 4MB 0x317000 build/spiffs.bin
*  Flash the app with command: idf.py flash monitor and enter the IP address show in console.
*  Learn more about [Arduino as ESP-IDF Component](https://docs.espressif.com/projects/arduino-esp32/en/latest/esp-idf_component.html)

