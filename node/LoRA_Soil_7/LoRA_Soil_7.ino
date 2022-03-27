#include <RTCZero.h>
#include <CircularBuffer.h>
#include <LoRa.h>
//#include <stdlib.h>
#include "Adafruit_seesaw.h"
//#include <SPI.h>

// Watchdog library that reboots unit after certain period of inactivity
#include <Adafruit_SleepyDog.h>

Adafruit_seesaw ss;

/* Create an rtc object */
RTCZero rtc;

namespace data {
  typedef struct {
    int timeStamp;
    int mois;
    float temp;
  } record;

  void print(record r) {
    Serial.print(r.timeStamp);
    Serial.print(" / ");
    Serial.print(r.mois);
    Serial.print(" / ");
    Serial.print(r.temp);
  }
}

//#define CIRCULAR_BUFFER_INT_SAFE
const int bufLen = 100;
CircularBuffer<data::record, bufLen> structs;

struct sensorReading{
  unsigned long timeStamp;
  unsigned int mois;
  float temp;
  } record;
  
sensorReading sensorReading0 = { 0, 0, 0};

int incSecs = 10; // reading timer increment
unsigned long elapsedTime = millis();  // trigger for wait timer
unsigned long alarmElapse = millis();

char cUpdate[7] = "update";
const int msgBuf = 50; //size of message buffer
char cMessage[msgBuf];
char LoRaData[251];
bool readData = true;

// Set watchdog timer
int countdownMS = Watchdog.enable(10000);

void setup()
{
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  
  if (!Serial){
    delay(2000);
  }
  
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
  
  // Attach interrupt to taking sensor reading
  rtc.attachInterrupt(updateDataFlag); //when interrupt is triggered, update reading flag
 
  Serial.flush();
  blink(5, 100); 
  delay(500);
  Serial.println("STARTING UP"); 
}

