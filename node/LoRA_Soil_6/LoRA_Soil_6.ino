#include <SPI.h>
#include <RTCZero.h>
#include <LoRa.h>
#include "Adafruit_seesaw.h"
#include <string.h>

Adafruit_seesaw ss;

#define CIRCULAR_BUFFER_INT_SAFE
#include <CircularBuffer.h>

/* Create an rtc object */
RTCZero rtc;

namespace data {
  typedef struct {
    unsigned long timeStamp;
    unsigned int mois;
    float temp;
  } record;

  void print(record r) {
    Serial.print(r.timeStamp);
    Serial.print("  ");
    Serial.print(r.mois);
    Serial.print("  ");
    Serial.print(r.temp);
  }
}

CircularBuffer<data::record, 10> structs;

#define SAMPLE_PIN A0
struct sensorReading{
  unsigned long timeStamp;
  unsigned int mois;
  float temp;
  } record;
String LoRaData;
int incSecs = 10; // reading timer increment
int wait = 10;    // wait time for array prints
unsigned long startTime;    // start of wait timer
unsigned long elapsedTime;  // trigger for wait timer
sensorReading sensorReading0 = { 0, 0, 0};
sensorReading stored0 = { 0, 0, 0};

void setup()
{
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  //while (!Serial);
  
  rtc.begin(); // initialize RTC 24H format
  byte timeSet[] = { 0 , 0 , 0 , 0 , 0 , 0 };
  updateTime(timeSet[0], timeSet[1], timeSet[2], timeSet[3], timeSet[4], timeSet[5]); // Initialize RTC on boot to 00:00:00. Ensures no odd issues with failiure to update.

  Serial.println("LoRa Startup"); // Start LoRA module
  if (!LoRa.begin(868E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  setupSoilSensor();
  
  doTimeUpdate();
  

  // Set up desired alarm time for first increment based on current RTC
  Serial.println("Setting alarm");
  alarmInc(incSecs); // Sets Alarm for x secs incrmment from now
  // Set Alarm time
  rtc.enableAlarm(rtc.MATCH_HHMMSS);
  Serial.println("Alarm set");
  
  pinMode(SAMPLE_PIN, INPUT);
  Serial.println("STARTING UP");

  // Attach interrupt to taking sensor reading
  rtc.attachInterrupt(takeAnalogReading); //when interrupt is triggered, take sensor reading
 
  Serial.flush();
  LoRaData = ""; // make sure stored LoRa mesage is empty
  blink(5, 100); 
  startTime = millis();
  delay(500);
}

void loop()
{
  /*Serial.println(startTime);
  if (millis() > startTime + (incSecs*1000) ){
    takeAnalogReading();
  }*/
  
   // unsigned int sample = analogRead(SAMPLE_PIN);
  if (sensorReading0.timeStamp != 0 ) {
    Serial.print("Sending: "); Serial.println(sensorReading0.timeStamp); 
    sendReading(sensorReading0.timeStamp, sensorReading0.mois, sensorReading0.temp);  // sends radio message to base unit for upload to DB
    LoRa.onReceive(onReceive); //onRecieve interrupt, listens for LoRa packets
    LoRa.receive(); // puts radio into recieve mode
    
    // Delay for confirmation (needs to be about >100ms to be safe)
    delay(250);
    if (recieveConfirmation(sensorReading0.timeStamp)) { // if correct confirmation is recieved
      Serial.print("Popping: "); Serial.println(sensorReading0.timeStamp); // confirm removal 
      sensorReading0 = { 0, 0, 0};
    }
    else {
      structs.push(data::record{sensorReading0.timeStamp, sensorReading0.mois, sensorReading0.temp});
      Serial.println("Saving record in buffer");
      sensorReading0 = {0, 0, 0};
    }
    Serial.println("---");
    delay(50);
  }

  if (!structs.isEmpty() ){
    Serial.print("Sending: "); Serial.println(structs.first().timeStamp); 
    sendReading(structs.first().timeStamp, structs.first().mois, structs.first().temp);  // sends radio message to base unit for upload to DB
    delay(20); // Pause briefly to allow hub unit to respond and not recieve old response
    LoRa.onReceive(onReceive); //onRecieve interrupt, listens for LoRa packets
    LoRa.receive(); // puts radio into recieve mode
    
    // Delay for confirmation (needs to be about >100ms to be safe)
    delay(250);

    if (recieveConfirmation(structs.first().timeStamp)) { // if correct confirmation is recieved
      Serial.print("Confirmed. Shifting: ");
      data::print(structs.shift());
      Serial.println();
      }
    }
 
  
  if (structs.isFull()) {
    Serial.println("Stack is full:");
    while (!structs.isEmpty()) {
      data::print(structs.shift());
      Serial.println();
    }
    Serial.println("START AGAIN");
  }
  
  blink(2,500);
}

void takeAnalogReading() { // takes sensor temp and capacitive moisure values.
  
  float tempC = ss.getTemp();
  uint16_t capread = ss.touchRead(0);
  
  //Serial.print("Temperature: "); Serial.print(tempC); Serial.println("*C");
  //Serial.print("Capacitive: "); Serial.println(capread);
  
  //String timestamp = print2digits(rtc.getHours()) + print2digits(rtc.getMinutes()) + print2digits(rtc.getSeconds()); // gets timestamp from rtc
  unsigned long timestamp = (rtc.getHours() * 10000) + (rtc.getMinutes() * 100) + (rtc.getSeconds() );
  //Serial.println(timestamp);
  
  sensorReading0.temp = tempC;
  sensorReading0.mois = capread;
  sensorReading0.timeStamp = timestamp;

  Serial.print("Taken reading at ");
  Serial.println(sensorReading0.timeStamp);
  alarmInc(incSecs); // increment next alarm by x seconds
  blink(2, 250); // blink for confirmation
}

// updates unit RTC based on LoRa time handshake being recieved
void updateTime(byte day, byte month, byte year, byte hours, byte minutes, byte seconds) { 
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

void sendUpdate() {   // sends passed string as LoRa message
  String outgoing = "update";
  LoRa.beginPacket();                   // start packet
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  Serial.print("Sent: "); Serial.println(outgoing);
}

void sendReading(unsigned long timeStamp, unsigned int mois, float temp) {   // sends passed string as LoRa message
  String outgoing;
  if (timeStamp < 100000){
    outgoing = "p/" + String(temp) + "/" + String(mois) + "/0" + String(timeStamp); // creates message string
  }
  else {
    outgoing = "p/" + String(temp) + "/" + String(mois) + "/" + String(timeStamp); // creates message string
  }
  LoRa.beginPacket();                   // start packet
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  Serial.print("Sent: "); Serial.println(outgoing);
}

// Function that gets current RTC clock time, and sets alarm for inc seconds in future
void alarmInc(int inc) { 
  int secs; int mins; int hours;

  secs = rtc.getSeconds();
  mins = rtc.getMinutes();
  hours = rtc.getHours();
  rtc.setAlarmTime(hours, mins, secs);

  if (rtc.getSeconds() + inc >= 60) {
    //Serial.println("increment minute/hr");
    secs = rtc.getSeconds() + (inc - 60);
    if (rtc.getMinutes() + 1 >= 60) {
      mins = rtc.getMinutes() + (1 - 60);
      if (rtc.getHours() + 1 >= 24) {
        hours = rtc.getHours() + (1 - 24);
      }
      else {hours = rtc.getHours() + 1;}
    }
    else {mins = rtc.getMinutes() + 1;}
  }
  else {
    secs = rtc.getSeconds() + inc;
    mins = rtc.getMinutes();
    hours = rtc.getHours();
  }
  rtc.setAlarmTime(hours, mins, secs);
}

void doTimeUpdate() {
  while (1) {
    //Request time packet
    Serial.println("Requesting time update");
    sendUpdate();

    LoRa.onReceive(onReceive);
    LoRa.receive();
    Serial.println(LoRaData);

    String data = LoRaData.substring(5); 
    Serial.println(data);
    byte day = data.substring(0, 2).toInt();
    byte month = data.substring(2, 4).toInt();
    byte year = data.substring(4, 6).toInt();
    byte hours = data.substring(6, 8).toInt();
    byte minutes = data.substring(8, 10).toInt();
    byte seconds = data.substring(10, 12).toInt();

    if (LoRaData != "" ) {
      updateTime(day, month, year, hours, minutes, seconds);
      blink(5, 50);
      break;
    }

    // Loop if not recieved
    blink(2, 1000);
    delay(500);
  }
}

bool recieveConfirmation(unsigned long timestamp) {
  int recTime = LoRaData.substring(4, 10).toInt(); // cut recieved packet to timestamp only, as Int (first 3 chars are identifier for messsage type)
  if (recTime == timestamp) { // compare message timestamp to confirmation timestamp, return true if match
      return true;
  }
  else {
    return false;
  }
}

void onReceive(int packetSize) { // recieves LoRa packet and stores data in global variable LoRaData

  // received a packet
  Serial.print("Received packet '");

  // read packet
  LoRaData = LoRa.readString();

  // print RSSI of packet
  Serial.print(LoRaData);
  Serial.print("' with RSSI ");
  Serial.println(LoRa.packetRssi());
}

void setupSoilSensor() {
  //Serial.println("seesaw Soil Sensor example!");
  if (!ss.begin(0x36)) {
    Serial.println("ERROR! seesaw not found");
    while (1) delay(1);
  } else {
    Serial.print("seesaw started! version: ");
    Serial.println(ss.getVersion(), HEX);
  }
}

void print2digits(int number) {
  if (number < 10) {
    Serial.print("0"); // print a 0 before if the number is < than 10
  }
  Serial.print(number);
}

void blink(int count, int pause) {
  for (int i = 0; i < count; i++) {
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(pause);                       // wait for a second
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    delay(pause);
  }
}
