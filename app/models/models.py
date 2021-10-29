from app  import db
from datetime       import *

class Book(db.Model):
    title = db.Column(db.String(80), unique=True, nullable=False, primary_key=True)

    def __repr__(self):
        return "<Title: {}>".format(self.title)

class Reading(db.Model):
    __tablename__ = 'readings'
    id = db.Column('id', db.Integer, unique=True, nullable=False, primary_key=True)
    timestamp = db.Column('entryTime', db.DateTime, default=datetime.utcnow)
    temperature = db.Column('temperature', db.Float, default=0.0)
    moisture = db.Column('moisture', db.Float, default=500)
    sensorID = db.Column('sensorID', db.Integer, default=0)
    unitID = db.Column('unitID', db.Integer, default=0)
    batV = db.Column('batteryV', db.Float, default=0)

    def __init__(self, temperature, moisture, batV, sensorID, unitID):
        self.temperature = temperature
        self.moisture = moisture
        self.sensorID = sensorID
        self.unitID = unitID
        self.batV = batV

    def to_json(self):
        return dict(id=self.name, temperature=self.datetime)

    def __repr__(self):
        return "<Entry Time: {}>".format(self.timestamp)
'''
class sensorTest(db.Model):    
    __tablename__ = 'sensorTest'
    id = db.Column('id', db.Integer, unique=True, nullable=False, primary_key=True)
    timestamp = db.Column('entryTime', db.DateTime, default=datetime.utcnow)
    sensorID = db.Column('sensorID', db.Integer, default=0)
    unitID = db.Column('unitID', db.Integer, default=0)
    value = db.Column('value', db.Float, default=0)

    def __init__(self, value, sensorID, unitID):
        self.value = value
        self.sensorID = sensorID
        self.unitID = unitID

    def to_json(self):
        return dict(id=self.name, timestamp=self.datetime)

    def __repr__(self):
        return "<Entry Time: {}>".format(self.timestamp)
'''