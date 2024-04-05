/**************************************************************

This file is a part of
https://github.com/atiderko/espwebconfig

Copyright [2020] Alexander Tiderko

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

**************************************************************/
#ifndef BBS_MQTT_HA_H
#define BBS_MQTT_HA_H

#include <Arduino.h>
// #ifdef ESP32
// #include <mutex>
// #endif
#include "ewcMqtt.h"
#include "../ewcConfig.h"

/** Helper to provider the device to home assistant.
 * see https://www.home-assistant.io/integrations/mqtt
 */

namespace EWC
{

  // const char SEP[2] = "/";

  class HAPropertyConfig
  {
  public:
    String uniqueId;
    String component;
    String name = "";
    String deviceClass = "";
    String objectId;
    String unit;
    bool retained;
    bool settable = false;
    Mqtt::MqttMessageFunction callback = nullptr;
    HAPropertyConfig(String component, String uniqueId, String name, String deviceClass, String objectId, String unit, bool retained);
    HAPropertyConfig(String component, String uniqueId, String name, String deviceClass, String objectId, Mqtt::MqttMessageFunction callback, String unit = "", bool retained = true);
    String getStateTopic(String discoveryPrefix);
    String getCmdTopic(String discoveryPrefix);
  };

  class HADevice
  {
  public:
    String id;
    String name;
    String model;
    String serialNumber;
    String swVersion;
    String configurationUrl;
    bool added = false;
    HADevice() {}
  };

  class HAProperty
  {
  public:
    String uniqueId;
    String discoveryTopic;
    String stateTopic;
    String commandTopic = "";
    bool publishedConfig = false;
    bool settable = false;
    Mqtt::MqttMessageFunction callback = nullptr;
    // {
    //   "name":"Irrigation",
    //   "device_class":"temperature",
    //   "state_topic":"homeassistant/sensor/sensorBedroom/state",
    //   "command_topic":"homeassistant/switch/irrigation/set",
    //   "unit_of_measurement":"°C",
    //   "value_template":"{{ value_json.temperature}}",
    //   "unique_id":"temp01ae",
    //   "device":{
    //     "identifiers":[
    //         "bedroom01ae"
    //     ],
    //     "name":"Bedroom",
    //     "manufacturer": "Example sensors Ltd.",
    //     "model": "K9",
    //     "serial_number": "12AE3010545",
    //     "hw_version": "1.01a",
    //     "sw_version": "2024.1.0",
    //     "configuration_url": "https://example.com/sensor_portal/config"
    //    }
    // }
    JsonDocument jsonConfig;
    HAProperty(String discoveryPrefix, HAPropertyConfig &config, HADevice &device);
    uint16_t publishConfig(EWC::Mqtt &mqtt);
  };

  class MqttHA
  {
  public:
    MqttHA();
    ~MqttHA();

    void setup(EWC::Mqtt &mqtt, String deviceId, String deviceName, String model = "esp");

    /** Adds property to the device
     * <discovery_prefix>/<component>/<object_id>/state
     * :component: One of the supported MQTT integrations, eg. binary_sensor.
     * :uniqueId: to allow changes to the entity and a device mapping so we can group all sensors of a device together.
     * :name: We can set “name” to '' if we want to inherit the device name for the entity.
     * :deviceClass: If name is left away and a device_class is set, the entity name part will be derived from the device_class:
     * 'date', 'enum', 'timestamp', 'apparent_power', 'aqi', 'atmospheric_pressure', 'battery', 'carbon_monoxide', 'carbon_dioxide',
     * 'current', 'data_rate', 'data_size', 'distance', 'duration', 'energy', 'energy_storage', 'frequency', 'gas', 'humidity',
     * 'illuminance', 'irradiance', 'moisture', 'monetary', 'nitrogen_dioxide', 'nitrogen_monoxide', 'nitrous_oxide', 'ozone',
     * 'ph', 'pm1', 'pm10', 'pm25', 'power_factor', 'power', 'precipitation', 'precipitation_intensity', 'pressure', 'reactive_power',
     * 'signal_strength', 'sound_pressure', 'speed', 'sulphur_dioxide', 'temperature', 'volatile_organic_compounds',
     * 'volatile_organic_compounds_parts', 'voltage', 'volume', 'volume_storage', 'water', 'weight', 'wind_speed'
     * :objectID: The ID of the device. This is only to allow for separate topics for each device and is not used for the entity_id.
     *  The ID of the device must only consist of characters from the character class [a-zA-Z0-9_-] (alphanumerics, underscore and hyphen)
     */
    bool addProperty(String component, String uniqueId, String name, String deviceClass, String objectId, String unit = "", bool retained = true);
    /** Adds a settable property.
     * <discovery_prefix>/<component>/<object_id>/state
     * <discovery_prefix>/<component>/<object_id>/set
     */
    bool addPropertySettable(String component, String uniqueId, String name, String deviceClass, String objectId, Mqtt::MqttMessageFunction callback, String unit = "", bool retained = true);
    /** Publishes a value a property. */
    void publishState(String uniqueId, String value, bool retain = false, uint8_t qos = 1);

  protected:
    EWC::Mqtt *_ewcMqtt;
    // #ifdef ESP32
    //     std::mutex _mutex;
    // #endif
    HADevice _mqttDevice;
    String _homieStateTopic;
    String _statusTopic;
    std::vector<HAPropertyConfig> _propertyConfigs;
    std::vector<HAProperty> _properties;
    uint32_t _idxPublishConfig;
    uint16_t _waitForPacketId;

    bool _hasProperty(String uniqueId);

    void _onMqttConnect();
    void _onMqttMessage(String &topic, String &payload);
    void _onMqttAck(uint16_t packetId);
  };
};
#endif
