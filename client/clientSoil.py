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

from board import SCL, SDA
import busio
 
from adafruit_seesaw.seesaw import Seesaw
 
i2c_bus = busio.I2C(SCL, SDA)
 
ss = Seesaw(i2c_bus, addr=0x36)

# API address
link = "https://garden2021.herokuapp.com/insert?"
unitID = 1
sensorID = 1



while True:
    
    f = open("log.txt", "a")
    
    try: 
        # read moisture level through capacitive touch pad
        touch = ss.moisture_read()
 
        # read temperature from the temperature sensor
        temp = ss.get_temp()
 
        #print("temp: " + str(temp) + "  moisture: " + str(touch))
        time.sleep(1)

    except: 
        # Get sensor reading
        temp = 0    # Placeholder
        touch = 300

    # Combine link address with reading
    url = link+"temp="+str(temp)+"&mois="+str(touch)+"&unitID="+str(unitID)+"&sensorID="+str(sensorID)

    try:
        response = requests.get(url)
        # If the response was successful, no Exception will be raised
        response.raise_for_status()

    except Exception as err:
        print('Other error occurred: %s' % err, file = f)
        print('Waiting 5 seconds to retry', file = f)
        f.close()
        time.sleep(5)

    else:
        print('Success! Posted reading = %f to %s' % (temp, url), file = f )
        print('Waiting 300 secs...', file = f)
        f.close()
        time.sleep(300)

   

