//Versione SerraOS_1.0
#include "logos.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Time.h>
#include <Wire.h>
#include <SimpleDHT.h>
#include <virtuabotixRTC.h> 



const char compileTIME[] = __TIME__;

virtuabotixRTC RTC(5, 4, 8);
SimpleDHT11 dht11;
Adafruit_SSD1306 display(128, 64);

#define PinTH 7
#define Iluci A3
#define Oluci 9
#define PinVentola 3
#define PinPompa 10
#define pinTasto 6
#define pinTerreno A0 
unsigned long t1, dt1, t2, dt2, t3, dt3;


int L;
int Lperc;
bool Vluci; 
bool Vvent;
bool tasto;
bool prev_tasto;
int stato = 0;
bool oneTime1 = true;
bool oneTime2 = true;
bool oneTime3 = true;
bool oneTime4 = true;
byte TEMP = 0;
byte HUM = 0;
int err = SimpleDHTErrSuccess;
int ValoreTerreno; 
int TerrenoPerc;
int Vpump;

void setup() {
  pinMode(PinPompa, OUTPUT);
  pinMode(Oluci, OUTPUT);
  pinMode(Iluci, INPUT);
  pinMode(pinTasto, INPUT);
  pinMode(PinVentola, OUTPUT);
  pinMode(PinTH, INPUT);
  delay(100); 
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  
  display.clearDisplay();  
  display.setTextColor(WHITE);  
  display.setRotation(0);  
  display.setTextWrap(false);  
  display.dim(0);  
  Serial.begin(9600);
  setClock();
  boot();
}

//---------------------Inizializzazione orologio RTC-------------------------------------------------------------------------
void setClock() {
  int h = convert(compileTIME[0], compileTIME[1]);
  int m = convert(compileTIME[3], compileTIME[4]);
  int s = convert(compileTIME[6], compileTIME[7]);
  RTC.setDS1302Time(s, m, h, 6, 3, 10, 2020);     // seconds, minutes, hours, day of the week, day of the month, month, year
  }
  int convert(char a, char b) {
    int intA = a - '0';
    int intB = b - '0';
    return intB + (intA * 10);
  }
//----------------------------------------------------------------------------------------------------------------------------

void boot(){
  
  // gestione display logo avvio
  
  display.clearDisplay();
  display.setCursor(0,0);
  display.drawBitmap(0, 1, STARTlogo_bmp, 117 , 64, 1);
  display.display();
  delay(3000);
  display.clearDisplay();
  display.display();
  }

void illuminazione(){
    dt2 = millis() - t2;
    if (dt2 >= 1000){ 
      t2 = millis();
      for(int i = 0; i < 10; i++){
        L += analogRead(Iluci);
        }
      L = L/10;  
      Lperc = map(L, 0, 1024, 0, 100);
      }
  if (Lperc < 40){
    Vluci = 1;
    }
  if (Lperc > 70){
    Vluci = 0;
    }
  digitalWrite(Oluci, Vluci);
  }

void TempHum(){
  if ((err = dht11.read(PinTH, &TEMP, &HUM, NULL)) != SimpleDHTErrSuccess){
    return;
    }
  if (TEMP > 23){
    Vvent = 1; 
    }
  if (TEMP < 20) {
    Vvent = 0;
    }
    digitalWrite(PinVentola, Vvent);
  }

void Irrigazione(){
  ValoreTerreno = analogRead(pinTerreno);
  //Serial.println(ValoreTerreno);
  TerrenoPerc = map(ValoreTerreno, 950, 350, 0, 100);
  if (ValoreTerreno < 700){
    Vpump = 1;
    }
  if (ValoreTerreno > 800){
    Vpump = 0;
    }
  if (RTC.hours == 7 && RTC.minutes == 30){
    Vpump = 1;
    }
  if (RTC.hours == 7 && RTC.minutes == 33){
    Vpump = 0;
    }
  digitalWrite(PinPompa, Vpump);
  }

  
  
