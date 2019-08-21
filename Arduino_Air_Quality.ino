//Written by Matthew Venter

#include <Adafruit_CCS811.h>
#include <Adafruit_Si7021.h>
#include <LiquidCrystal.h>
#include <HPMA115S0.h>
#include <SoftwareSerial.h>
#include <IRremote.h>
#include <MQ2.h>
#include <MQ7.h>
#include <Keypad.h>

//Used to store data sent to LCD
float temp, hum, index, smoke, co, gas;
int tvoc, co2, disp_index, alc;
unsigned int pm2_5, pm10;


int quality, prev_qual = 0;
const int pm2_5standard = 12;
int count = 0;
String qual = "";
String alc_level = "xxx";

//RGB LED
const int red_led_pin = 2;
const int green_led_pin = 3;
const int blue_led_pin = 4;
int led_blinker = 0;

//Keypad
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
const byte rowPins[ROWS] = {32, 33, 34, 35}; //connect to the row pinouts of the keypad
const byte colPins[COLS] = {36, 37, 38, 39}; //connect to the column pinouts of the keypad
char key;//To store input
char prev_key;
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);


//LCD
const int rs = 31, en = 27, en2 = 29, d4 = 24, d5 = 26, d6 = 22, d7 = 30;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
LiquidCrystal lcd2(rs, en2, d4, d5, d6, d7);
int DELAY;

//Sensors
//SCL / SDA
static Adafruit_CCS811 ccs;
static Adafruit_Si7021 Si7021;
const int HPMA_TX = 10;
const int HPMA_RX = 11;
SoftwareSerial hpmaSerial(HPMA_TX, HPMA_RX);
HPMA115S0 hpma115S0(hpmaSerial);

//Analog Sensors
const int MQ2_Analog_Input = A0;
MQ2 mq2(MQ2_Analog_Input);
const int MQ7_Analog_Input = A1;
MQ7 mq7(MQ7_Analog_Input, 5.0);
const int MQ3_Analog_Input = A2;


//IR Receiver
const int IR_RECV_PIN = 28;
IRrecv irrecv(IR_RECV_PIN);
decode_results results;
enum scr {welcome};
unsigned long ir_val = welcome;
unsigned long prev_ir_val = 1;
//IR codes, specific to Elegoo remote
#define code_pwr 16753245
#define code_volup 16736925
#define code_voldwn 16754775
#define code_func 16769565
#define code_back 16720605
#define code_play_pause 16712445
#define code_fwd 16761405
#define code_dwn 16769055
#define code_up 16748655
#define code_0 16738455
#define code_1 16724175
#define code_2 16718055
#define code_3 16743045
#define code_4 16716015
#define code_5 16726215
#define code_6 16734885
#define code_7 16728765
#define code_8 16730805
#define code_9 16732845

//Piezo Buzzer
static int piezo_pin = 5;
int pitch = 2500;

/**
 * Functions to be moved to separate file after cleaned up
 */

void playStartTone() {
  tone(piezo_pin, 900, 150);
  delay(125); 
  tone(piezo_pin, 2500, 100);
}

unsigned long convertCode(char key_val) {
  switch (key_val) {
    case '1':
      ir_val = code_1;
      break;
    case '0':
      ir_val = code_0;
      break;
    case '2':
      ir_val = code_2;
      break;
    case 'A':
      ir_val = code_pwr;
      break;
    case 'B':
      ir_val = code_func;
  }
}

void RGB_color(int red_light_value, int green_light_value, int blue_light_value)
 {
  analogWrite(red_led_pin, red_light_value);
  analogWrite(green_led_pin, green_light_value);
  analogWrite(blue_led_pin, blue_light_value);
}

//Need to instantiate lcd(s) before using drawScrxxx
void drawScrWelcome(LiquidCrystal lcd1, LiquidCrystal lcd2) {
  //Blink LED
  if (led_blinker % 2 == 0)//On
    RGB_color(0, 255, 0);
  else//Off
    RGB_color(0, 0, 0);
  lcd1.clear();
  lcd2.clear();
  lcd1.print("Air Quality Monitor");
  lcd1.setCursor(22, 0);
  lcd1.print("Options:");
  lcd1.setCursor(0, 1);
  lcd1.print("0: Air Standards");
  lcd2.print("1: Real-time Data");
  lcd2.setCursor(0, 1);
  lcd2.print("2: Real-time Haz-Data");
  lcd1.setCursor(22 , 1);
  lcd1.print("4: TBD");
  lcd2.setCursor(22, 0);
  lcd2.print("Func: Breathalyzer");
  lcd2.setCursor(22, 1);
  lcd2.print("PWR: Home");
  led_blinker++;
}


