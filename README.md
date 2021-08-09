# ESP8266-MQTT-Lamps
Code for a pair of ESP8266s with RGB LEDs. They will communicate with an MQTT broker so each lamp will control the other's color. To send a color, the button is pushed, and with each push, the lamp cycles through the array of colors. When the button is not pushed for a certain length of time, the lamp sends that color and resumes glowing its assigned color (as assigned by the other lamp). The button is also used for WPS (see below). The potentiometer controls brightness and is heavily averaged to produce a smooth brightness adjustment.
# State of the project:
I am a student and do not have the time to keep a close eye on this. It does not work as well as I wish it did, but it is also not finished yet. This is meant to be a starting point so people don't have to build up the code how I had to. Please feel free to fork and improve as that is the spirit of the project.
# WPS Info
The ESP8266 will remember the last successfully used WiFi credentials in non-volitile memory, so if it loses connection or power, it'll try the last used credentials. The WPS pushbutton can be used to start WPS if the last used WiFi credentials don't work. If the builtin LED indicates WPS is being requested, push the WPS button on your router, then push the WPS button on the ESP8266.
# Builtin LED Indications
1 Flash: Successfully booted <br />
2 Flashes: Connected to WiFi <br />
Flashing on half-second intervals: Requesting WPS <br />
Flashing on two-second intervals: Connected to MQTT broker <br />
