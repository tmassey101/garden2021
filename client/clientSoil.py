# Sensor setup details 

#Pi 3V3 to sensor VIN // Pin 1
#Pi GND to sensor GND // Pin 6 or 9
#Pi SCL to sensor SCL // Pin 3 / GPIO 8
#Pi SDA to sensor SDA // Pin 5 / GPIO 9
#https://learn.adafruit.com/adafruit-stemma-soil-sensor-i2c-capacitive-moisture-sensor/python-circuitpython-test
#sudo pip3 install adafruit-circuitpython-seesaw
 
import requests
from random import random
import time
import datetime

from board import SCL, SDA
import busio
 
from adafruit_seesaw.seesaw import Seesaw
 
i2c_bus = busio.I2C(SCL, SDA)
 
ss = Seesaw(i2c_bus, addr=0x36)

# API address
link = "https://garden2021.herokuapp.com/insert?"
unitID = 1
sensorID = 1
waitTime = 300
errorWait = 150
logfile = "log.txt"

def readSensor():
    # read moisture level through capacitive touch pad
    touch = ss.moisture_read()

    # read temperature from the temperature sensor
    temp = ss.get_temp()

    print("temp: " + str(temp) + "  moisture: " + str(touch))
    time.sleep(1)
    return (touch,temp)

def writeLog(logfile, data):

    currDate = str(datetime.datetime.now())
    logdata = data+" - "+currDate

    with open(logfile, 'a') as f:
        f.write('\n'+logdata)
    f.close()
    
    return

while True:
    
    try: 
        touch, temp = readSensor()

        # Combine link address with reading
        url = link+"temp="+str(temp)+"&mois="+str(touch)+"&unitID="+str(unitID)+"&sensorID="+str(sensorID)
        
        # Send HTTP request to DB
        response = requests.get(url)

        # If the response was successful, no Exception will be raised
        response.raise_for_status()
        
        print('Success! Posted reading = %f to %s. Now waiting %d secs' % (temp, link, waitTime))
        time.sleep(waitTime)
         
        
    except Exception as err:
        errorMsg = str(err)
        print('Other error occurred: %s. Waiting %d seconds to retry' % (errorMsg, errorWait) )
        writeLog(logfile, errorMsg)
        time.sleep(errorWait)
        
        # Get sensor reading defaults
        temp = 0    # Placeholder
        touch = 300   
