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

@app.route('/garden', methods=['GET', 'POST'])
def garden():

    readings = Reading.query.all()
    readings = convertTime(readings)
    format = "%d/%m/%Y %H:%M:%S"

    y = []
    x = []
    
    
    for reading in readings:
        y.append(reading.temperature)
        xRaw = reading.timestamp
        xProc = xRaw.strftime(format)
        x.append(xProc)

    print(y[1])
    print(x[1])

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
        t = Reading(temperature=request.form.get("temperature"))
        db.session.add(t)
        db.session.commit()
    
    readings = Reading.query.all()
    localReadings = convertTime(readings)
        
    return render_template("garden/reading.html", readings=localReadings)


