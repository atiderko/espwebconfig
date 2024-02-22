## Integration of **Time** extension

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
    // start webServer
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
