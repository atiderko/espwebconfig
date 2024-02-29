## Integration of **Updater** extension

```cpp
#include <Arduino.h>
#include <ewcConfigServer.h>
#include <extensions/ewcUpdater.h>

EWC::ConfigServer server;
EWC::Updater ewcUpdater;

void setup() {
    EWC::I::get().logger().setBaudRate(115200);
    EWC::I::get().logger().setLogging(true);

    // add updater configuration
    EWC::I::get().configFS().addConfig(ewcUpdater);
    // start webServer
    server.setup();
}


void loop() {
    // process dns requests and connection state AP/STA
    server.loop();
    // should be called to perform reboot after successful update
    ewcUpdater.loop();
    if (WiFi.status() == WL_CONNECTED) {
        // do your stuff if connected
    } else {
        // or if not yet connected
    }
    delay(1);
}
```
