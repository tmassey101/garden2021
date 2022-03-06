#include <SPI.h>
#include <LoRa.h>
#include "Adafruit_seesaw.h"

Adafruit_seesaw ss;
int counter = 0;

#include <RTCZero.h>

/* Create an rtc object */
RTCZero rtc;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  rtc.begin(); // initialize RTC 24H format
  String initialSet = "xxxx000000000000";
  updateTime(initialSet);
  
  Serial.println("LoRa Startup");
  if (!LoRa.begin(868E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  // Recieve time update
  while (1){
    int receipt = 0;
    String LoRaData;
    
    //Request time packet
    Serial.println("Requesting time update");
    String packet = "update";
    sendMessage(packet);
    
    for(int i=0 ; i=5000; i++)
    {
      int packetSize = LoRa.parsePacket();
      if (packetSize) 
      {
        // received a packet
        Serial.print("Received packet '");
        receipt = 1;

        // read packet
        while (LoRa.available())
        {
          LoRaData = LoRa.readString();
          Serial.print(LoRaData); 
        }

        // print RSSI of packet
        Serial.print("' with RSSI ");
        Serial.println(LoRa.packetRssi());
        if (receipt) break;
      }
    }
    
    // if update received update time then break loop
    if (receipt) {
      updateTime(LoRaData);
      break;
    }
    // retry if no response after 1 second
    delay(1000);
  }

  //Serial.println("seesaw Soil Sensor example!");
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

    //Serial.print("Temperature: "); Serial.print(tempC); Serial.println("*C");
    //Serial.print("Capacitive: "); Serial.println(capread);

    String timestamp =print2digits(rtc.getHours())+print2digits(rtc.getMinutes())+print2digits(rtc.getSeconds());
    String header = "p/";
    String packet = header+tempC+"/"+capread+"/"+timestamp;

    // Serial confirmation
    Serial.print("Sending packet: ");
    Serial.print(packet);
    Serial.print(" as number: ");
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
  //Serial.print("Sent packet: "); Serial.println(outgoing);
  //msgCount++;                           // increment message ID
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