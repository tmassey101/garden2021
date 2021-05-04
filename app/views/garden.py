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
import pendulum

@app.route('/garden', methods=['GET', 'POST'])
def garden():

    readings = Reading.query.all()
    readings = convertTime(readings)
    format = "%d/%m/%Y %H:%M:%S"

    y1 = []
    y2 = []
    x = []
    
    for reading in readings:
        y1.append(reading.temperature)
        y2.append(reading.moisture)
        xLast = reading.timestamp
        xRaw = reading.local
        xProc = xRaw.strftime(format)
        print(xProc)
        x.append(xProc)

    return render_template("garden/garden.html", graph_x=x, graph_y1=y1, graph_y2=y2)

@app.route('/gardenhome', methods=['GET', 'POST'])
def gardenhome():

    return render_template("garden/gardenindex.html")


# function to convert 
def convertTime(readings):
    
    format = "%Y-%m-%d %H:%M:%S %Z%z"
    targettz = 'Europe/London'
    tz = pendulum.timezone(targettz)

    for reading in readings:
        timestamp = reading.timestamp
        p = pendulum.instance(timestamp)
        q = p.in_timezone(tz)
        reading.local = q
    
    return readings

@app.route('/reading', methods=['GET', 'POST'])
def reading():

    if request.form:
        t = Reading(temperature=request.form.get("temperature"))
        db.session.add(t)
        db.session.commit()
    
    readings = Reading.query.all()
    localReadings = convertTime(readings)
        
    return render_template("garden/reading.html", readings=localReadings)


