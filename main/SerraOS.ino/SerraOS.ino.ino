#include "WiFiS3.h"
#include "Arduino_LED_Matrix.h"
#include <ArduinoMqttClient.h>
#include <EEPROM.h>
#include <SimpleDHT.h>
#include <SHT1x-ESP.h>

// functions signatures
char* concatenateTopics(const char* arduinoId, const char* topic);
void generateRandomString(char *str, int length);
void handleSensor(int sensorPin, char *topic, bool isPerc, bool reversed, int lowerBound, int upperBound);
void handleDHTSensor(int sensorPin);
void handleSHTSensor();
void connectToWifiAndBroker();
// --------------------
// WIFI and MQTT settings -----------------------------
char ssid[] = "XXXX";    // your network SSID (name)
char pass[] = "YYYY";    // your network password 

struct ArduinoSettings {
  char ssid[32]; //  your network SSID (name)
  char password[32]; // your network password
  char arduinoId[6];
  bool keepWifiSettings;     // if set to false on reboot arduino will ask for the wifi name and pass
  bool keepArduinoId;        // if set to false on reboot arduino will generate a new id 
};

#define EEPROM_SIZE sizeof(ArduinoSettings)

const int EEPROM_ADDRESS = 0;
const char broker[] = "192.168.1.22"; //IP address of the EMQX broker.
int        port     = 1883;
char arduinoId[6];
// subscribe topics
char *subscribeIrrigationPump;
char *subscribeUVLight;
char *subscribeVentilation;
// ----------------
// publish topics
char *publishTerrainHumidity;
char *publishLightQuantity;
char *publishIsTankEmpty;
char *publishAirTemperature;
char *publishAirHumidity;
// --------------
// ----------------------------------------------------



