#include <AZ3166WiFi.h>
#include <HTS221Sensor.h>
#include "MQTTClient.h"
#include "MQTTNetwork.h"
#include "RGB_LED.h"

#define RGB_LED_BRIGHTNESS 32

// Screen lines
#define HEADER 0
#define LINE_1 1
#define LINE_2 2
#define LINE_3 3

#define send_interval 2000
#define setup_delay 2000

static const float DEFAULT_TEMP = -1000;
static const float DEFAULT_HUMID = -1000;

DevI2C *i2c;
HTS221Sensor *sensor;

float humidity = 0;
float temperature = 0;
static bool hasWifi = false;
static bool hasSensors = false;
static RGB_LED rgbLed;

int status = WL_IDLE_STATUS;
int arrivedcount = 0;

const char* mqttServer = "10.1.1.97";   
int port = 1883;
const char* topic = "home/room/temp-mon/data";
char* clientId = "temp-mon";

///// Init
static void InitWifi()
{
  Screen.print(LINE_1, "Connecting WiFi...");

  if (WiFi.begin() == WL_CONNECTED)
  {
    Screen.print(LINE_2, "Connected...\r\n");   
    hasWifi = true;
    IPAddress ip = WiFi.localIP();
    Screen.print(LINE_3, ip.get_address());
  }
  else
  {
    hasWifi = false;
    Screen.print(LINE_2, "No Wi-Fi");
  }
}

static void InitSensors()
{
  Screen.print(LINE_1, "Init Sensors...",true);

  // Setup sensor
  i2c = new DevI2C(D14, D15);
  sensor = new HTS221Sensor(*i2c);

  // init sensor
  sensor->init(NULL);

  humidity = DEFAULT_HUMID;
  temperature = DEFAULT_TEMP;

  hasSensors = true;
}

//// Util

static void SetLEDError()
{
  rgbLed.setColor(RGB_LED_BRIGHTNESS,0, 0);
}

static void blinkLED()
{
  rgbLed.turnOff();
  rgbLed.setColor(0,0, RGB_LED_BRIGHTNESS);
  delay(500);
  rgbLed.turnOff();
}

float ReadTemperature()
{
  sensor->reset();

  float temperature = DEFAULT_TEMP;
  sensor->getTemperature(&temperature);
  return temperature;
}

float ReadHumidity()
{
  sensor->reset();

  float humidity = DEFAULT_HUMID;
  sensor->getHumidity(&humidity);
  return humidity;
}

MQTTNetwork *mqttNetwork;
MQTT::Client<MQTTNetwork, Countdown> *client;

void ConnectToMqqtServer()
{
  mqttNetwork = new MQTTNetwork();
  client = new MQTT::Client<MQTTNetwork, Countdown>(*mqttNetwork);

  char msgBuf[100];
  Screen.print(LINE_1, "Connecting to MQTT server..");
  sprintf(msgBuf, "Connecting to MQTT server %s:%d", mqttServer, port);
  Serial.println(msgBuf);

  blinkLED();

  int rc = mqttNetwork->connect(mqttServer, port);
  if (rc != 0) {
    Serial.println("Connected to MQTT server failed");
    Screen.print(LINE_1, "Failed to connect...");
    SetLEDError();
  } else {
    Serial.println("Connected to MQTT server successfully");
    Screen.print(LINE_1, "Connected successfully");
  }

  MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
  data.MQTTVersion = 3;
  data.clientID.cstring = clientId;
  data.username.cstring = (char*)"";
  data.password.cstring = (char*)"";
  
  if ((rc = client->connect(data)) != 0) {
      Serial.println("MQTT client connect to server failed");
      SetLEDError();
  }
  
  if ((rc = client->subscribe(topic, MQTT::QOS2, messageArrived)) != 0) {
      Serial.println("MQTT client subscribe from server failed");
      SetLEDError();
  }
}

void messageArrived(MQTT::MessageData& md)
{
    MQTT::Message &message = md.message;

    char msgInfo[60];
    sprintf(msgInfo, "Message arrived: qos %d, retained %d, dup %d, packetid %d", message.qos, message.retained, message.dup, message.id);
    Serial.println(msgInfo);

    sprintf(msgInfo, "Payload: %s", (char*)message.payload);
    Serial.println(msgInfo);
    ++arrivedcount;
}

void sendMqttMessage() 
{
  MQTT::Message message;
  int rc;

  // Sample sensors
  float t = ReadTemperature();
  float h = ReadHumidity();

  // Update Screen with sensor data
  char sensorLine[20];
  sprintf(sensorLine, "T:%.2f H:%.2f", t, h);
  Screen.print(LINE_3, sensorLine);

  // QoS 0
  char buf[100];
  snprintf(buf, 100, "{\"temperature\":%f,\"humidity\":%f}", t, h);

  message.qos = MQTT::QOS0;
  message.retained = false;
  message.dup = false;
  message.payload = (void*)buf;
  message.payloadlen = strlen(buf)+1;

  if ((rc = client->publish(topic, message)) == 0)
  {
    Serial.println("Message published successfully");
    Screen.print(LINE_1, "Sent successfully");
  }
  else
  {
    Serial.println("Failed to send MQTT message");
    Screen.print(LINE_1, "Failed to send message");
    SetLEDError();
  }
  client->yield(100);
}

void setup() {
  //Initialize serial and Wi-Fi:
  Serial.begin(115200);
  Screen.print(HEADER, "Setup...");

  hasWifi = false;

  // Check for Wifi connection
  InitWifi();

  if(!hasWifi){
    return;
  }
  
  // Init Sensors
  InitSensors();
  if(!hasSensors){
    return;
  }

  // Connect to MQTT server
  ConnectToMqqtServer();

  Serial.print("Sending interval at ");
  Serial.println(send_interval);

  delay(setup_delay);

  Screen.clean();
}

void loop() {
  Serial.println("\r\n>>>Running...");
  Screen.print(HEADER, "Running...");

  if (hasWifi && hasSensors)
  {
    sendMqttMessage();
  }

  delay(send_interval);
}
