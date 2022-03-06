#include <SPI.h>
#include <LoRa.h>
#include <RingBuf.h>
#include "Adafruit_seesaw.h"
#include <RTCZero.h>

Adafruit_seesaw ss;

int incSecs = 10; // message timer increment

int wait = 10;    // wait time for array prints
unsigned long startTime;    // start of wait timer
unsigned long elapsedTime;  // trigger for wait timer

/* Create an rtc object */
RTCZero rtc;

// Container for LoRaData String & outgoing packet
RingBuf<String, 5> myBuffer; // create Ring Buffer for storing sensor values
String LoRaData; // String container for LoRa messages
String data; // String container for message data to be sent

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  rtc.begin(); // initialize RTC 24H format
  String initialSet = "xxxx000000000000";
  updateTime(initialSet); // Initialize RTC on boot to 00:00:00. Ensures no odd issues with failiure to update.

  Serial.println("LoRa Startup"); // Start LoRA module
  if (!LoRa.begin(868E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  doTimeUpdate(); // Update RTC over radio
  setupSoilSensor(); // Initialize soil sensor

  // Set up desired alarm time for first increment based on current RTC
  packet = "";
  Serial.println("Setting alarm");
  alarmInc(incSecs); // Sets Alarm for x secs incrmment from now

  // Set Alarm time
  rtc.enableAlarm(rtc.MATCH_HHMMSS);
  Serial.println("Alarm set");

  // Attach interrupt to taking sensor reading
  rtc.attachInterrupt(takeAnalogReading); //when interrupt is triggered, take sensor reading
  Serial.flush();
  LoRaData = ""; // make sure stored LoRa mesage is empty
  blink(5, 100); 
  startTime = millis();
  delay(1000); // wait for 1 second to ensure base station is ready
}

void loop() {

  if (!myBuffer.isEmpty()) {
    // Execute necessary function for sending data
    data = myBuffer[0];
    Serial.print("Sending: "); Serial.println(data); 
    sendMessage(data);  // sends radio message to base unit for upload to DB
    LoRa.onReceive(onReceive); //onRecieve interrupt, listens for LoRa packets
    Serial.println("Waiting to recieve");
    LoRa.receive(); // puts radio into recieve mode


    // Delay for confirmation (needs to be about >100ms to be safe)
    delay(100);
    if (recieveConfirmation(data)) { // if correct confirmation is recieved
      if (myBuffer.lockedPop(data)) { // remove message from array using blocking Pop function
        Serial.print("Popped: "); Serial.println(data); // confirm removal 
      }
    }
    else {
      delay(1000); // if no confirmation, wait 1 second before trying again
    }
  }

  //  Check elapsed time since last update, blink and print array
  elapsedTime = millis() - startTime;
  if (elapsedTime > (wait * 1000) ) {
    startTime = millis();
    blink(3, 150);
    if (!myBuffer.isEmpty()) {
      // loop & print buffer contents (if any) at regular interval
      Serial.println("");
      for (uint8_t j = 0; j < myBuffer.size(); j++) {
        Serial.print(myBuffer[j]);
        Serial.print("//");
      }
      Serial.println("");
      Serial.println("--------");
      Serial.flush();
    }
  }
}

bool recieveConfirmation(String data) {
  int recTime = LoRaData.substring(4, 10).toInt(); // cut recieved packet to timestamp only, as Int (first 3 chars are identifier for messsage type)
  int index = data.lastIndexOf('/'); // find index of last delimiter character
  int timestamp = data.substring(index + 1, index + 7).toInt(); // use index to identify timestamp
  //Serial.println(""); Serial.println(LoRaData); Serial.println(recTime); Serial.println(timestamp);
  if (recTime == timestamp) { // compare message timestamp to confirmation timestamp, return true if match
    return true;
  }
  else {
    return false;
  }
}

void sendMessage(String outgoing) {   // sends passed string as LoRa message
  LoRa.beginPacket();                   // start packet
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  Serial.print("Sent: "); Serial.println(outgoing);
}

void updateTime(String message) { // updates unit RTC based on LoRa time handshake being recieved
  //Serial.println(message);
  String data = message.substring(5); 
  //Serial.println(data);
  byte day = data.substring(0, 2).toInt();
  byte month = data.substring(2, 4).toInt();
  byte year = data.substring(4, 6).toInt();
  byte hours = data.substring(6, 8).toInt();
  byte minutes = data.substring(8, 10).toInt();
  byte seconds = data.substring(10, 12).toInt();
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

String print2digits(int number) { // Convert single digit numbers to 2 digits if <10, for time formatting
  if (number < 10) {
    String result = "0" + String(number);
    return result;
  }
  else {
    return String(number);
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
  Serial.print(LoRa.packetRssi());
  Serial.print(" // ");
}

void blink(int count, int pause) {
  for (int i = 0; i < count; i++) {
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(pause);                       // wait for a second
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    delay(pause);
  }
}

void alarmInc(int inc) { // Function that gets current RTC clock time, and sets alarm for inc seconds in future
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

void takeAnalogReading() { // takes sensor temp and capacitive moisure values.
  float tempC = ss.getTemp();
  uint16_t capread = ss.touchRead(0);

  //Serial.print("Temperature: "); Serial.print(tempC); Serial.println("*C");
  //Serial.print("Capacitive: "); Serial.println(capread);

  String timestamp = print2digits(rtc.getHours()) + print2digits(rtc.getMinutes()) + print2digits(rtc.getSeconds()); // gets timestamp from rtc
  String header = "p/";
  String packet = header + tempC + "/" + capread + "/" + timestamp; // creates message string

  // adds new message string to buffer
  if (! myBuffer.push(packet)) {
    // oops error, push failed because the buffer is full
    Serial.println("Oops Buffer Full"); // if full, do nothing
  }

  alarmInc(incSecs); // increment next alarm by x seconds
  blink(2, 250); // blink for confirmation
}

void doTimeUpdate() {
  while (1) {
    //Request time packet
    Serial.println("Requesting time update");
    packet = "update";
    sendMessage(packet);

    LoRa.onReceive(onReceive);
    LoRa.receive();
    //Serial.println(LoRaData);

    if (LoRaData != "" ) {
      updateTime(LoRaData);
      blink(5, 50);
      break;
    }

    // Loop if not recieved
    blink(2, 1000);
    delay(500);
  }
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
