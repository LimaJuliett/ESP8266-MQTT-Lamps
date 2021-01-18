# ESP8266-MQTT-PubSubClient
Some generic Arduino IDE code for the ESP8266 to communicate with an MQTT broker. This uses the PubSubClient and ESP8266WiFi libraries. Support for WPS is included, but you can also hard-code WiFi creds.
# WPS Info
The ESP8266 will remember the last successfully used WiFi credentials in non-volitile memory, so if it loses connection or power, it'll try the last used credentials. The WPS pushbutton can be used to start WPS if the last used WiFi credentials don't work. If the builtin LED indicates WPS is being requested, push the WPS button on your router, then push the WPS button on the ESP8266.
# Builtin LED Indications
1 Flash: Successfully booted <br />
2 Flashes: Connected to WiFi <br />
Flashing on half-second intervals: Requesting WPS <br />
Flashing on two-second intervals: Connected to MQTT broker <br />
