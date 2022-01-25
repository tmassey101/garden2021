# SPDX-FileCopyrightText: 2018 Brent Rubell for Adafruit Industries
#
# SPDX-License-Identifier: MIT

"""
Example for using the RFM9x Radio with Raspberry Pi.

Learn Guide: https://learn.adafruit.com/lora-and-lorawan-for-raspberry-pi
Author: Brent Rubell for Adafruit Industries
"""
# Import Python System Libraries
import time
import datetime
import requests
import logging
import sys
# Import Blinka Libraries
import busio
from digitalio import DigitalInOut, Direction, Pull
import board
# Import the SSD1306 module.
import adafruit_ssd1306
# Import RFM9x
import adafruit_rfm9x

# Upload URL
link = "https://garden2021.herokuapp.com/insert?"
errorWait = 1
radio_freq = 868.0 #RF freq in MHz (868.0 = EU)

# Logging setup
log=logging.getLogger(__name__)

logging.basicConfig(
    level=logging.INFO ,
    format='%(asctime)s::%(levelname)s\t::%(message)s',
    handlers=[
        logging.FileHandler("debug.log"),
        logging.StreamHandler(sys.stdout)
        ]
    )



# Button A
btnA = DigitalInOut(board.D5)
btnA.direction = Direction.INPUT
btnA.pull = Pull.UP

# Button B
btnB = DigitalInOut(board.D6)
btnB.direction = Direction.INPUT
btnB.pull = Pull.UP

# Button C
btnC = DigitalInOut(board.D12)
btnC.direction = Direction.INPUT
btnC.pull = Pull.UP

# Create the I2C interface.
i2c = busio.I2C(board.SCL, board.SDA)

# 128x32 OLED Display
reset_pin = DigitalInOut(board.D4)
display = adafruit_ssd1306.SSD1306_I2C(128, 32, i2c, reset=reset_pin)
# Clear the display.
display.fill(0)
display.show()
width = display.width
height = display.height

# Configure LoRa Radio
CS = DigitalInOut(board.CE1)
RESET = DigitalInOut(board.D25)
spi = busio.SPI(board.SCK, MOSI=board.MOSI, MISO=board.MISO)

rfm9x = adafruit_rfm9x.RFM9x(spi, CS, RESET, radio_freq)
rfm9x.tx_power = 23
prev_packet = None

print("Listening on "+str(radio_freq)+"MHz")

def uploadPacket(packet_data):
    
    unitID = 1
    sensorID = 1
    temp = packet_data[1]
    mois = packet_data[2]
    batV = 0
    #try:
    # Combine link address with reading
    url = (link+"temp="+str(temp)
           +"&mois="+str(mois)
           +"&sensorID="+str(sensorID)
           +"&unitID="+str(unitID)
           +"&bat="+str(batV))   
    #log.debug("Created URL: %s" % url)
    print(url)
    
    try:
        # Send HTTP request to DB 
        response = requests.get(url)
        # If the response was successful, no Exception will be raised
        response.raise_for_status()
        timenow = datetime.datetime.now()
        #log.info('Success! Posted reading')
        #log.info('Now waiting %d secs' % waitTime)
        #time.sleep(waitTime)

    except Exception as err:
        #log.error('Other error occurred. Waiting %d seconds to retry' % errorWait)
        #log.error(err)
        print('HTTP error:', err)
        time.sleep(1)



while True:
    packet = None
    # draw a box to clear the image
    display.fill(0)
    display.text('RasPi LoRa', 35, 0, 1)

    # check for packet rx
    packet = rfm9x.receive(with_header=True)
    if packet is None:
        display.show()
        display.text('- Waiting for PKT -', 15, 20, 1)
    else:
        # Display the packet text and rssi
        display.fill(0)
        prev_packet = packet
        packet_text = str(prev_packet, "utf-8")
        print(packet_text)
        
        data = packet_text.split('-')
        print(data)
        
        uploadPacket(data)
        
        
        # Update LED display
        display.text('RX: ', 0, 0, 1)
        display.text(packet_text, 25, 0, 1)
        time.sleep(1)

    if not btnA.value:
        # Send Button A
        display.fill(0)
        button_a_data = bytes("Button A!\r\n","utf-8")
        rfm9x.send(button_a_data)
        display.text('Sent Button A!', 25, 15, 1)
    elif not btnB.value:
        # Send Button B
        display.fill(0)
        button_b_data = bytes("Button B!\r\n","utf-8")
        rfm9x.send(button_b_data)
        display.text('Sent Button B!', 25, 15, 1)
    elif not btnC.value:
        # Send Button C
        display.fill(0)
        button_c_data = bytes("Button C!\r\n","utf-8")
        rfm9x.send(button_c_data)
        display.text('Sent Button C!', 25, 15, 1)


    display.show()
    time.sleep(0.1)
