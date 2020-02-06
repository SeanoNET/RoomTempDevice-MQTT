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

///// Config
#define send_interval 2000
#define setup_delay 2000

const char* mqtt_server = "10.1.1.201";   
int port = 1883;
const char* topic = "home/room/temp-mon/data";
char* client_id = "temp-mon";
char* username = "";
char* password = "";

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
  sprintf(msgBuf, "Connecting to MQTT server %s:%d", mqtt_server, port);
  Serial.println(msgBuf);

  blinkLED();

  int rc = mqttNetwork->connect(mqtt_server, port);
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
  data.clientID.cstring = client_id;
  data.username.cstring = username;
  data.password.cstring = password;
  
  if ((rc = client->connect(data)) != 0) {
      Serial.println("MQTT client connect to server failed");
      SetLEDError();
  }
  
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
    Screen.print(LINE_1, "OK");
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
  InitWifi();

  if(!hasWifi){
    return;
  }
  
  // Init Sensors
  InitSensors();
  if(!hasSensors){
    return;
  }

  // Connection to MQTT server
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
  else
  {
    Screen.print(LINE_1, "An error has occurred");
    SetLEDError();
  }
  

  delay(send_interval);
}