uint8_t okFrame[8][12] = {
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0 },
  { 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0 },
  { 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0 },
  { 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0 },
  { 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0 },
  { 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

uint8_t koFrame[8][12] = {
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0 },
  { 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0 },
  { 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0 },
  { 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0 },
  { 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0 },
  { 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

unsigned long time1, deltaTime1;
int executeEvery = 5000;

// Sensors pin 
const int terraineHumidityTemperaturPin = 10;
const int clockPin = 11;
const int airHumidityTemperaturePin = 7;
const int lightQuantityPin = A3;

// Actuators pin
const int irrigationPumpPin = 12;
const int ventilationPin = 6;
const int uvLightPin = 5;

// ------------ obj ---------------------
WiFiClient wifiClient;
ArduinoLEDMatrix matrix;
MqttClient mqttClient(wifiClient);
SimpleDHT11 dht11;
ArduinoSettings readSettings;
SHT1x sht1x(terraineHumidityTemperaturPin, clockPin);
// --------------------------------------

void setup() {
  Serial.begin(9600);
  matrix.begin();
  randomSeed(analogRead(0));
  pinMode(terraineHumidityTemperaturPin, INPUT);
  pinMode(airHumidityTemperaturePin, INPUT);
  pinMode(lightQuantityPin, INPUT);
  pinMode(irrigationPumpPin, OUTPUT);
  pinMode(ventilationPin, OUTPUT);
  pinMode(uvLightPin, OUTPUT);


  digitalWrite(irrigationPumpPin, LOW);

  // Reading data from EEPROM
  Serial.println("Reading data from EEPROM");
 
  EEPROM.get(EEPROM_ADDRESS, readSettings);
  Serial.print("ssid: ");
  Serial.println(readSettings.ssid);
  Serial.print("pass: ");
  Serial.println(readSettings.password);
  Serial.print("arduinoId: ");
  Serial.println(readSettings.arduinoId);
  Serial.print("keepWifiSettings: ");
  Serial.println(readSettings.keepWifiSettings);
  Serial.print("keepArduinoId: ");
  Serial.println(readSettings.keepArduinoId);

  ArduinoSettings deafultArduinoSettings;
  strcpy(deafultArduinoSettings.ssid, "XXX");
  strcpy(deafultArduinoSettings.password, "XXX");
  strcpy(deafultArduinoSettings.arduinoId , "XXX");
  deafultArduinoSettings.keepWifiSettings = true;
  deafultArduinoSettings.keepArduinoId = true;

  if (checkEEPROM()) {
    Serial.println("No data on EEPROM");
    EEPROM.put(EEPROM_ADDRESS, deafultArduinoSettings);
  }
  bool needSave = false;      // change this variable to true if you want to save new settings on EEPROM
  if ((strcmp(readSettings.ssid, "XXX") == 0) || (strcmp(readSettings.password, "XXX") == 0) || (!readSettings.keepWifiSettings)) {
    // ask wifi name and password
    needSave = true;
  }
  if ((strcmp(readSettings.arduinoId, "XXX") == 0) || (!readSettings.keepArduinoId)) {
    generateRandomString(readSettings.arduinoId, 5);
    needSave = true;
  }
  if (needSave) {
    Serial.println("Writing on EEPROM...");
    EEPROM.put(EEPROM_ADDRESS, readSettings);
    needSave = false;
  }

  subscribeIrrigationPump = concatenateTopics(readSettings.arduinoId, "/IrrigationPump");
  subscribeUVLight = concatenateTopics(readSettings.arduinoId, "/UVLight");
  subscribeVentilation = concatenateTopics(readSettings.arduinoId, "/Ventilation");
  publishTerrainHumidity = concatenateTopics(readSettings.arduinoId, "/TerrainHumidity");
  publishLightQuantity = concatenateTopics(readSettings.arduinoId, "/LightQuantity");
  publishIsTankEmpty = concatenateTopics(readSettings.arduinoId, "/IsTankEmpty");
  publishAirTemperature = concatenateTopics(readSettings.arduinoId, "/AirTemperature");
  publishAirHumidity = concatenateTopics(readSettings.arduinoId, "/AirHumidity");

  connectToWifiAndBroker();
}

void sendMQTTMessage(const char* topic, int message) {
  Serial.print("Publishing the following message at topic ");
  Serial.print(topic);
  Serial.println(": ");
  Serial.println(message);
  mqttClient.beginMessage(topic);
  mqttClient.print(message);
  mqttClient.endMessage();
}

void onMqttMessage(int messageSize) {
  // we received a message, print out the topic and contents
  Serial.println("Received a message with topic '");
  Serial.print(mqttClient.messageTopic());
  Serial.print("', length ");
  Serial.print(messageSize);
  Serial.println(" bytes:");
  // use the Stream interface to print the contents
  Serial.println();
  Serial.println();

  char inChar = (char)mqttClient.read();
  
  struct TopicPinMapping {
    const char* topic;
    int pin;
  };

  // Define the mapping between topics and pins
  TopicPinMapping mappings[] = {
    {subscribeIrrigationPump, irrigationPumpPin},
    {subscribeUVLight, uvLightPin},
    {subscribeVentilation, ventilationPin}
  };

  // Determine the state based on the message content
  bool value = (inChar == '1');
  const char* action = value ? "turning on" : "turning off";

  // Iterate over the mappings to find the matching topic
  for (const auto& mapping : mappings) {
    if (mqttClient.messageTopic().equals(mapping.topic)) {
      Serial.print(action);
      Serial.print(" ");
      Serial.print(mapping.topic);
      Serial.println("...");
      digitalWrite(mapping.pin, value);
      break;
    }
  }
}


char* concatenateTopics(const char* arduinoId, const char* topic) {
  size_t len = strlen(arduinoId) + strlen(topic) + 1; 
  char* result = new char[len];
  strcpy(result, arduinoId);
  strcat(result, topic);
  return result;
}


void generateRandomString(char *str, int length) {
  static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

  for (int i = 0; i < length; ++i) {
    str[i] = alphanum[random(strlen(alphanum))];
  }
  str[length] = '\0'; 
}


bool checkEEPROM() {
  for (int i = 0; i < EEPROM_SIZE; i++) {
    if (EEPROM.read(i) != 255) {  // empty EEPROM contains values  255
      return false; 
    }
  }
  return true;  
}


void handleSensor(int sensorPin, char *topic, bool isPerc, bool reversed, int lowerBound, int upperBound) {
  // Read the analog value from the sensor
  int sensorValue = analogRead(sensorPin);
  // Map the sensor value to a percentage
  if (isPerc) {
    if (reversed) {
      sensorValue = map(sensorValue, lowerBound, upperBound, 100, 0);
    } else {
      sensorValue = map(sensorValue, lowerBound, upperBound, 0, 100);
    }
  }
  sendMQTTMessage(topic, sensorValue);
}

void handleDHTSensor(int sensorPin) {
  int err = SimpleDHTErrSuccess;
  byte airTemperature = 0;
  byte airHumidity = 0;
  if ((err = dht11.read(sensorPin, &airTemperature, &airHumidity, NULL)) != SimpleDHTErrSuccess){
    return;
  }
  sendMQTTMessage(publishAirTemperature, airTemperature);
  sendMQTTMessage(publishAirHumidity, airHumidity);
}


void handleSHTSensor() {
  float terrainHumidity, terrainTemperature;

  terrainHumidity = sht1x.readHumidity();
  terrainTemperature = sht1x.readTemperatureC();
  sendMQTTMessage(publishTerrainHumidity, terrainHumidity);
}


void connectToWifiAndBroker() {
  int attempts = 0;
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(ssid);
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    // failed, retry
    Serial.print("."); 
    delay(100);
    attempts++;
    if (attempts > 100) {
      delay(1000);
      NVIC_SystemReset(); 
    }
  }

  Serial.println("You're connected to the network");
  Serial.println();
  Serial.print("Attempting to connect to the MQTT broker.");

  while (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    matrix.renderBitmap(koFrame, 8, 12);
    delay(1000);
  }

  Serial.println("You're connected to the MQTT broker!");
  // topic sub
  Serial.print("Subscribing to topic: ");
  Serial.println(subscribeIrrigationPump);
  mqttClient.subscribe(subscribeIrrigationPump);
  
  Serial.print("Subscribing to topic: ");
  Serial.println(subscribeUVLight);
  mqttClient.subscribe(subscribeUVLight);
  
  Serial.print("Subscribing to topic: ");
  Serial.println(subscribeVentilation);
  mqttClient.subscribe(subscribeVentilation);
  
  // set the message receive callback
  mqttClient.onMessage(onMqttMessage);
  matrix.renderBitmap(okFrame, 8, 12);

}


void loop() {
  deltaTime1 = millis() - time1;
  if (deltaTime1 > executeEvery) {
    time1 = millis();
    int attempts = 0;
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Connection lost...");
      connectToWifiAndBroker();
    }
    handleSHTSensor();
    handleDHTSensor(airHumidityTemperaturePin);
    handleSensor(lightQuantityPin, publishLightQuantity, true, false, 0, 1023);
    sendMQTTMessage(publishIsTankEmpty, 0);
  }
  mqttClient.poll();
}
