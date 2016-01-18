#include <OneWire.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

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
OneWire  ds(12); //different in new board version, new=13
int dscount = 0;
int loopcount = 0;
int i=0;
float f[4]; // 4 1-wire sensors total
boolean hbState;
long lastConnectionTime = 0;

void setup() {
  uart.begin(9600);
  Serial.begin(9600);
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
  delay(500); 
  display.invertDisplay(false);
  delay(500);
  
  // display some text
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("In Control!");
  display.display();

  ledWrite(1, 1, 1);

  // find the 1-Wire devices
  byte addr[8];
  while ( ds.search(addr)) {
    dscount += 1;
    delay(250);
  }
  ds.reset_search();
  float f[dscount]; //recast to the # of sensors
  Serial.println(dscount);
  ds.reset_search();
  display.clearDisplay();
  display.print("Found = ");
  display.println(dscount);
  display.display();
  delay(1000);
}

void loop() {
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;
  display.setTextSize(2);
  
  if ( !ds.search(addr) ) {
    //Serial.println("No more addresses.");
    ds.reset_search();
    loopcount = 0;
    delay(250);
    return;
  } else {
    loopcount += 1;
  }
  Serial.print("loopcount= ");
  Serial.println(loopcount);
  
  Serial.print("ROM =");
  for( i = 0; i < 8; i++) {
    Serial.write(' ');
    Serial.print(addr[i], HEX);
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  ledWrite(0, 1, 0);
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  ledWrite(0, 0, 0);
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  Serial.print("  Data = ");
  Serial.print(present, HEX);
  Serial.print(" ");
  for ( i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds.read();
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  /*Serial.print(" CRC=");
  Serial.print(OneWire::crc8(data, 8), HEX);
  Serial.println();*/

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    // default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  fahrenheit = celsius * 1.8 + 32.0;
  f[loopcount] = fahrenheit;
  Serial.print("  Temp = ");
  Serial.print(celsius);
  Serial.print(" C, ");
  Serial.print(fahrenheit);
  Serial.println(" F");
  
  // display temps
  if ( loopcount == dscount) {
    display.clearDisplay();
    display.setCursor(0,0);
    for (int j=1; j<dscount+1; j+=1) {
      display.print(f[j]);
      display.println(" F");
    }
    /*display.print("ROM =");
    for( i = 0; i < 8; i++) {
      display.write(' ');
      display.print(addr[i], HEX);
    }*/
    display.display();
  }

  if(needUpdate() == 1) {
    doUpdate();
  }
  //Teensy heartbeat:
  hbState = !hbState;
  digitalWrite(11, hbState);
}

// Write the rgb led
void ledWrite(int r, int g, int b) {
  analogWrite(RED, r);
  analogWrite(GREEN, g);
  analogWrite(BLUE, b);
}

// Wakeup the WiFly
void wakeUp(){
  long wakeTime=millis();
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
void doUpdate() {
  ledWrite(0, 0, 2);  
  delay(1000);
    /*Serial.println("updating...");
    wakeUp();
    while (uart.available()) Serial.print(char(uart.read()));
    Serial.println();
    T1old=T1A;
    T2old=T2A;
    
    dtostrf(T1A,0,2,f1); 
    dtostrf(T2A,0,2,f2);
    dtostrf(valve1A,0,2,f3);
    
    //"http://api.thingspeak.com/update?key=PK4E465VG92479M3&field1=1&field2=1..."
    String WiStr="GET /update?key=PK4E465VG92479M3&field1="+String(f1)+"&field2="+String(f2)+"&field3="+String(f3);
    uart.println(WiStr);
    uart.println();
    delay(1000);
  
    Serial.print("WiStr= ");
    Serial.println(WiStr);
    //delay(1000);
    while (uart.available()) Serial.print(char(uart.read()));//x=uart.read();
    Serial.println();*/
  ledWrite(0, 0, 0);
}

