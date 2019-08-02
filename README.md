Smart-Home-Device-ESP8266

ESP8266 connect to MQTT broker, subscribe for topic corresponding to device ("light", "fan", ...)

When new message come, control device on/off based on message

IDE: Arduino IDE 1.8.7

Boards:
	- esp8266 	by ESP8266 Community version 2.5.2

Configure board:
	- Board:		"NodeMCU 1.0 (ESP-12E Module)"
	- Upload speed:		"115200"
	- CPU Frequency: 	"80 MHz"
	- Flash size: 		"4M (3M SPIFFS)"
	- IwIP Variant:		"v2 Lower Memory"
	- VTables:		"Flash"
	- SSL Support:		"All SSL ciphers(most compatible)"
	- Programmer:		"Parallel Programmer"

Library:
	- PubSubClient	by Nick O'Leary version 2.7.0
	- ArduinoJson 	by Benoit Blanchon version 6.11.3
	- Adafruit SleepyDog Library	by Adafruit version 1.1.3