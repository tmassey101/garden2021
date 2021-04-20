# API routes for common tasks such as data entry

import os
from flask import Flask, request, redirect

# App modules
from app        import app, db
from app.models.models import Reading
from datetime import datetime
from pytz import timezone

'''

# function to convert 
def convertTime(readings):
    
    format = "%Y-%m-%d %H:%M:%S %Z%z"
    targettz = 'Europe/London'

    for reading in readings:
        timestamp = reading.timestamp
        timestamp = timestamp.astimezone(timezone('UTC'))
        timestamp_local = timestamp.astimezone(timezone(targettz))
        reading.local = timestamp_local
    
    return readings

@app.route('/insert', methods=['POST'])
def insertReading:

    timestamp = request.json['timestamp']
    id = request.json['id']
    temperature = request.json['temperature']

    newReading = 

    newReading = convertTime(newReading)
    db.session.add(newReading)
    db.session.commit()

    return 

'''