void Display(){
  switch (stato){
    case 0:  
//---------------------------------INTERFACCIA GENERALE-------------------------------
//--------------------ILLUMINAZIONE------------------------------
      display.clearDisplay();
      display.setTextSize(0);
      display.setCursor(15, 0);
      display.println("SerraOS_1.0");
      display.setCursor(90, 0);
      RTC.updateTime();
      display.print(RTC.hours);
      display.print(":");
      display.print(RTC.minutes);
      display.drawLine(0, 8, 200, 8, WHITE);  // Draw line (x0,y0,x1,y1,color)
      display.setCursor(0, 16);
      display.print("LUCE : ");
      display.print(Lperc);
      display.print("%");
      if (Vluci == 1){
        display.drawBitmap(115, 15, logo12_lamp_bmp, 12 , 12, 1);
        }
      if (Vvent == 1){
        display.drawBitmap(115, 30, fan12_lamp_bmp, 12 , 12, 1);
        }
    //----------------------------TEMPERATURA E UMIDITA'-----------
      display.setCursor(0, 29);
      display.print("TEMPERATURA : ");
      display.print(TEMP);
      display.drawRect(98, 29, 2, 2, WHITE);  // Draw rectangle (x,y,width,height,color)
      display.print(" C");
      display.setCursor(0, 42);
      display.print("UMIDITA' : ");
      display.print(HUM);
      display.print("%");
    //----------------------IRRIGAZIONE------------------------------
      display.setCursor(0, 55);
      display.print("IRRIGAZIONE : ");
      if (Vpump == 1){
        display.print("ON");
        }
      if (Vpump == 0){
        display.print("OFF");
        }
      display.display();
    break;
    case 1:         //dettaglio luce
      oneTime4 = true;
      if (oneTime1){
        display.clearDisplay();
        display.setTextSize(0);
        display.drawBitmap(0, 0, logo_biglamp_bmp, 128, 64, 1);
        display.display();
        delay(1000);
        oneTime1 = false;
        }
      display.clearDisplay();
      display.setTextSize(0);
      display.setCursor(15, 0);
      display.println("SerraOS_1.0");
      display.setCursor(90, 0);
      RTC.updateTime();
      display.print(RTC.hours);
      display.print(":");
      display.print(RTC.minutes);
      display.drawLine(0, 8, 200, 8, WHITE);  // Draw line (x0,y0,x1,y1,color)
      display.setTextSize(2);
      display.setCursor(0, 16);
      display.print("LUCE: ");
      display.print(Lperc);
      display.print("%");
      if (Vluci == 1){
        display.setTextSize(0);
        display.setCursor(5, 40);
        display.print("Illuminazione UV ON");
        display.drawBitmap(54, 50, logo12_lamp_bmp, 12 , 12, 1);
        }
      display.display();
    break;
    case 2:        //dettaglio temperatura     
      oneTime1 = true;
      if (oneTime2){
        display.clearDisplay();
        display.setTextSize(0);
        display.drawBitmap(0, 0, TEMPlogo_bmp, 128, 64, 1);
        display.display();
        delay(1000);
        oneTime2 = false;
        }
      display.clearDisplay();
      display.setTextSize(0);
      display.setCursor(15, 0);
      display.println("SerraOS_1.0");
      display.setCursor(90, 0);
      RTC.updateTime();
      display.print(RTC.hours);
      display.print(":");
      display.print(RTC.minutes);
      display.drawLine(0, 8, 200, 8, WHITE);  // Draw line (x0,y0,x1,y1,color)
      display.setTextSize(2);
      display.setCursor(0, 16);
      display.print("TEMP: ");
      display.print(TEMP);
      display.drawRect(100, 16, 4, 4, WHITE);  // Draw rectangle (x,y,width,height,color)
      display.print(" C");
      display.display();
    break;
    case 3:       //dettaglio umidità
      oneTime2 = true;
      if (oneTime3){
        display.clearDisplay();
        display.setTextSize(0);
        display.drawBitmap(0, 2, HUMlogo_bmp, 128, 64, 1);
        display.display();
        delay(1000);
        oneTime3 = false;
        }
      display.clearDisplay();
      display.setTextSize(0);
      display.setCursor(15, 0);
      display.println("SerraOS_1.0");
      display.setCursor(90, 0);
      RTC.updateTime();
      display.print(RTC.hours);
      display.print(":");
      display.print(RTC.minutes);
      display.drawLine(0, 8, 200, 8, WHITE);  // Draw line (x0,y0,x1,y1,color)
      display.setTextSize(2);
      display.setCursor(0, 16);
      display.print("HUM: ");
      display.print(HUM);
      display.print("%");
      display.display();
    break;
    case 4:       //dettaglio irrigazione 
      oneTime3 = true;
      if (oneTime4){
        display.clearDisplay();
        display.setTextSize(0);
        display.drawBitmap(0, 6, IRRlogo_bmp, 128, 64, 1);
        display.display();
        delay(1000);
        oneTime4 = false;
        }
      display.clearDisplay();
      display.setTextSize(0);
      display.setCursor(15, 0);
      display.println("SerraOS_1.0");
      display.setCursor(90, 0);
      RTC.updateTime();
      display.print(RTC.hours);
      display.print(":");
      display.print(RTC.minutes);
      display.drawLine(0, 8, 200, 8, WHITE);  // Draw line (x0,y0,x1,y1,color) 
      display.setTextSize(2);
      display.setCursor(0, 16);
      display.println("HUM ");
      display.print("TERRA: ");
      display.print(ValoreTerreno);
      //display.print(TerrenoPerc);
      //display.println("%");
      display.setTextSize(0);
      display.setCursor(0, 55);
      display.print("IRRIGAZIONE: "); 
      if (Vpump == 1){
        display.print("ON");
        }
      if (Vpump == 0){
        display.print("OFF");
        }
      display.display();
    break;
    case 5:
      display.clearDisplay(); 
      display.display();
    }
  }

void SerialMonitor(){
  //Serial.println(compileDATE);
  //Serial.println(compileTIME);
  Serial.println(compileTIME);
  Serial.print("ORARIO : ");
  Serial.print(RTC.hours);
  Serial.print(":");
  Serial.println(RTC.minutes);
  Serial.print("LUCE : ");
  Serial.print(Lperc);
  Serial.println("%");
  Serial.print("TEMP : ");
  Serial.print(TEMP);
  Serial.println("°C");
  Serial.print("HUM : ");
  Serial.print(HUM);
  Serial.println("%");
  Serial.print("IRRIGAZIONE : ");
  Serial.println(Vpump);
  Serial.print("HUM TERRA : ");
  Serial.println(ValoreTerreno);
  Serial.print("LAMPADA UV : ");
  Serial.println(Vluci);
  Serial.println("------------------------------------");
  }




void OS(){
  dt3 = millis() - t3;
    if(dt3 >= 50){
      t3 = millis();
      tasto = digitalRead(pinTasto);       //fronte salita e discesa 
      if(!prev_tasto && tasto){
        stato = stato + 1;
        if (stato > 5){
          stato = 0;
          }
        Serial.println(stato);
        prev_tasto = tasto;
        }  
      if(prev_tasto && !tasto){
        prev_tasto = tasto;
        }
      } 
  dt1 = millis() - t1; 
    if (dt1 >= 1000){
      t1 = millis();
      Display();
      SerialMonitor();
      }
  
  illuminazione();
  TempHum();
  Irrigazione();
  }

void loop() {

  OS();    
  
}
