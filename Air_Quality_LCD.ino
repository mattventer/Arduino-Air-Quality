//Written by Matthew Venter

#include <Adafruit_CCS811.h>
#include <Adafruit_Si7021.h>
#include <LiquidCrystal.h>
#include <hpma115S0.h>
#include <SoftwareSerial.h>
#include <IRremote.h>


void drawTemplate40_4(LiquidCrystal lcd1, LiquidCrystal lcd2) {
  //Top
  lcd1.print("Temperature: ");
  lcd1.setCursor(19, 0);
  lcd1.print("F");
  lcd1.setCursor(23, 0);
  lcd1.print("eCO2: ");
  lcd1.setCursor(34, 0);
  lcd1.print("ppm");
  lcd1.setCursor(0, 1);
  lcd1.print("PM2.5: ");
  lcd1.setCursor(23, 1);
  lcd1.print("PM10: ");
  lcd2.print("Humidity: ");
  lcd2.setCursor(15, 0);
  lcd2.print("%");
  lcd2.setCursor(23, 0);
  lcd2.print("TVOC: ");
  lcd2.setCursor(34, 0);
  lcd2.print("ppb");
  lcd2.setCursor(0, 1);
  lcd2.print("Air Quality: ");
  lcd2.setCursor(23, 1);
  lcd2.print("PM2.5 Index: ");
}


//LCD 
const int rs = 12, en = 9, en2 = 13, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
LiquidCrystal lcd2(rs, en2, d4, d5, d6, d7);

//Sensors
static Adafruit_CCS811 ccs;
static Adafruit_Si7021 Si7021;

//Air Purifier Control
enum {off, low, med, high};
int state = off;
int qual = 0;
IRsend irsend;
//Temporary Vizio codes (Fan up = channel up; fan down = channel down)
int POWER = 0x10EF;
int FAN_UP = 0x00FF;
int FAN_DOWN = 0x807F;

SoftwareSerial hpmaSerial(11, 10);
HPMA115S0 hpma115S0(hpmaSerial);


//Used to calculate air quality
static float pm2_5standard = 12.0;


//Used to store data sent to LCD
float temp, hum;
int tvoc, co2, disp_index;
unsigned int pm2_5, pm10;
String overall_quality = "";

void setup() {
  Serial.begin(9600);
  hpmaSerial.begin(9600);
  delay(5000);
  while (!Serial)
    delay(10);
  bool res = ccs.begin();
  if (!res) {
    Serial.println("Couldn't find CCS811 sensor...");
    while(1);
  }
  if (!Si7021.begin()) {
    Serial.println("Couldn't find Si7021 sensor...");
    while(1);
  }
  hpma115S0.Init();
  hpma115S0.StartParticleMeasurement();
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  lcd2.begin(16, 2);
}

void loop() {
  drawTemplate40_4(lcd, lcd2);
  //Update data
  temp = (Si7021.readTemperature() * 1.8) + 32;
  hum = Si7021.readHumidity();
  if (ccs.available()) {
     ccs.readData();
     co2 = ccs.geteCO2();
     tvoc = ccs.getTVOC();
  }
  hpma115S0.ReadParticleMeasurement(&pm2_5, &pm10);

  
  Serial.print(temp);
  Serial.print(" ");
  Serial.print(hum);
  Serial.print(" ");
  Serial.print(co2);
  Serial.print(" ");
  Serial.print(tvoc);
  Serial.print(" ");
  Serial.print(pm2_5);
  Serial.print(" ");
  Serial.println(pm10);

  float pm25 = float(pm2_5);
  float index = (pm25/pm2_5standard) * 100.0;
  if (index < 50.0) {
    overall_quality = "Good";
    qual = 1;
  }
  else if (index >= 50.0 && index < 150.0) {
    overall_quality = "Moderate";
    qual = 2;
  }
  else {
    overall_quality = "Poor";
    qual = 3;
  }

  
  //Adjust Air Purifier mode
  if (state == off) {
     //Turn on purifier, set to low
     state = low;
  }
  else if (qual > state) {
    //Turn up fan
    state++;
  }
  else if (qual < state) {
    //Turn down fan
    state--;
  }
    
   
  
  //Update LCD
  //Temp
  lcd.setCursor(13, 0);
  lcd.print(temp);
  //eCO2
  lcd.setCursor(29, 0);
  lcd.print(co2);
  //PM2.5
  lcd.setCursor(7, 1);
  lcd.print(pm2_5);
  //PM10
  lcd.setCursor(29, 1);
  lcd.print(pm10);
  lcd2.setCursor(10, 0);
  lcd2.print(hum);
  lcd2.setCursor(29, 0);
  lcd2.print(tvoc);
  lcd2.setCursor(13, 1);
  lcd2.print(overall_quality);
  lcd2.setCursor(36, 1);
  disp_index = int(index);
  lcd2.print(disp_index);
  delay(15000);
  lcd.clear();
  lcd2.clear();
}
