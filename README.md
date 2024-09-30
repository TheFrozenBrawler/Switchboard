# Switchboard
This is project created as a task in PUT Rocketlab student group. It was developed to handle a board with switches that enable control of rocket infrastracure. Key features of this project are robustness, ease of use and hassle-free communication. It's written in C/C++ with usage of FreeRTOS. To assure robustness it was divided into prioritized tasks. Hardware platform is ESP32-S3 Dev Kit C1 by espressif and switch board created by other group member.

# Components
The project consists of a few main components and features:
- battery handler
- handler of user interface consisting of LED
- Google Protocol Buffer mechanism  for serializing data
- WiFi connection
- MQTT communication
