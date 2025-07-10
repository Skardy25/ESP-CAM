[env:esp32cam]
platform = espressif32
board = esp32cam
framework = arduino
monitor_port = COM5
monitor_speed = 115200
lib_deps = 
	ArduinoJson
	densaugeo/base64@^1.4.0
