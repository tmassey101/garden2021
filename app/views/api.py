# API routes for common tasks such as data entry

import os
from flask import Flask, request, render_template

# App modules
from app        import app, db
from app.models.models import Reading, digitalRead
from datetime import datetime
from pytz import timezone

# function to convert timestamp to local timezone
def convertTime(readings):
    
    format = "%Y-%m-%d %H:%M:%S %Z%z"
    targettz = 'Europe/London'

    for reading in readings:
        timestamp = reading.timestamp
        timestamp = timestamp.astimezone(timezone('UTC'))
        timestamp_local = timestamp.astimezone(timezone(targettz))
        reading.local = timestamp_local
    
    return readings

@app.route('/insert', methods=['GET'])
def insertReading():
    print("Inserting value")

    if request.args.get("rtime"): 
        recordtime = request.args.get("rtime")
    else: recordtime = None

    if request.args.get("temp"): temperature = request.args.get("temp")
    else: temperature = None
    if request.args.get("mois"): moisture = request.args.get("mois")
    else: moisture = None
    if request.args.get("bat"): batV = request.args.get("bat")
    else: batV = None
    if request.args.get("sensorID"): sensorID = request.args.get("sensorID")
    else: sensorID = 0
    if request.args.get("unitID"): unitID = request.args.get("unitID")
    else: unitID = 0

    reading = Reading(recordtime, temperature, moisture, batV, sensorID, unitID)
    db.session.add(reading)
    db.session.commit()
        
    return "Success"

@app.route('/digins', methods=['GET'])
def insertDigital():
    print("Inserting value")

    if request.args.get("rtime"): recordtime = request.args.get("rtime")
    else: recordtime = None

    if request.args.get("sensorID"): sensorID = request.args.get("sensorID")
    else: sensorID = 0
    if request.args.get("unitID"): unitID = request.args.get("unitID")
    else: unitID = 0

    if request.args.get("count"): count = request.args.get("count")
    else: count = None
    if request.args.get("lastState"): lastState = request.args.get("lastState")
    else: lastState = None

    digReading = digitalRead(recordtime, unitID, sensorID, count, lastState)
    db.session.add(digReading)
    db.session.commit()
        
    return count