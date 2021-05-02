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
link = "https://garden2021.herokuapp.com/insert?t="

while True:
    
    try: 
        # read moisture level through capacitive touch pad
        touch = ss.moisture_read()
 
        # read temperature from the temperature sensor
        temp = ss.get_temp()
 
        print("temp: " + str(temp) + "  moisture: " + str(touch))
        time.sleep(1)

    except: 
        # Get sensor reading
        temp = -1.0    # Placeholder

    # Combine link address with reading
    url = link+str(temp)

    try:
        response = requests.get(url)
        # If the response was successful, no Exception will be raised
        response.raise_for_status()
    except Exception as err:
        print('Other error occurred: %s', err)  
    else:
        print('Success! Posted reading = %f to %s' % (temp, url) )

    print('Waiting 60 secs...')
    time.sleep(60)

