## Integration of **MQTT** extension with Homie support

```cpp
#include <Arduino.h>
#include <ewcConfigServer.h>
#include <extensions/ewcMqtt.h>
#include <extensions/ewcMqttHomie.h>

EWC::ConfigServer server;
EWC::Mqtt ewcMqtt;
EWC::MqttHomie homieMqtt;
uint64_t tsSendMqttState = 0;

using namespace EWC;

void _onMqttCallback(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
    I::get().logger() << "set by MQTT; topic: " << topic << ", payload: " << payload << endl;
}


void setup() {
    Serial.begin(115200);
    // add MQTT configuration
    I::get().configFS().addConfig(ewcMqtt);
    // setup homie device
    homieMqtt.setup(ewcMqtt, "My Homie Device", "ewc-"+I::get().config().getChipId());
    // with homie pode
    homieMqtt.addNode("mynode", "My Homie Node");
    // and homie properties
    homieMqtt.addProperty("mynode", "my_property_id", "My Property", "integer", "0:100", "%");
    homieMqtt.addPropertySettable("mynode", "my_property_id_c1", "My callable property", "boolean", _onMqttCallback);
    // start webServer
	server.setup();
}


void loop() {
    // process dns requests and connection state AP/STA
    server.loop();
    // MQTT uses async library, no need to call loop
    if (ewcMqtt.client().connected()) {
        // send mqtt state every 10 seconds
        if (millis() - tsSendMqttState > 10000) {
            homieMqtt.publishState("mynode", "my_property_id_c1", String("false"));
            // publish example value of 35%
            homieMqtt.publishState("mynode", "my_property_id", String(35));
            tsSendMqttState = millis();
        }
    }
    delay(1);
}
```

You can remove Homie dependency and use **AsyncMqttClient** directly with _ewcMqtt.client()_.
