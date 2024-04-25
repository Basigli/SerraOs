#include "WiFiS3.h"
#include "Arduino_LED_Matrix.h"
#include <ArduinoMqttClient.h>
#include <EEPROM.h>
#include <SimpleDHT.h>

// functions signatures
char* concatenateTopics(const char* arduinoId, const char* topic);
void generateRandomString(char *str, int length);
void handleSensor(int sensorPin, char *topic, bool isPerc, bool reversed, int lowerBound, int upperBound);
void handleDHTSensor(int sensorPin);
// --------------------
// WIFI and MQTT settings -----------------------------
char ssid[] = "Vodafone-34913283";    // your network SSID (name)
char pass[] = "cj4e6ma26yeh22t";    // your network password 

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

// ------------ obj ---------------------
WiFiClient wifiClient;
ArduinoLEDMatrix matrix;
MqttClient mqttClient(wifiClient);
SimpleDHT11 dht11;
// --------------------------------------

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
const int terrainHumidityPin = A0;
const int airHumidityTemperaturePin = 7;
const int lightQuantityPin = A3;

// Actuators pin
const int irrigationPumpPin = 12;

void setup() {
  Serial.begin(9600);
  matrix.begin();
  randomSeed(analogRead(0));
  pinMode(terrainHumidityPin, INPUT);
  pinMode(airHumidityTemperaturePin, INPUT);
  pinMode(lightQuantityPin, INPUT);
  pinMode(irrigationPumpPin, OUTPUT);

  digitalWrite(irrigationPumpPin, HIGH)

  // Reading data from EEPROM
  Serial.println("Reading data from EEPROM");
  ArduinoSettings readSettings;
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
    needSave = 0;
  }

  subscribeIrrigationPump = concatenateTopics(readSettings.arduinoId, "/IrrigationPump");
  subscribeUVLight = concatenateTopics(readSettings.arduinoId, "/UVLight");
  subscribeVentilation = concatenateTopics(readSettings.arduinoId, "/Ventilation");
  publishTerrainHumidity = concatenateTopics(readSettings.arduinoId, "/TerrainHumidity");
  publishLightQuantity = concatenateTopics(readSettings.arduinoId, "/LightQuantity");
  publishIsTankEmpty = concatenateTopics(readSettings.arduinoId, "/IsTankEmpty");
  publishAirTemperature = concatenateTopics(readSettings.arduinoId, "/AirTemperature");
  publishAirHumidity = concatenateTopics(readSettings.arduinoId, "/AirHumidity");
  // Connect to WiFi
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(ssid);
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    // failed, retry
    Serial.print("."); 
    delay(5000);
  }

  Serial.println("You're connected to the network");
  Serial.println();
  Serial.print("Attempting to connect to the MQTT broker.");

  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    matrix.renderBitmap(koFrame, 8, 12);
    while (1);
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

void sendMQTTMessage(const char* topic, int message) {
  //Serial.print("Publishing the following message at topic ");
  //Serial.print(topic);
  //Serial.println(": ");
  //Serial.println(message);
  mqttClient.beginMessage(topic);
  mqttClient.print(message);
  mqttClient.endMessage();
}

void onMqttMessage(int messageSize) {
  // we received a message, print out the topic and contents
  //char message[messageSize + 1];
  Serial.println("Received a message with topic '");
  Serial.print(mqttClient.messageTopic());
  Serial.print("', length ");
  Serial.print(messageSize);
  Serial.println(" bytes:");
  // use the Stream interface to print the contents
  int i = 0;
  //while (mqttClient.available()) {
    //Serial.print((char)mqttClient.read());
  //  message[i] = (char)mqttClient.read();
  //  i++;
  //}
  //message[i] = '\0';

  //Serial.print(message);
  Serial.println();

  Serial.println();

  switch (String(mqttClient.messageTopic())) {
    case subscribeIrrigationPump:
      char inChar;
      inChar = (char)mqttClient.read();
      Serial.print((char)mqttClient.read());
      bool value = false;

      if (inChar == '1') {
        Serial.println("turning on irrigation pump...");
        value = true;
      }
      if (inChar == '0') {
        Serial.println("turning off irrigation pump...");
        value = false;
      }
      digitalWrite(irrigationPumpPin, !value)
      break;
    case subscribeUVLight:
      // statements
      break;
    case subscribeVentilation:
      // statements
    default:
      // statements
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

void loop() {
  deltaTime1 = millis() - time1;
  if (deltaTime1 > executeEvery) {
    time1 = millis();
    handleSensor(terrainHumidityPin, publishTerrainHumidity, true, true, 349, 783);
    handleDHTSensor(airHumidityTemperaturePin);
    handleSensor(lightQuantityPin, publishLightQuantity, true, false, 0, 1023);
    sendMQTTMessage(publishIsTankEmpty, 0);
  }
  mqttClient.poll();
}
