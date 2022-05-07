// DIGITAL PIN 2 DOES NOT SUPPORT INTERRUPTS, SO CURRENTLY LOOPING ON TIMER CYCLE

#include <CircularBuffer.h>
#include <LoRa.h>
//#include "Adafruit_seesaw.h"

// Watchdog library that reboots unit after certain period of inactivity
#include <Adafruit_SleepyDog.h>

//Adafruit_seesaw ss;

#define ms_per_hour  3600000
#define ms_per_min    60000
#define ms_per_sec    1000

// Define Unit ID values
const int devID = 22222; // Dev ID Unit 1
//const int devID = 11111; // Dev ID Test 
const int msgBuf = 500; //size of message buffer

// Define Input Pins
const int digPin = 2;
const int greenLED = 3;
const int yellowLED = 6;

// Define increment timers
const int updateTimerInc = (60 * 30); // Increment (secs) to request time update
const int sensorInc = 3; // take sensor count every n seconds
const int incSecs = 10; // reading timer increment
const int ledInc = 10000;  // Increment LED strobe in n ms
const int GledInc = 500;
const int YledInc = 10;
const int confirmTimerInc = 250;


namespace data {
  typedef struct {
    char type;
    unsigned long timeStamp;
    int val1;
    int val2;
  } record;

  void print(record r) {
    Serial.print(r.type);
    Serial.print(" / ");
    Serial.print(r.timeStamp);
    Serial.print(" / ");
    Serial.print(r.val1);
    Serial.print(" / ");
    Serial.print(r.val2);
  }
}

//#define CIRCULAR_BUFFER_INT_SAFE
const int bufLen = 100;
CircularBuffer<data::record, bufLen> structs;

struct sensorReading{
  char type;
  unsigned long timeStamp;
  int val1;
  int val2;
  } record;
  
sensorReading sensorReading0 = { 0, 0, 0, 0};

// Set up counter for inputs
int digCount = 0; 
byte lastState = 0;
byte lastSentState = 0;

// Set up Timers
unsigned long unixTime = 1; // Placeholder for UnixTime (SECONDS SINCE JAN 01 1970. (UTC))
unsigned long elapsedTime = millis();  // trigger for wait timer
unsigned long confirmTimer = millis();  // trigger for msg confirmation time-out
unsigned long msgElapse = millis(); // Timer stores time of last sent message
unsigned long lastUpdate = millis(); // storage for clock increments (as onboard RTC unreliable)
const int loopFloat = 700; // time in ms for message send to complete (to offset running time to maintain interval)

// LED timers
unsigned long ledTimer = millis();
unsigned long yellowLEDTimer = millis();
unsigned long greenLEDTimer = millis();


char tMessage[7] = "t/unix"; // message code for requesting time update
char loraMessage[msgBuf]; // create buffer of desired size
char LoRaData[251];    // buffer for Lora message. 251 is maximum chars
bool readData = true;  // flag to do sensor reading

int countdownMS = Watchdog.enable(20000); // Set watchdog timer

void setup()
{
  Serial.begin(115200);

  // Set pin modes
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(yellowLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(digPin, INPUT);
  
  if (!Serial){
    delay(5000);
  }
  
  Serial.println("LoRa Startup"); // Start LoRA module
  if (!LoRa.begin(868E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

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
      blinkPin(yellowLED, 2, 500);
      delay(1000);
    }
    /*// Attach interrupt to taking sensor reading
    rtc.attachInterrupt(updateDataFlag); //when interrupt is triggered, update reading flag
    */
  
  Serial.flush();
  blinkPin(greenLED, 5, 100); 
  Serial.println("STARTING UP"); 
}

void loop()
{
  /* // DIGITAL PIN 2 DOES NOT SUPPORT INTERRUPTS, SO CURRENTLY LOOPING ON TIMER CYCLE
  if (readData == true){
    takeAnalogReading();
    readData = false;
  }*/
  if (sensorReading0.timeStamp != 0 ) {
    sendMessage(sensorReading0.type, sensorReading0.timeStamp, sensorReading0.val1, sensorReading0.val2);  // sends radio message to base unit for upload to DB
    LoRa.onReceive(onReceive); //onRecieve interrupt, listens for LoRa packets
    LoRa.receive(); // puts radio into recieve mode
    
    // Delay to allow for confirmation before moving on
    delay(300);

    if (sensorReading0.timeStamp != 0 ) {
        structs.push(data::record{sensorReading0.type, sensorReading0.timeStamp, sensorReading0.val1, sensorReading0.val2});
        Serial.println("Saving record in buffer");
        sensorReading0 = {0, 0, 0};
    }
  }

  // Checks for entries in buffer. If buffer not empty, tries to send messages and clear buffer
  if (!structs.isEmpty() ){
    //Serial.print("Sending: "); Serial.println(structs.first().timeStamp); 
    sendMessage(structs.first().type, structs.first().timeStamp, structs.first().val1, structs.first().val2);  // sends radio message to base unit for upload to DB / true is flag for sensor reading
    delay(20); // Pause briefly to allow hub unit to respond and not recieve old response
    LoRa.onReceive(onReceive); //onRecieve interrupt, listens for LoRa packets for confirmation
    LoRa.receive(); // puts radio into recieve mode
    
    // Delay to allow for confirmation before moving on
    delay(250);
    }
  
   // Placeholder function to take reading on timer //
   if(millis() > msgElapse + (sensorInc * 1000) - loopFloat){
      checkDigitalCount();
      msgElapse = millis();
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
      requestUnixTime();
   }
     
  // Strobe BuiltIn LED each defined interval
  if(millis() > ledTimer + ledInc){
    alterLED(LED_BUILTIN);
    ledTimer = millis();
  }
  if(millis() > yellowLEDTimer + YledInc){
    digitalWrite(yellowLED, LOW);
    yellowLEDTimer = millis();  
  }
  if(millis() > greenLEDTimer + GledInc){
    digitalWrite(greenLED, LOW);
    greenLEDTimer = millis();  
  }

  // Read input pin value
  checkDigitalReading();

  // Placeholder function to take reading on timer //
  if(millis() > msgElapse + (sensorInc * 1000) - loopFloat){
    //takeAnalogReading();
    checkDigitalCount();
    msgElapse = millis();
  }

  // Reset Watchdog timer
  countdownMS = Watchdog.enable(20000);

  // delay(250);
}
  
/*
// DIGITAL PIN 2 DOES NOT SUPPORT INTERRUPTS, SO CURRENTLY LOOPING ON TIMER CYCLE
void updateDataFlag() {
  readData = true;
}*/
void requestUnixTime (){
  Serial.println("Updating Unix Time");
  //Request time packet
  Serial.println("Requesting time update");
  sendMessage('t',0,0,0); // 'false' is flag to send update message signal
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
    blinkPin(greenLED, 3, 100);
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
      Serial.println("Timecheck failed. Request update");
      updateUnixTime();  
    }
    else { break; }  
  }
  //Serial.print("unixStamp = "); Serial.println(unixStamp);
  return unixStamp;
}

