
#include "WiFiS3.h"
#include <ArduinoMqttClient.h>

char ssid[] = "xxx";    // your network SSID (name)
char pass[] = "yyy";    // your network password 

char mqtt_user[] = "admin";
char mqtt_pass[] = "pass";


WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

const char broker[] = "192.168.1.22"; //IP address of the EMQX broker.
int        port     = 1883;
const char publish_topic[]  = "1.TerrainHumidity";

const int sensorPin = A0;

void setup() {
  Serial.begin(9600); // Initialize serial communication for debugging

  pinMode(sensorPin, INPUT);

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

  // You can provide a username and password for authentication
  mqttClient.setUsernamePassword(mqtt_user, mqtt_pass);

  Serial.print("Attempting to connect to the MQTT broker.");

  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());

    while (1);
  }

  Serial.println("You're connected to the MQTT broker!");
}

void loop() {
  // Read the analog value from the sensor
  int sensorValue = analogRead(sensorPin);

  // Map the sensor value to a moisture percentage (adjust min and max values as needed)
  int moisturePercentage = map(sensorValue, 0, 1023, 0, 100);

  mqttClient.beginMessage(publish_topic);
  mqttClient.print(moisturePercentage);
  mqttClient.endMessage();

  delay(1000); // Delay for a second before taking the next reading

}
