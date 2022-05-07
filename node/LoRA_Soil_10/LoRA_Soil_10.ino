#include <CircularBuffer.h>
#include <LoRa.h>
#include "Adafruit_seesaw.h"

// Watchdog library that reboots unit after certain period of inactivity
#include <Adafruit_SleepyDog.h>

Adafruit_seesaw ss;

#define ms_per_hour  3600000
#define ms_per_min    60000
#define ms_per_sec    1000

const int devID = 12968; // Dev ID Unit 1
//const int devID = 11111; // Dev ID Test 
const int msgBuf = 500; //size of message buffer

unsigned long unixTime = 1; // Placeholder for UnixTime (SECONDS SINCE JAN 01 1970. (UTC))

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
unsigned long alarmElapse = millis(); // alarm timer
unsigned long lastUpdate = millis(); // storage for clock increments (as onboard RTC unreliable)
unsigned long relayStart = (4,294,967,295-1); // storage for start of relay timer
const int updateTimerInc = (60 * 30); // Increment (secs) to request time update
const int sensorInc = 10; // take sensor reading every n seconds
const int loopFloat = 700; // time in ms for message send to complete (to offset running time to maintain interval)
const int relayPin = 1;     // Pin number for watering pump relay
const int relayDelay = 1; // Water duration in seconds

// LED timer
unsigned long ledTimer = millis();
int ledInc = 1000;  // Increment LED strobe in n ms

char cUpdate[7] = "t/unix"; // static message code for requesting time update
char cMessage[msgBuf]; // create buffer of desired size
char LoRaData[251];    // buffer for Lora message. 251 is maximum chars
bool readData = true;  // flag to do sensor reading

int countdownMS = Watchdog.enable(20000); // Set watchdog timer