//Displays the indoor standards for air quality, the highest you want these readings
//Temp is arbitrary 
void drawScrStandards(LiquidCrystal lcd1, LiquidCrystal lcd2) {
  RGB_color(0, 0, 255);
  lcd1.clear();
  lcd2.clear();
  lcd1.print("Temperature: 80");
  lcd1.setCursor(17, 0);
  lcd1.print("F");
  lcd1.setCursor(23, 0);
  lcd1.print("eCO2: 1000");
  lcd1.setCursor(34, 0);
  lcd1.print("ppm");
  lcd1.setCursor(0, 1);
  lcd1.print("PM2.5: 12");
  lcd1.setCursor(23, 1);
  lcd1.print("PM10: 50");
  lcd2.print("Humidity: 50");
  lcd2.setCursor(15, 0);
  lcd2.print("%");
  lcd2.setCursor(23, 0);
  lcd2.print("TVOC: 500");
  lcd2.setCursor(34, 0);
  lcd2.print("ppb");
  lcd2.setCursor(0, 1);
  lcd2.print("Air Quality: Moderate");
  lcd2.setCursor(23, 1);
  lcd2.print("PM2.5 Index: 49");
}

int aqcount = 0;
//Need to instantiate lcd(s) before using 
void drawScr1(LiquidCrystal lcd1, LiquidCrystal lcd2, String temp, String hum, String co2, String pm25,
                 String pm10, String tvoc, String aq, String index) {
  //Set LED quality indicator
  if (aq == "Good")
    RGB_color(0, 255, 0);
  else if (aq == "Moderate")  
    RGB_color(255, 200, 0);
  else if (aq == "Poor") {  
    RGB_color(255, 0, 0);
    if (aqcount%2 == 0)//On
      RGB_color(255, 0, 0);  
    else//Off
      RGB_color(0, 0, 0);
      ++aqcount;
  }
  lcd1.clear();
  lcd2.clear();
  lcd1.print("Temperature: " + temp);
  lcd1.setCursor(19, 0);
  lcd1.print("F");
  lcd1.setCursor(23, 0);
  lcd1.print("eCO2: " + co2);
  lcd1.setCursor(34, 0);
  lcd1.print("ppm");
  lcd1.setCursor(0, 1);
  lcd1.print("PM2.5: " + pm25);
  lcd1.setCursor(23, 1);
  lcd1.print("PM10: " + pm10);
  lcd2.print("Humidity: " + hum);
  lcd2.setCursor(15, 0);
  lcd2.print("%");
  lcd2.setCursor(23, 0);
  lcd2.print("TVOC: " + tvoc);
  lcd2.setCursor(34, 0);
  lcd2.print("ppb");
  lcd2.setCursor(0, 1);
  lcd2.print("Air Quality: " + aq);
  lcd2.setCursor(23, 1);
  lcd2.print("PM2.5 Index: " + index);
}

//Need to instantiate lcd(s) before using 
//Bad stuff, set up buzzer alarm
void drawScr2(LiquidCrystal lcd1, LiquidCrystal lcd2, float smoke, float co,
                float gas) {
  if (smoke > 0.0 || gas > 0.0)
    RGB_color(255, 0, 0);
  else
    RGB_color(0, 255, 0);
  lcd1.clear();
  lcd2.clear();
  lcd1.print("Smoke: " + String(smoke) + " ppm");
  lcd1.setCursor(20, 0);
  lcd1.print("CO: " + String(co) + " ppm");
  lcd1.setCursor(0, 1);
  lcd1.print("Gas: " + String(gas) + " ppm");
}

int alccount = 0;
//Alcohol Readings/Breathalyzer
void drawBreathalyzer(LiquidCrystal lcd1, LiquidCrystal lcd2) {
  alc = readAlcohol(MQ3_Analog_Input);
  alc_level = getAlcLev(alc); 
  //Set LED quality indicator
  if (alc_level == "Oh you drunk")
    RGB_color(0, 255, 0);//Green
  else if (alc_level == "You're getting there...")  
    RGB_color(255, 200, 0);//Yellow
  else if (alc_level == "You are sober :(")  
    RGB_color(255, 0, 0);//Red
  else if (alc_level == "A few beers")
    RGB_color(255, 50, 0);//Orange
  else {
    if (alccount%2 == 0)//On
      RGB_color(255, 0, 0);  
    else//Off
      RGB_color(0, 0, 0);
      ++alccount;
  }
  lcd1.clear();
  lcd2.clear();
  lcd1.print("Breathalyzer: Blow into red sensor...");
  lcd2.print("Alcohol reading: " + String(alc));
  lcd2.setCursor(0, 1);
  lcd2.print(alc_level);
}

