# API routes for common tasks such as data entry

import os
from flask import Flask, request

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
    if request.args.get("t"):
        t = Reading(temperature=request.args.get("t"))
        db.session.add(t)
        db.session.commit()
    
    readings = Reading.query.all()
    localReadings = convertTime(readings)
        
    return render_template("garden/reading.html", readings=localReadings)