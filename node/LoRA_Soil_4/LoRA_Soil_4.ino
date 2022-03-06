#include <SPI.h>
#include <LoRa.h>
#include "Adafruit_seesaw.h"

Adafruit_seesaw ss;
int counter = 0;
int incSecs = 10;

#include <RTCZero.h>

/* Create an rtc object */
RTCZero rtc;

// Container for LoRaData String & outgoing packet
String LoRaData;
String packet;

void setup() {
  Serial.begin(115200);
  //while (!Serial);
  pinMode(LED_BUILTIN, OUTPUT);

  rtc.begin(); // initialize RTC 24H format
  String initialSet = "xxxx000000000000";
  updateTime(initialSet); // Initialize RTC on boot to 00:00:00. Ensures no odd issus with failiure to update.
  
  Serial.println("LoRa Startup");
  if (!LoRa.begin(868E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  
  doTimeUpdate(); // Update RTC
  setupSoilSensor(); // Initialize soil sensor

  // Set up desired alarm time for first increment based on current RTC
  packet = "";
  Serial.println("Setting alarm");
  alarmInc(incSecs);

  // Set Alarm time 
  rtc.enableAlarm(rtc.MATCH_HHMMSS);
  Serial.println("Alarm set");

  // Attach interrupt to taking sensor reading
  rtc.attachInterrupt(takeAnalogReading);
}

void loop() {
  // Do long blink for activity indicator
  blink(1, 100);
  delay(1000);
  //Serial.println(".");
}

void sendMessage(String outgoing) {
  LoRa.beginPacket();                   // start packet
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
}

void updateTime(String message) {
  //Serial.println(message);
  String data = message.substring(5); // why can't I find 't' here?
  //Serial.println(data);
  byte day = data.substring(0,2).toInt();
  byte month = data.substring(2,4).toInt();
  byte year = data.substring(4,6).toInt();
  byte hours = data.substring(6,8).toInt();
  byte minutes =data.substring(8,10).toInt();
  byte seconds = data.substring(10,12).toInt();
  rtc.setTime(hours, minutes, seconds);
  rtc.setDate(day, month, year);
  Serial.print("Set date & time to: "); 
  Serial.print(rtc.getDay()); Serial.print("/");
  Serial.print(rtc.getMonth()); Serial.print("/");
  Serial.print(rtc.getYear()); Serial.print(" ");
  Serial.print(rtc.getHours()); Serial.print(":"); 
  Serial.print(rtc.getMinutes()); Serial.print(":"); 
  Serial.println(rtc.getSeconds());
}

String print2digits(int number) {
  if (number < 10) {
    String result = "0"+ String(number);
    return result;
  }
  else {
    return String(number);
  }
}

void onReceive(int packetSize) {
  
  // received a packet
  Serial.print("Received packet '");

  // read packet
  LoRaData = LoRa.readString();

  // print RSSI of packet
  Serial.print(LoRaData);
  Serial.print("' with RSSI ");
  Serial.println(LoRa.packetRssi());
}

void blink(int count, int pause) {
  for(int i=0; i< count; i++){
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(pause);                       // wait for a second
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    delay(pause); 
  }
}

void alarmInc(int inc){
  int secs; int mins; int hours;

  secs = rtc.getSeconds();
  mins = rtc.getMinutes();
  hours = rtc.getHours();
  rtc.setAlarmTime(hours, mins, secs);
  Serial.print("Interrupt at time: "); 
  Serial.print(hours); Serial.print(".");
  Serial.print(mins); Serial.print(".");
  Serial.println(secs);

  if (rtc.getSeconds()+ inc >= 60){
    Serial.println("increment minute/hr");
    secs = rtc.getSeconds() + (inc-60);
    if (rtc.getMinutes()+1 >= 60){
      mins = rtc.getMinutes()+(1-60);
      if (rtc.getHours()+1 >= 24){
        hours = rtc.getHours()+(1-24);
      }
      else{
        hours = rtc.getHours()+1;
      }
    }
    else{
      mins = rtc.getMinutes()+1;
    }
  }
  else{
    secs = rtc.getSeconds() + inc;
    mins = rtc.getMinutes();
    hours = rtc.getHours();
  }
  rtc.setAlarmTime(hours, mins, secs);
  Serial.print("Set alarm to: "); 
  Serial.print(hours); Serial.print(".");
  Serial.print(mins); Serial.print(".");
  Serial.println(secs);
}

void takeAnalogReading(){
    float tempC = ss.getTemp();
    uint16_t capread = ss.touchRead(0);

    //Serial.print("Temperature: "); Serial.print(tempC); Serial.println("*C");
    //Serial.print("Capacitive: "); Serial.println(capread);

    String timestamp =print2digits(rtc.getHours())+print2digits(rtc.getMinutes())+print2digits(rtc.getSeconds());
    String header = "p/";
    String packet = header+tempC+"/"+capread+"/"+timestamp;

    // Serial confirmation
    Serial.print("Sending packet: "); Serial.print(packet); Serial.print(" as number: "); Serial.println(counter);
    
    // send packet
    sendMessage(packet);
    counter++;
    alarmInc(incSecs);
    blink(3,100);
    //Serial.println("Looping");
}

void doTimeUpdate(){
    while (1) {
    //Request time packet
    Serial.println("Requesting time update");
    packet = "update";
    sendMessage(packet);
    
    LoRa.onReceive(onReceive);
    LoRa.receive();
    Serial.println(LoRaData);

    if(LoRaData != "" ){
        updateTime(LoRaData);
        blink(5,50);
        break;
    }
    
    // Loop after 1 second if not recieved
    delay(1000);
    blink(2,1000);
  }
}

void setupSoilSensor(){
    //Serial.println("seesaw Soil Sensor example!");
  if (!ss.begin(0x36)) {
    Serial.println("ERROR! seesaw not found");
    while(1) delay(1);
  } else {
    Serial.print("seesaw started! version: ");
    Serial.println(ss.getVersion(), HEX);
  }
}
