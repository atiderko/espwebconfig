# ESP8266/ESP32 Web Configuration

Yet another library web configuration library for ESP8266.

## Why another

There are already many cool web-based configuration libraries out there, but I haven't found one that suits all my needs. Some of these libraries are [AutoConnect](https://github.com/Hieromon/AutoConnect), [ESPAsyncWiFiManager](https://github.com/alanswx/ESPAsyncWiFiManager), [Homie](https://github.com/homieiot/homie-esp8266), [IoTWebConf](https://github.com/prampec/IotWebConf),  [JeeUI](https://github.com/jeecrypt/JeeUIFramework), [JeeUI2](https://github.com/jeecrypt/JeeUI2), [WiFiManager](https://github.com/tzapu/WiFiManager) or [ESPUI](https://github.com/s00500/ESPUI). The restrictions caused by blocking the startup process, the number of user-defined parameters, customizing websites or heap hunger are reasons for the development of this library.

## Features

- Modern web design // _based on [AutoConnect](https://github.com/Hieromon/AutoConnect)_
- Simple integration into your sketch
- No blocking, even if not yet connected
- Pages are created on the client site by JavaScript
- Logging using __<<-operator__ // _based on [Homie](https://github.com/homieiot/homie-esp8266)_
- Reset configuration by double press on reset button // _based on https://github.com/datacute/DoubleResetDetector_
- Interface to store runtime data in RTC memory.
- Option to add own languages.
- Extensions for __OTA Update__, __MQTT__ or __Time__ setup pages.
- Create you own pages and test the functionality with a webserver of your choice. Include the HTML, CSS and JS file as auto generated header files // _based on [ESPUI](https://github.com/s00500/ESPUI)_

## Integration

Below you see the simplest sketch with full functionality.

```cpp
#include <Arduino.h>
#include <ewcConfigServer.h>

EWC::ConfigServer server;

void setup() {
    Serial.begin(115200);
    // start webserver
	server.setup();
}


void loop() {
    // process dns requests and connection state AP/STA
    server.loop();
    if (WiFi.status() == WL_CONNECTED) {
        // do your stuff if connected
    } else {
        // or if not yet connected
    }
    delay(1);
}
```
On first start it creates a captive portal where you can enter your credentials to connect to your WiFi. The credentials are stored by Arduino WiFi library.

<img src="docs/images/wifi_not_connected.png" width="200">&emsp;<img src="docs/images/wifi_connected.png" width="200">

Default _Info_ and _Access-Configuration_ pages:

<img src="docs/images/info.png" width="200">&emsp;<img src="docs/images/access.png" width="200">

## Add _Time_ extension
```cpp
#include <Arduino.h>
#include <ewcConfigServer.h>
#include <extensions/ewcTime.h>

EWC::ConfigServer server;
EWC::Time ewcTime;
bool timePrinted = false;

void setup() {
    Serial.begin(115200);
    // add time configuration
    EWC::I::get().configFS().addConfig(ewcTime);
    // start webserver
	server.setup();
}


void loop() {
    // process dns requests and connection state AP/STA
    server.loop();
    if (WiFi.status() == WL_CONNECTED) {
        if (ewcTime.ntpAvailable() && !timePrinted) {
            timePrinted = true;
            // print current time
            EWC::I::get().logger() << "Current time:" << ewc.str() << endl;
            // or current time in seconds
            EWC::I::get().logger() << "  as seconds:" << ewcTime.currentTime() << endl;
        }
    } else {
        // or if not yet connected
    }
    delay(1);
}
```