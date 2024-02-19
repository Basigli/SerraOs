#include "WiFiS3.h"
#include "Arduino_LED_Matrix.h"
#include <ArduinoMqttClient.h>
#include <EEPROM.h>

// functions signatures
char* concatenateTopics(const char* arduinoId, const char* topic);
void generateRandomString(char *str, int length);
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

WiFiClient wifiClient;
ArduinoLEDMatrix matrix;
MqttClient mqttClient(wifiClient);

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

void setup() {
  Serial.begin(9600);
  matrix.begin();
  randomSeed(analogRead(0));

  // Read data from EEPROM
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
  Serial.println("Received a message with topic '");
  Serial.print(mqttClient.messageTopic());
  Serial.print("', length ");
  Serial.print(messageSize);
  Serial.println(" bytes:");

  // use the Stream interface to print the contents
  while (mqttClient.available()) {
    Serial.print((char)mqttClient.read());
  }
  Serial.println();

  Serial.println();
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


void loop() {
  deltaTime1 = millis() - time1;
  if (deltaTime1 > executeEvery) {
    time1 = millis();
    sendMQTTMessage(publishTerrainHumidity, 10);
    sendMQTTMessage(publishLightQuantity, 20);
    sendMQTTMessage(publishIsTankEmpty, 0);
    sendMQTTMessage(publishAirTemperature, 40);
    sendMQTTMessage(publishAirHumidity, 50);
  }
  mqttClient.poll();
}
