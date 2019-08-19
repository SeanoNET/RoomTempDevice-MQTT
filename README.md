# RoomTempDevice-MQTT
Sends sensor data from a MXChip AZ3166 IoT Devkit to a MQTT Broker, a local/cheaper alternative to the cloud based solution [RoomTempDevice-IoT](https://github.com/SeanoNET/RoomTempDevice-IoT)

## Getting Started
* Run through [Get started - IoT DevKit guide](https://microsoft.github.io/azure-iot-developer-kit/docs/get-started/)
    * Prepare your development environment
    * Provision your device using the Azure DPS
    * Load your IoT Hub Device Connection String
    * Git clone && Upload Device Code


## Configuration

|| Description|
|---|---|
|`send_interval`| The interval at which the device will send the payload to the MQTT broker.|
|`setup_delay` | The delay the setup messages will be displayed on the screen.|
|`mqtt_server` | The MQTT broker server IP.|
|`username` | The MQTT broker server username.| 
|`password` | The MQTT broker server password.|     
|`port` | The MQTT broker server port.|    
|`topic` | The MQTT topic `home/room/temp-mon/data`.|    
|`client_id` | The MQTT device client id `temp-mon`.|        

## Payload 

An example of the MQTT message payload

```JSON
{
	"temperature": 31.2999990000000,
	"humidity": 47.0000000000000
}
```

## MQTT Broker

There are plenty of MQTT brokers to choose from. This example was implemented using [Eclipse Mosquitto](https://mosquitto.org/) running in docker. For information on how to setup and configure the Mosquitto MQTT broker see [eclipse-mosquitto](https://hub.docker.com/_/eclipse-mosquitto) or binary installation [downloads](https://mosquitto.org/download/)