void setup()
{
  Serial.begin(115200);

  // Set pin types
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(relayPin, OUTPUT);
  
  if (!Serial){
    delay(5000);
  }
  
  Serial.println("LoRa Startup"); // Start LoRA module
  if (!LoRa.begin(868E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  setupSoilSensor(); // Initializes sensor library
  while(1){
    requestUnixTime();  // Request time update and update clock
    LoRa.onReceive(onReceive); //onRecieve interrupt, listens for LoRa packets for confirmation
    LoRa.receive(); // puts radio into recieve mode
    // Loop if not recieved
    delay(1000);
    if(unixTime != 1){
      break;
    }
    Serial.print("No response or not valid. timeInt =");
    Serial.println("Retrying in 2 seconds...");
    blink(2, 1000);
  }
  /*// Attach interrupt to taking sensor reading
  rtc.attachInterrupt(updateDataFlag); //when interrupt is triggered, update reading flag
  */
  
  Serial.flush();
  blink(5, 100); 
  //delay(500);
  Serial.println("STARTING UP"); 
}

void loop()
{
  /*
  if (readData == true){
    takeAnalogReading();
    readData = false;
  }*/
  
  if (sensorReading0.timeStamp != 0 ) {
    sendMessage(true, sensorReading0.timeStamp, sensorReading0.mois, sensorReading0.temp);  // sends radio message to base unit for upload to DB
    LoRa.onReceive(onReceive); //onRecieve interrupt, listens for LoRa packets
    LoRa.receive(); // puts radio into recieve mode
    
    // Delay to allow for confirmation before moving on
    delay(300);

    if (sensorReading0.timeStamp != 0 ) {
        structs.push(data::record{sensorReading0.timeStamp, sensorReading0.mois, sensorReading0.temp});
        Serial.println("Saving record in buffer");
        sensorReading0 = {0, 0, 0};
    }
  }

  // Checks for entries in buffer. If buffer not empty, tries to send messages and clear buffer
  if (!structs.isEmpty() ){
    //Serial.print("Sending: "); Serial.println(structs.first().timeStamp); 
    sendMessage(true, structs.first().timeStamp, structs.first().mois, structs.first().temp);  // sends radio message to base unit for upload to DB / true is flag for sensor reading
    delay(20); // Pause briefly to allow hub unit to respond and not recieve old response
    LoRa.onReceive(onReceive); //onRecieve interrupt, listens for LoRa packets for confirmation
    LoRa.receive(); // puts radio into recieve mode
    
    // Delay to allow for confirmation before moving on
    delay(250);
    }
  
   // Placeholder function to take reading on timer //
   if(millis() > alarmElapse + (sensorInc * 1000) - loopFloat){
      takeAnalogReading();
      alarmElapse = millis();
   }

      // Placeholder function to take reading on timer //
   if(relayStart != (4,294,967,295-1)){
      if(millis() > relayStart + (relayDelay * 1000)){
        digitalWrite(relayPin, LOW);
        Serial.println("Relay Off");
        relayStart = (4,294,967,295-1);
     }
   }
   
   // If buffer has entries, print out to Serial for monitoring
   if(millis() > elapsedTime + 10000){
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

  // If buffer has entries, print out to Serial for monitoring
   if(millis() > lastUpdate + (updateTimerInc * 1000)) {
      updateUnixTime();
   }
  
  // Strobe LED each defined interval
  if(millis() > ledTimer + ledInc){
    alterLED();
    ledTimer = millis();
  }
  
  //delay(5);
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
  char target = LoRaData[4];
   
  switch(target){
    case ('t'):
      Serial.print("Message Type = "); Serial.println("Time update");
      updateUnixTime();
      break;
    case ('c'):
      Serial.print("Message Type = "); Serial.println("Message Confirmation");
      if (recieveConfirmation(sensorReading0.timeStamp)) { // if correct confirmation is recieved
      Serial.print("Popping: "); Serial.println(sensorReading0.timeStamp); // confirm removal 
      sensorReading0 = { 0, 0, 0};
      }
      
      // If confirmation recieved, remove entry from buffer
      if (recieveConfirmation(structs.first().timeStamp)) { // if correct confirmation is recieved
        Serial.print("Confirmed. Shifting: ");
        data::print(structs.shift());
        Serial.println();
      }
      break;
    case ('z'):
      triggerRelay();
    default:
      Serial.println();Serial.print("Message type not recognised: "); Serial.println(LoRaData);
      break;
  }
  Serial.println("---");
  // Delay to wait for new message to clear
  delay(50);
}

void requestUnixTime (){
  Serial.println("Updating Unix Time");
  //Request time packet
  Serial.println("Requesting time update");
  sendMessage(false,0,0,0); // 'false' is flag to send update message signal
}

void updateUnixTime(){
  lastUpdate = millis(); // Updates lastUpdate flag with current CPU clock to calculate increment later

  char timeBuf[10];
    for (int i=6;i<16;i++){
      timeBuf[i-6] = LoRaData[i];
    }
  unsigned long timeInt = atoi(timeBuf);
  //Serial.print("timeInt: "); Serial.println(timeInt);
  
  if (checkTime(timeInt) == true){
    blink(5, 50);
    unixTime = timeInt;
    Serial.print("New unixTime set: ");
    Serial.println(unixTime);
  }

  //Reset watchdog timer at end of loop
  countdownMS = Watchdog.enable(20000);
}
  
unsigned long getUnixStamp (){
  //Serial.println("getUnixStamp()");
  unsigned long unixStamp;
  while(1){
    unsigned long now = millis();
    unsigned long timeInc = now - lastUpdate;
    unixStamp = unixTime + int(timeInc/1000);
    if (checkTime(unixStamp) == false){
      Serial.println("timecheck failed. Request update");
      updateUnixTime();  
    }
    else { break; }  
  }
  //Serial.print("unixStamp = "); Serial.println(unixStamp);
  return unixStamp;
}

bool checkTime(unsigned long timeInt) {
  if ( ( timeInt > 1648978179 ) && ( timeInt < 2000000000 ))// check time recieved is in acceptable range
  {
    return true;
  }
  else {
    Serial.println("Unix Time corrupted.");
    return false;
  }
}

void triggerRelay(){
  digitalWrite(relayPin, HIGH);
  relayStart = millis();
  Serial.println("Relay On");
}

// Interupt routine - takes sensor temp and capacitive moisure values in temporary storage, then increments alarm for next reading
void takeAnalogReading() { 
  
  Serial.println("takeAnalogReading");
  float tempC = ss.getTemp();
  uint16_t capread = ss.touchRead(0); 

  // For testing without sensor
  //float tempC = 0.0;
  //uint16_t capread = 0;

  unsigned long timestamp = getUnixStamp();
  
  sensorReading0.temp = tempC;
  sensorReading0.mois = capread;
  sensorReading0.timeStamp = timestamp;

  Serial.print("Taken reading at ");
  Serial.println(sensorReading0.timeStamp);
  blink(3, 100); // blink for confirmation

  //Reset watchdog timer at end of loop
  countdownMS = Watchdog.enable(20000);
}

void sendMessage(bool type, int timeStamp, int mois, float temp) {   // sends passed string as LoRa message
  //Serial.println("sendMessage");
  if (type == true){
    int tempInt = temp*100; 

    // creates message string, adds leading 0 to timestamp int if needed for sending
    if (timeStamp < 100000){ 
      snprintf(cMessage,msgBuf,"p/%d/%d/0%d/%d", tempInt, mois, timeStamp, devID); 
    }
    else {
      snprintf(cMessage,msgBuf,"p/%d/%d/%d/%d", tempInt, mois, timeStamp, devID); 
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

bool recieveConfirmation(unsigned long timestamp) {
  //Serial.println("recieveConfirmation");
  char charLora[10];
  for(int i=0;i<10;i++){
    charLora[i] = LoRaData[i+6];
  }
  int recTime = atoi(charLora);

  char charID[5];
  for(int i=0;i<5;i++){
    charID[i] = LoRaData[i+6+10+1];
  }
  int recID = atoi(charID);
  
  //Serial.print("recTime = "); Serial.println(recTime); 
  //Serial.print("recID = "); Serial.println(recID); 
  
  if (recTime == timestamp && recID == devID) { // compare message timestamp & ID to confirmation timestamp & devID, return true if match
      return true;
  }
  else {
      Serial.print("Confirmation not valid for unit. Ignored");
      return false;
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

void blink(int count, int pause) {
  for (int i = 0; i < count; i++) {
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(pause);                       // wait for a second
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    delay(pause);
  }
}

void alterLED(){
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

/*
void updateDataFlag() {
  readData = true;
}*/
