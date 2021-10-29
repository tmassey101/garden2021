# API routes for common tasks such as data entry

import os
from flask import Flask, request, render_template

# App modules
from app        import app, db
from app.models.models import Reading
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

    if request.args.get("temp"): temperature = request.args.get("temp")
    else: temperature = 0.0
    if request.args.get("mois"): moisture = request.args.get("mois")
    else: moisture = 500
    if request.args.get("bat"): batV = request.args.get("bat")
    else: batV = 0.0
    if request.args.get("sensorID"): sensorID = request.args.get("sensorID")
    else: sensorID = 0
    if request.args.get("unitID"): unitID = request.args.get("unitID")
    else: unitID = 0

    reading = Reading(temperature, moisture, batV, sensorID, unitID)
    db.session.add(reading)
    db.session.commit()
    
    readings = Reading.query.order_by(Reading.timestamp.desc()).all()
    localReadings = convertTime(readings)
        
    return render_template("garden/reading.html", readings=localReadings)

@app.route('/newreading/<temp>', methods=['GET','POST'])
def newReading(temp):
    
    t = Reading(temperature=temp)
    db.session.add(t)
    db.session.commit()
    
    readings = Reading.query.all()
    localReadings = convertTime(readings)
        
    return render_template("garden/reading.html", readings=localReadings)