void drawErr(LiquidCrystal lcd1, LiquidCrystal lcd2) {
  RGB_color(255, 0, 0);
  lcd1.clear();
  lcd2.clear();
  lcd.print("Didn't get whole IR signal...");
  lcd2.print("Make sure there is a line of sight.");
  lcd2.setCursor(0, 1);
  lcd2.print("Hit button again...");
}


void alarmCheck(int qual, int last_qual) {
  //Sound buzzer alarm if quality changes
  int diff = qual - last_qual;
  if (diff > 0) {
    //Beep according to how many levels quality jumped (got worse)
   for (int i = diff; i > 0; --i)
      tone(piezo_pin, 500, 750);//Long, low pitch = quality worsened
      delay(200);
  }
  else if (diff < 0) {
   for (int i = abs(diff); i > 0; --i)
      tone(piezo_pin, 2500, 750);//Quick, high pitch tone = quality improved
      delay(400);
  }
}


void setDelay(int co2, int qual) {
  if (co2 < 400 && quality == 0)
    DELAY = 750;
  else
    DELAY = 15000;
}

void calcData(float pm25) {
  index = (pm25/pm2_5standard) * 100.0;
  disp_index = int(index);
  if (index >=0 && index < 50.0)
    qual = "Good";
  else if (index >= 50.0 && index < 150.0)
    qual = "Moderate";
  else if (index >= 150.0 && index < 2000)
    qual = "Poor";
  else
    qual = "Oh u dead";
}

//Excludes alcohol sensor
void readAllSensors() {
  temp = (Si7021.readTemperature() * 1.8) + 32;
  hum = Si7021.readHumidity();
  if (ccs.available()) {
     ccs.readData();
     co2 = ccs.geteCO2();
     tvoc = ccs.getTVOC();
  }
  hpma115S0.ReadParticleMeasurement(&pm2_5, &pm10);
  smoke = mq2.readSmoke();
  co = mq7.getPPM();
  //gas = mq2.readLPG();
}

//Taken from Nick Koumaris, slightly modified
int readAlcohol(int analogPin) {
  int val = 0;
  int val1;
  int val2;
  int val3; 
  val1 = analogRead(analogPin); 
  delay(10);
  val2 = analogRead(analogPin); 
  delay(10);
  val3 = analogRead(analogPin);
  
  val = (val1+val2+val3)/3;
  return val;
}
 
String getAlcLev(int value) {
  if(value<200)
  {
     return ("You are sober.");
  }
  if (value>=200 && value<280)
  {
     return ("You had a beer.");
  }
  if (value>=280 && value<350)
  {
     return ("A few beers");
  }
  if (value>=350 && value <450)
  {
     return ("You're getting there...");
  }
  if(value>450)
  {
     return ("Oh you drunk");
  }
}

void setup() {
  pinMode(piezo_pin, OUTPUT);
  pinMode(red_led_pin, OUTPUT);
  pinMode(green_led_pin, OUTPUT);
  pinMode(blue_led_pin, OUTPUT);
  playStartTone();
  RGB_color(255, 255, 255);//white
  //keypad.addEventListener(keypadEvent);
  //Initialize sensors & LCD
  Serial.begin(9600);
  irrecv.enableIRIn(); 
  hpmaSerial.begin(9600);
  delay(3000);
  while (!Serial)
    delay(10);
  ccs.begin();
  Si7021.begin();
  hpma115S0.Init();
  hpma115S0.StartParticleMeasurement();
  mq2.begin();
  lcd.begin(16, 2);
  lcd2.begin(16, 2);
  RGB_color(0, 0, 0);
}



void loop() {
  //Check for IR signal
  if (irrecv.decode(&results))
    ir_val = results.value;
  key = keypad.getKey();
  if (key != prev_key)
    convertCode(key);
  //Determine LCD data based on IR code
  switch (ir_val) {
    case welcome:
      drawScrWelcome(lcd, lcd2);
      break;
    case code_pwr:
      drawScrWelcome(lcd, lcd2);
      break;
    case code_0:
      drawScrStandards(lcd, lcd2);
      break;
    case code_1:
      drawScr1(lcd, lcd2, String(temp), String(hum), String(co2), String(pm2_5),
             String(pm10), String(tvoc), String(qual), String(disp_index));
      break;
    case code_2:
      drawScr2(lcd, lcd2, smoke, co, gas);
      break;
    case code_func://Only want to read mq3 when on this screen
      drawBreathalyzer(lcd, lcd2);
      break;
    default://No IR, keep current screen
      ir_val = ir_val;
      drawErr(lcd, lcd2);
      break;
  }
  irrecv.resume();
  //Always update data
  readAllSensors();
  float pm25 = float(pm2_5);
  calcData(pm25);
  alarmCheck(quality, prev_qual);
  prev_qual = quality;
  prev_ir_val = ir_val;
  prev_key = key;
  delay(100);//Changed for modifications
}