bool checkTime(unsigned long timeInt) {
  if ( ( timeInt > 1648978179 ) && ( timeInt < 2000000000 ))// check time recieved is in acceptable range
  { return true; }
  else {
    Serial.println("Unix Time corrupted.");
    return false;
  }
}

// the loop routine runs over and over again forever:
void checkDigitalReading() {
  // read the input pin:
  byte pinState = digitalRead(digPin);
  if (pinState == 1){
    digitalWrite(yellowLED, HIGH); 
    if (lastState == 0) {
      lastState = 1;
      digCount++;
      // print out the state of the button & count:
      Serial.print("Digital Input: ");Serial.print(pinState);
      Serial.print(" // Digital Count: "); Serial.println(digCount);
    }
  }
  else if (pinState == 0){
    if (lastState == 1){
      lastState = 0;
    }
  }
}

// checks counter for any inputs, creates message if not 0
void checkDigitalCount(){
  if (digCount != 0 || lastSentState != lastState){
    char type = 'd';
    sensorReading0.type = type;
    sensorReading0.val1 = digCount;
    sensorReading0.val2 = lastState;
    unsigned long timestamp = getUnixStamp();
    sensorReading0.timeStamp = timestamp;

    Serial.print("Taken reading at ");
    Serial.println(sensorReading0.timeStamp);

    // Reset input counter to 0
    digCount = 0;
    lastSentState = lastState;
    
    //Reset watchdog timer at end of loop
    countdownMS = Watchdog.enable(20000);
  }
}

void sendMessage(char type, int timeStamp, int val1, int val2) {   // sends passed string as LoRa message
  // creates message string, adds leading 0 to timestamp int if needed for sending
  snprintf(loraMessage,msgBuf,"%c/%d/%d/%d/%d", type, val1, val2, timeStamp, devID);
  
  LoRa.beginPacket();                   // start packet
  if (type == 't') {  // Send custom message for time update
    LoRa.print(tMessage); 
    Serial.print("Sent: "); Serial.println(tMessage);
  }
  else {
    LoRa.print(loraMessage);                 // add payload
    Serial.print("Sent: "); Serial.println(loraMessage);
  }
  LoRa.endPacket();                     // finish packet and send it
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
  char target = LoRaData[4];
   
  switch(target){
    case ('t'):
      //Serial.print("Message Type = ");Serial.println("Time update");
      updateUnixTime();
      break;
    case ('c'):
      //Serial.print("Message Type = ");Serial.println("Message Confirmation");
      if (recieveConfirmation(sensorReading0.timeStamp)) { // if correct confirmation is recieved
        Serial.print("Popping: "); Serial.println(sensorReading0.timeStamp); // confirm removal 
        sensorReading0 = { 0, 0, 0};
        digitalWrite(greenLED, HIGH);
        greenLEDTimer = millis();
      }
      
      // If confirmation recieved, remove entry from buffer
      else if (recieveConfirmation(structs.first().timeStamp)) { // if correct confirmation is recieved
        Serial.print("Confirmed. Shifting: ");
        data::print(structs.shift());
        Serial.println();
        digitalWrite(greenLED, HIGH);
        greenLEDTimer = millis();
      }

      else {
        Serial.println("Confirmation not valid. Ignored");
      }
      break;
    case ('z'):
      break;
    case ('p'):
    break;
    default:
      Serial.print("Message type not recognised: "); Serial.println(LoRaData);
      break;
  }
  Serial.println("---");
  // Delay to wait for new message to clear
  delay(50);
}

void blink(int count, int pause) {
  for (int i = 0; i < count; i++) {
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(pause);                       // wait for a second
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    delay(pause);
  }
}

void blinkPin(int pin,int count, int pause) {
  for (int i = 0; i < count; i++) {
    digitalWrite(pin, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(pause);                       // wait for a second
    digitalWrite(pin, LOW);    // turn the LED off by making the voltage LOW
    delay(pause);
  }
}

void alterLED(int led){
  digitalWrite(led, !digitalRead(led));
}
