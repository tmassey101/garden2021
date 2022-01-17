# SPDX-FileCopyrightText: 2020 Jerry Needell for Adafruit Industries
# SPDX-License-Identifier: MIT

# Example to receive addressed packed with ACK and send a response

import time
import datetime
import board
import busio
import digitalio
import adafruit_rfm69
import requests
import logging
import sys

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

# Constants
link = "https://garden2021.herokuapp.com/insert?"
errorWait = 1

log.debug("Starting script")

def splitPacket(packet):
    try:
        packet_data = [int(packet[0:2]),
                       int(packet[2:4]),
                       float(packet[4:8]),
                       int(packet[8:12]),
                       float(packet[12:16])]
        log.info ("Split packet successfully: %s" % packet_data)
    except Exception as err:
        log.error("Packet Split Error: %s" % packet)
        log.error(err)
        
    return packet_data

def uploadPacket(packet_data):
    
    unitID = packet_data[0]
    sensorID = packet_data[1]
    temp = packet_data[2]
    mois = packet_data[3]
    batV = packet_data[4]
    #try:
    # Combine link address with reading
    url = (link+"temp="+str(temp)
           +"&mois="+str(mois)
           +"&sensorID="+str(sensorID)
           +"&unitID="+str(unitID)
           +"&bat="+str(batV))   
    log.debug("Created URL: %s" % url)
    
    try:
        # Send HTTP request to DB 
        response = requests.get(url)
        # If the response was successful, no Exception will be raised
        response.raise_for_status()
        timenow = datetime.datetime.now()
        log.info('Success! Posted reading')
        #log.info('Now waiting %d secs' % waitTime)
        #time.sleep(waitTime)

    except Exception as err:
        log.error('Other error occurred. Waiting %d seconds to retry' % errorWait)
        log.error(err)
        time.sleep(errorWait)
    

# Define radio parameters.
RADIO_FREQ_MHZ = 433.0  # Frequency of the radio in Mhz. Must match your
# module! Can be a value like 915.0, 433.0, etc.

# Define pins connected to the chip.
# set GPIO pins as necessary - this example is for Raspberry Pi
CS = digitalio.DigitalInOut(board.CE1)
RESET = digitalio.DigitalInOut(board.D25)

# Initialize SPI bus.
spi = busio.SPI(board.SCK, MOSI=board.MOSI, MISO=board.MISO)
# Initialze RFM radio
rfm69 = adafruit_rfm69.RFM69(spi, CS, RESET, RADIO_FREQ_MHZ)

# Set TX power
rfm69.tx_power = 20

# Optionally set an encryption key (16 byte AES key). MUST match both
# on the transmitter and receiver (or be set to None to disable/the default).
rfm69.encryption_key = (
    b"\x01\x02\x03\x04\x05\x06\x07\x08\x01\x02\x03\x04\x05\x06\x07\x08"
)

# set delay before transmitting ACK (seconds)
rfm69.ack_delay = 0.5
# set node addresses
rfm69.node = 1
rfm69.destination = 2
# initialize counter
counter = 0
ack_failed_counter = 0

# Wait to receive packets.
log.info("Waiting for packets...")

while True:
    
    # Look for a new packet: only accept if addresses to my_node
    packet = rfm69.receive(with_ack=True, with_header=True)
    # If no packet was received during the timeout then None is returned.
    if packet is not None:
        # Received a packet!
        # Print out the raw bytes of the packet:
        #print("Received (raw header):", [hex(x) for x in packet[0:4]])
        log.info("Received (raw payload): {0}".format(packet[4:]))
        #print(packet)
        log.info("RSSI: {0}".format(rfm69.last_rssi))
     
        try:
            packet_data = splitPacket(packet[4:])      
            uploadPacket(packet_data)
        except Exception as err:
            log.error("Upload failed. Err: ", err)  
        
        # send response x sec after any packet received
        #time.sleep(0.5)
        counter += 1
         
        # send a  mesage to destination_node from my_node
        log.debug("Sending ack")
        
        if not rfm69.send_with_ack(
            bytes("response from node {} {}".format(rfm69.node, counter), "UTF-8")
        ):
            ack_failed_counter += 1
            log.warning(" No Ack: %d from %d" % (counter, ack_failed_counter)) 
