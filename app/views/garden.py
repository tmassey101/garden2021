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

def queryReadings(values=None, id=None, sensor=None):

    readings = Reading.query.filter_by(unitID = id,sensorID = sensor).all()
    if values != None:
        readings = readings[-values:]
    readings = convertTime(readings)
    format = "%Y-%m-%dT%H:%M:%S"
    t = []
    m = []
    x = []
    b = []
    x2 = []
    
    for reading in readings:

        t.append(reading.temperature)
        m.append(reading.moisture)
        b.append(reading.batV)
        xRaw = reading.local
        xProc = xRaw.strftime(format)
        x.append(xProc)
        x2Raw = reading.recordtime
        x2Proc = x2Raw.strftime(format)
        x2.append(x2Proc)

    return (x, x2, t, m, b)

def queryUnits():
    unitList = [] 
    sensorList = []
    units = Reading.query.with_entities(Reading.unitID).distinct()
    sensors = Reading.query.with_entities(Reading.sensorID).distinct()
    for unit in units:
        unitList.append(unit.unitID)
    for sensor in sensors:
        sensorList.append(sensor.sensorID)
    print(sensorList)
    return unitList, sensorList

@app.route('/garden', methods=['GET', 'POST'])
def garden():       
    unitList, sensorList = queryUnits()
    if request.args.get('id') is None:
        id = unitList[0]
    else: id = int(request.args.get('id'))
    if request.args.get('sensor') is None:
        sensor = sensorList[0]
    else: sensor = int(request.args.get('sensor'))
    if request.args.get('values') is None:
        values = 500 #default last 500 values to limit loads
    else: values =int(request.args.get('values'))
    (x,x2,t,m,b) = queryReadings(values=values,id=id, sensor=sensor)
    lastX = x[-1]
    return render_template("garden/garden.html", graph_x=x, graph2_x=x2, graph_y1=t, graph_y2=m, graph2_y=b, unitList=unitList, sensorList=sensorList, id=id, sensor=sensor, lastX=lastX)

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
    
    readings = Reading.query.order_by(Reading.timestamp.desc()).limit(1000).all()

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




