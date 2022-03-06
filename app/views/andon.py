# Flask modules
from flask               import render_template, request, url_for, redirect, send_from_directory
from werkzeug.exceptions import HTTPException, NotFound, abort
from jinja2              import TemplateNotFound

# App modules
from app        import app, db
from app.models.models import digitalRead
from datetime import datetime
from pytz import timezone
import pendulum

'''
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
    
    return readings'''

def queryReadings(id):

    #readings = Reading.query.filter_by(unitID = id).all()
    count = digitalRead.query.filter_by(unitID = id).count()

    return count

@app.route('/digSummary', methods=['GET', 'POST'])
def andonCount():
           
    if request.args.get('id') is None:
        id = 0
    else: id = int(request.args.get('id'))

    count = queryReadings(id)
    
    msg = "Success. Total entries: " + str(count)
    print(msg)
    return msg