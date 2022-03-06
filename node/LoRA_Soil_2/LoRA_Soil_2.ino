#include <SPI.h>
#include <LoRa.h>
#include "Adafruit_seesaw.h"

Adafruit_seesaw ss;
int counter = 0;

#include <RTCZero.h>
/* Create an rtc object */
RTCZero rtc;

// Do RTC things!



void setup() {
  Serial.begin(115200);
  //#while (!Serial);

  Serial.println("LoRa Sender");

  if (!LoRa.begin(868E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  Serial.println("seesaw Soil Sensor example!");
  
  if (!ss.begin(0x36)) {
    Serial.println("ERROR! seesaw not found");
    while(1) delay(1);
  } else {
    Serial.print("seesaw started! version: ");
    Serial.println(ss.getVersion(), HEX);
  }

  
  delay(1000);
}

int lastTime = millis();
int trigger = 0;
int interval = 10000;

void loop() {
  
  int time = millis();
  //Serial.print("Last Time: ");Serial.println(lastTime);
  //Serial.print("Time: ");Serial.println(time);
  
  //Serial.print("Trigger: ");Serial.println(trigger);
  if (time > trigger) {
    //Serial.println("Send message");
    lastTime = time;
    trigger = trigger + interval;
  
    float tempC = ss.getTemp();
    uint16_t capread = ss.touchRead(0);

    Serial.print("Temperature: "); Serial.print(tempC); Serial.println("*C");
    Serial.print("Capacitive: "); Serial.println(capread);

    String header = "p/";
    String packet = header+tempC+"/"+capread+"/"+counter;

    // Serial confirmation
    Serial.print("Sending packet: ");
    Serial.print(packet);
    Serial.print(" in number: ");
    Serial.println(counter);
    
    // send packet
    sendMessage(packet);
    counter++;
    //Serial.println("Looping");
  }
}

void sendMessage(String outgoing) {
  LoRa.beginPacket();                   // start packet
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  //msgCount++;                           // increment message ID
}