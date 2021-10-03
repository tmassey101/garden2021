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

def queryReadings(values=None):

    values = values
    readings = Reading.query.all()
    if values != None:
        readings = readings[-values:]
    readings = convertTime(readings)
    format = "%Y-%m-%dT%H:%M:%S"

    y1 = []
    y2 = []
    x = []
    
    for reading in readings:

        y1.append(reading.temperature)
        y2.append(reading.moisture)
        xLast = reading.timestamp
        xRaw = reading.local
        xProc = xRaw.strftime(format)
        x.append(xProc)

    return (x, y1, y2) 

@app.route('/garden', methods=['GET', 'POST'])
def garden():

    (x,y1,y2) = queryReadings()
    return render_template("garden/garden.html", graph_x=x, graph_y1=y1, graph_y2=y2)

@app.route('/garden500', methods=['GET', 'POST'])
def garden500():

    (x,y1,y2) = queryReadings(values=500)
    return render_template("garden/garden.html", graph_x=x, graph_y1=y1, graph_y2=y2)

@app.route('/timeexample', methods=['GET', 'POST'])
def timeexample():

    return render_template("garden/timeseriesexample.html")

@app.route('/gardenhome', methods=['GET', 'POST'])
def gardenhome():

    (x,y1,y2) = queryReadings()
    return render_template("garden/gardenindex.html", graph_x=x, graph_y1=y1, graph_y2=y2)

@app.route('/reading', methods=['GET', 'POST'])
def reading():

    if request.form:
        t = Reading(temperature=request.form.get("temperature"))
        db.session.add(t)
        db.session.commit()
    
    readings = Reading.query.order_by(Reading.timestamp.desc()).all()

    if request.args.get("q"):
        q = int(request.args.get("q"))
        readings = readings[-q:]

    localReadings = convertTime(readings)
        
    return render_template("garden/reading.html", readings=localReadings)

# function to convert 
def convertTime(readings):
    
    targettz = 'Europe/London'
    tz = pendulum.timezone(targettz)

    for reading in readings:
        timestamp = reading.timestamp
        p = pendulum.instance(timestamp)
        q = p.in_timezone(tz)

        # Formats timestamp to ISO8601 format
        q.isoformat()

        reading.local = q
    
    return readings




