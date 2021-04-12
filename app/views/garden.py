# Python modules
import os, logging 

# Flask modules
from flask               import render_template, request, url_for, redirect, send_from_directory
from werkzeug.exceptions import HTTPException, NotFound, abort
from jinja2              import TemplateNotFound

# App modules
from app        import app, db
from app.models.models import Reading
from datetime import datetime
from pytz import timezone

x = [1,2,3,4,5]
y = [0.1, 0.3, 1.1, 1.5, 1.4]

@app.route('/garden', methods=['GET', 'POST'])
def garden():

    readings = Reading.query.all()
    
    x = []
    y = []

    for reading in readings:
        x.append(reading.id)
        y.append(reading.temperature)
    
    print(x)
    print(y)

    return render_template("garden/garden.html", graph_x=x, graph_y=y)

@app.route('/gardenhome', methods=['GET', 'POST'])
def gardenhome():

    return render_template("garden/gardenindex.html")


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

@app.route('/reading', methods=['GET', 'POST'])
def reading():

    if request.form:
        reading = Reading(temperature=request.form.get("temperature"))
        db.session.add(reading)
        db.session.commit()
    
    readings = Reading.query.all()
    localReadings = convertTime(readings)
        
    return render_template("garden/reading.html", readings=localReadings)

@app.route('/newreading/<type>/<value>', methods=['GET','POST'])
def newReading(type, value):

    if str(type) == 'temperature':
            reading = Reading(temperature=value)
            db.session.add(reading)
            db.session.commit()
    
    else:
        return('Error: Attribute type "%s" not found' % type)

    readings = Reading.query.all()
    localReadings = convertTime(readings)

    return render_template("garden/reading.html", readings=readings)
