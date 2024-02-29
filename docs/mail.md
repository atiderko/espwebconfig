## Integration of **Mail** extension

```cpp
#include <Arduino.h>
#include <ewcConfigServer.h>
#include <extensions/ewcMail.h>

EWC::ConfigServer server;
EWC::Mail ewcMail;
bool mailSend = false;

void setup() {
    EWC::I::get().logger().setBaudRate(115200);
    EWC::I::get().logger().setLogging(true);

    // add mail configuration
    EWC::I::get().configFS().addConfig(ewcMail);
    // start webServer
    server.setup();
}


void loop() {
    // process dns requests and connection state AP/STA
    server.loop();
    // should be called to perform send and receive ACK
    ewcMail.loop();
    if (WiFi.status() == WL_CONNECTED) {
        // do your stuff if connected
        if (!mailSend) {
            mailSend = ewcMail.sendChange("Subject", "Device connected");
        }
    } else {
        // or if not yet connected
    }
    delay(1);
}
```