void loop()
{
  if (readData == true){
    //Serial.println("Confirm read data");
    takeAnalogReading();
    //Serial.println("Set read Data flag = false");
    readData = false;
  }
  
  
  if (sensorReading0.timeStamp != 0 ) {
    sendMessage(true, sensorReading0.timeStamp, sensorReading0.mois, sensorReading0.temp);  // sends radio message to base unit for upload to DB
    LoRa.onReceive(onReceive); //onRecieve interrupt, listens for LoRa packets
    LoRa.receive(); // puts radio into recieve mode
    
    // Delay to allow for confirmation before moving on
    delay(300);
    
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

  // Checks for entries in buffer. If buffer not empty, tries to send messages and clear buffer
  if (!structs.isEmpty() ){
    Serial.print("Sending: "); Serial.println(structs.first().timeStamp); 
    sendMessage(true, structs.first().timeStamp, structs.first().mois, structs.first().temp);  // sends radio message to base unit for upload to DB / true is flag for sensor reading
    delay(20); // Pause briefly to allow hub unit to respond and not recieve old response
    LoRa.onReceive(onReceive); //onRecieve interrupt, listens for LoRa packets for confirmation
    LoRa.receive(); // puts radio into recieve mode
    
    // Delay to allow for confirmation before moving on
    delay(250);

    // If confirmation recieved, remove entry from buffer
    if (recieveConfirmation(structs.first().timeStamp)) { // if correct confirmation is recieved
      Serial.print("Confirmed. Shifting: ");
      data::print(structs.shift());
      Serial.println();
      }
    }

    /*
   // Placeholder function to take reading on timer //
   if(millis() > alarmElapse + 10000){
      takeAnalogReading();
      alarmElapse = millis();
   }*/

  
   // If buffer has entries, print out to Serial for monitoring
   if(millis() > elapsedTime + 30000){
      if (!structs.isEmpty() ){
        Serial.print("Records in Buffer. Total records: ");
        Serial.println(structs.size());
        Serial.print("Last Entry:");
        data::print(structs.last());
        Serial.println();
        elapsedTime = millis();
      }
   }

  // Should never be needed, if buffer fills up, remove all entries 
  if (structs.isFull()) {
    Serial.println("BUFFER FULL. CLEARING BUFFER");
    while (!structs.isEmpty()) {
      data::print(structs.shift());
      Serial.println();
    }
  }
  blink(1, 250);
  delay(500);

  //Reset watchdog timer at end of loop
  countdownMS = Watchdog.enable(4000);
}

void updateDataFlag() {
  readData = true;
}

// Interupt routine - takes sensor temp and capacitive moisure values in temporary storage, then increments alarm for next reading
void takeAnalogReading() { 
  alarmInc(incSecs); // increment next alarm by x seconds
  
  //Serial.println("takeAnalogReading");
  float tempC = ss.getTemp();
  uint16_t capread = ss.touchRead(0); 
  unsigned long timestamp = (rtc.getHours() * 10000) + (rtc.getMinutes() * 100) + (rtc.getSeconds() );
  
  sensorReading0.temp = tempC;
  sensorReading0.mois = capread;
  sensorReading0.timeStamp = timestamp;

  Serial.print("Taken reading at ");
  Serial.println(sensorReading0.timeStamp);
  blink(3, 100); // blink for confirmation
}

// updates unit RTC based on LoRa time handshake being recieved
void updateTime(byte day, byte month, byte year, byte hours, byte minutes, byte seconds) {
  //Serial.println("updateTime"); 
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

void sendMessage(bool type, int timeStamp, int mois, float temp) {   // sends passed string as LoRa message
  //Serial.println("sendMessage");
  if (type == true){
    int tempInt = temp*100; 

    // creates message string, adds leading 0 to timestamp int if needed for sending
    if (timeStamp < 100000){ 
      snprintf(cMessage,msgBuf,"p/%d/%d/0%d", tempInt, mois, timeStamp); 
    }
    else {
      snprintf(cMessage,msgBuf,"p/%d/%d/%d", tempInt, mois, timeStamp); 
    }
    LoRa.beginPacket();                   // start packet
    LoRa.print(cMessage);                 // add payload
    LoRa.endPacket();                     // finish packet and send it
    Serial.print("Sent: "); Serial.println(cMessage);
  }
  else {  // Send message for time update
    LoRa.beginPacket();                   // start packet
    LoRa.print(cUpdate);                 // add payload
    LoRa.endPacket();                     // finish packet and send it
    Serial.print("Sent: "); Serial.println(cUpdate); 
  }
}

// Function that gets current RTC clock time, and sets alarm for inc seconds in future
void alarmInc(int inc) { 
  //Serial.println("alarmInc");
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
    Serial.println("doTimeUpdate()");
    //Request time packet
    Serial.println("Requesting time update");
    sendMessage(false,0,0,0); // 'false' is flag to send update message signal

    LoRa.onReceive(onReceive);
    LoRa.receive();
    delay(500);
    Serial.println(LoRaData);
  
    char dateBuf[7];
      for (int i=5;i<11;i++){
        dateBuf[i-5] = LoRaData[i];
      }
    int dateInt = atoi(dateBuf);
    //Serial.println(dateInt);
  
    byte day =      (dateInt/10000) % 100;
    byte month =    (dateInt/100) % 100;
    byte year =     dateInt % 100;
  
    char timeBuf[7];
    for (int i=11;i<17;i++){
      timeBuf[i-11] = LoRaData[i];
    }
    int timeInt = atoi(timeBuf);
    //Serial.println(timeInt);
  
    byte hours =    (timeInt/10000) % 100;
    byte minutes =  (timeInt/100) % 100;
    byte seconds =  timeInt % 100;

    if (year != 0 ) {
      updateTime(day, month, year, hours, minutes, seconds);
      blink(5, 50);
      break;
    }

    // Loop if not recieved
    Serial.println("No response.");
    blink(2, 1000);
    delay(500);
  }
}

bool recieveConfirmation(unsigned long timestamp) {
  //Serial.println("recieveConfirmation");
  char charLora[6];
  for(int i=0;i<6;i++){
    charLora[i] = LoRaData[i+4];
  }
  int recTime = atoi(charLora);
  //Serial.print("recTime = "); Serial.println(recTime); 
  if (recTime == timestamp) { // compare message timestamp to confirmation timestamp, return true if match
      return true;
  }
  else {
    return false;
  }
}

void onReceive(int packetSize) {
  // received a packet
  Serial.print("Received packet '");

  // read packet
  for (int i = 0; i < packetSize; i++) {
    LoRaData[i] = ((char)LoRa.read());
    Serial.print(LoRaData[i]);
  }

  // print RSSI of packet
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

void blink(int count, int pause) {
  for (int i = 0; i < count; i++) {
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(pause);                       // wait for a second
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    delay(pause);
  }
}
