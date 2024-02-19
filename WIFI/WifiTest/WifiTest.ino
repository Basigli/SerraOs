#include <WiFiS3.h>
//#include <WiFiWebServer.h>
#include <EEPROM.h>

const char *ssid = "ArduinoAP";
const char *password = "password";
const int eepromAddr = 0; // Indirizzo di memoria EEPROM per memorizzare le credenziali WiFi

WiFiServer server(80);

String page = "<!DOCTYPE html><html><head><title>Configurazione WiFi</title></head><body><h1>Inserisci le credenziali WiFi</h1><form action='/save' method='POST'>SSID: <input type='text' name='ssid'><br>Password: <input type='password' name='password'><br><input type='submit' value='Salva'></form></body></html>";
int status = WL_IDLE_STATUS;

void setup() {
  Serial.begin(9600);
    // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // Inizializza EEPROM
  //EEPROM.begin(512);
  // Indirizzo IP del server
  //IPAddress IP = WiFi.localIP();
  WiFi.config(IPAddress(192,48,56,2));
  // Inizializza modalità access point
  WiFi.beginAP(ssid, password);


  // Configura le rotte del server web
  /*
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", page);
  });

  server.on("/save", HTTP_POST, []() {
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    
    // Salva le credenziali WiFi nella memoria EEPROM
    saveCredentials(ssid, password);

    // Riavvia l'Arduino
    server.send(200, "text/plain", "Credenziali salvate. Riavvio in corso...");
    delay(1000);
    //ESP.restart();
  });
  */
  server.begin();
  Serial.println("Server avviato");
   // you're connected now, so print out the status
  printWiFiStatus();
}

void loop() {
   
  // compare the previous status to the current status
  if (status != WiFi.status()) {
    // it has changed update the variable
    status = WiFi.status();

    if (status == WL_AP_CONNECTED) {
      // a device has connected to the AP
      Serial.println("Device connected to AP");
    } else {
      // a device has disconnected from the AP, and we are back in listening mode
      Serial.println("Device disconnected from AP");
    }
  }
  
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    Serial.println("new client");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      delayMicroseconds(10);                // This is required for the Arduino Nano RP2040 Connect - otherwise it will loop so fast that SPI will never be served.
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out to the serial monitor
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            client.println(page);
            client.println();
            // break out of the while loop:
            break;
          }
          else {      // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        }
        else if (c != '\r') {    // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

     
        if (currentLine.endsWith("POST /save")) {
          saveCredentials("test", "test");
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}

void saveCredentials(String ssid, String password) {
  // Salva le credenziali nella memoria EEPROM
  /*
  for (int i = 0; i < ssid.length(); ++i) {
    EEPROM.write(eepromAddr + i, ssid[i]);
  }
  EEPROM.write(eepromAddr + ssid.length(), '\0'); // Aggiungi terminatore di stringa

  for (int i = 0; i < password.length(); ++i) {
    EEPROM.write(eepromAddr + ssid.length() + 1 + i, password[i]);
  }
  EEPROM.write(eepromAddr + ssid.length() + 1 + password.length(), '\0'); // Aggiungi terminatore di stringa

  EEPROM.commit();
*/
  return;
  Serial.print("ssid: ");
  Serial.print(ssid);
  Serial.print("      password: ");
  Serial.println(password);
  
}

void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);

}