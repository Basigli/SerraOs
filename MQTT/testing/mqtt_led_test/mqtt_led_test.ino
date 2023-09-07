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
const char subscribe_topic[]  = "Led";


#define pinLed 4




void setup() {
  pinMode(pinLed, OUTPUT);

  // Create serial connection and wait for it to become available.
  Serial.begin(9600);
  while (!Serial) {
    ; 
  }

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

  Serial.print("Subscribing to topic: ");
  Serial.println(subscribe_topic);

  // subscribe to a topic
  mqttClient.subscribe(subscribe_topic);

  // topics can be unsubscribed using:
  // mqttClient.unsubscribe(topic);
  //mqttClient.setCallback(callback);
  Serial.print("Waiting for messages on topic: ");
  Serial.println(subscribe_topic);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("Callback function");
  char messageBuffer[30];
  memcpy(messageBuffer, payload, length);
  messageBuffer[length] = '\0';

}




void loop() {
  int messageSize = mqttClient.parseMessage();
  if (messageSize) {
    // we received a message, print out the topic and contents
    Serial.print("Received a message with topic '");
    Serial.print(mqttClient.messageTopic());
    Serial.print("', length ");
    Serial.print(messageSize);
    Serial.println(" bytes:");

    // use the Stream interface to print the contents
    while (mqttClient.available()) {

      char inChar;
      inChar = (char)mqttClient.read();
      Serial.print((char)mqttClient.read());
      bool value = false;

      if (inChar == '1') {
        Serial.println("accendo il led...");
        value = true;
      }
      if (inChar == '0') {
        Serial.println("spengo il led...");
        value = false;
      }
      //digitalWrite(pinLed, !digitalRead(pinLed));
      digitalWrite(pinLed, value);
    } 
    
    Serial.println();
  }

  // send message, the Print interface can be used to set the message contents
 
  delay(3000);
 

}
