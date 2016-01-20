// 1/17/16 JP

#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define ONE_WIRE_BUS 12 //different in new board version, new=13
#define OLED_RESET 9
#define RED 15
#define GREEN 14
#define BLUE 10 //different in new board version, new=12
//#define CON 3  //wrong pin, is this used?
#define WAKE 0
#define SSR1 1
#define SSR2 2
#define SSR3 3
#define SSR4 4

HardwareSerial uart = HardwareSerial();
Adafruit_SSD1306 display(OLED_RESET);
OneWire ds(ONE_WIRE_BUS); 
DallasTemperature sensors(&ds); 

uint8_t dscount = 0;
uint8_t loopcount = 0;
int i=0;
boolean hbState;
long lastConnectionTime = 0;

// arrays to hold 1-W device addresses
DeviceAddress insideT1, outsideT1, insideT2, outsideT2;

void setup() {
  uart.begin(9600);
  Serial.begin(9600);
  sensors.begin();
  
  pinMode(WAKE, OUTPUT); //WiFly wakepin
  //pinMode(CON, INPUT_PULLUP); //WiFly connectedPin, not used yet
  pinMode(SSR1, OUTPUT);
  pinMode(SSR2, OUTPUT);
  pinMode(SSR3, OUTPUT);
  pinMode(SSR4, OUTPUT);
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3D);
  display.display();
  delay(1000);
  display.invertDisplay(true);
  delay(300); 
  display.invertDisplay(false);
  delay(300);
  
  // display some text
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("In Control!");
  display.display();

  ledWrite(1, 1, 1);

  // locate devices on the bus
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");
  
  // report parasite power requirements
  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");

  // assigns the first address found to insideThermometer
  ds.reset_search();
  if (!ds.search(insideT1)) Serial.println("Unable to find address for insideT1");
  if (!ds.search(outsideT1)) Serial.println("Unable to find address for outsideT1");
  if (!ds.search(insideT2)) Serial.println("Unable to find address for insideT2");
  if (!ds.search(outsideT2)) Serial.println("Unable to find address for outsideT2");
}

void loop() {
  //request temps
  //Serial.print("Requesting temperatures...");
  sensors.requestTemperatures();
  //Serial.println("DONE");
  float iT1, oT1, iT2, oT2;
  iT1 = sensors.getTempF(insideT1);
  oT1 = sensors.getTempF(outsideT1);
  iT2 = sensors.getTempF(insideT2);
  oT2 = sensors.getTempF(outsideT2);
  
  // display temps
  display.setTextSize(2);
  display.clearDisplay();
  display.setCursor(0,0);
  display.print("iT1=");
  display.println(iT1);
  display.setCursor(0,16);
  display.print("oT1=");
  display.println(oT1);
  display.setCursor(0,32);
  display.print("iT2=");
  display.println(iT2);
  display.setCursor(0,48);
  display.print("oT2=");
  display.println(oT2);
  display.display();
  
  if(needUpdate() == 1) {
    doUpdate(iT1, oT1, iT2, oT2);
  }
  //Teensy heartbeat:
  hbState = !hbState;
  digitalWrite(11, hbState);
  ledWrite(0, hbState, 0);
}

// Write the rgb led
void ledWrite(int r, int g, int b) {
  analogWrite(RED, r);
  analogWrite(GREEN, g);
  analogWrite(BLUE, b);
}

// Wakeup the WiFly
void wakeUp(){
  //long wakeTime=millis();
  digitalWrite(WAKE, LOW);
  delay(50);
  digitalWrite(WAKE, HIGH);
  delay(350);
  //digitalWrite(connectPin,HIGH);
  /*while (millis()<wakeTime+2000){
    int con = digitalRead(CON);
    if(con==1){
      i=1;
      return; 
    }
  }*/
  //took too long to connect...
  i++;
}

// Timer for WiFly wakeup
int needUpdate() {
  if(millis()-lastConnectionTime > 59500){ //TODO: move time to top
    //changeSettings();
    lastConnectionTime = millis();
    return 1;    
  }
  else{
    return 0;
  }
}

// Change WiFly settings
void changeSettings(){
  uart.print("$$$\n");
  Serial.print(uart.read());
  uart.print("set sys autoconn 0\n");
  Serial.print(uart.read());
  uart.print("save\n");
  Serial.print(uart.read());
  uart.print("reboot");
}

// Use WiFly to update cloud...
void doUpdate(float iT1, float oT1, float iT2, float oT2) {
  ledWrite(0, 0, 2);  
  delay(1000);
    Serial.println("updating...");
    wakeUp();
    while (uart.available()) Serial.print(char(uart.read()));
    Serial.println();

    int f3 = 0;
    
    //"http://api.thingspeak.com/update?key=PK4E465VG92479M3&field1=1&field2=1..."
    String WiStr="GET /update?key=PK4E465VG92479M3&field1="+String(iT1)+"&field2="+String(oT1)+"&field3="+String(f3);
    uart.println(WiStr);
    uart.println();
    delay(1000);
  
    Serial.print("WiStr= ");
    Serial.println(WiStr);
    //delay(1000);
    while (uart.available()) Serial.print(char(uart.read()));//x=uart.read();
    Serial.println();
  ledWrite(0, 0, 0);
}

