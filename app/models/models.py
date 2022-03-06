from app import db
from datetime import datetime

class Book(db.Model):
    title = db.Column(db.String(80), unique=True, nullable=False, primary_key=True)

    def __repr__(self):
        return "<Title: {}>".format(self.title)

class Reading(db.Model):
    __tablename__ = 'sdata'
    id = db.Column('id', db.Integer, unique=True, nullable=False, primary_key=True)
    timestamp = db.Column('entryTime', db.DateTime, default=datetime.utcnow)
    recordtime = db.Column('recordtime', db.DateTime, default=datetime.utcnow)
    temperature = db.Column('temperature', db.Float, default=0)
    moisture = db.Column('moisture', db.Float, default=0)
    sensorID = db.Column('sensorID', db.Integer, default=0)
    unitID = db.Column('unitID', db.Integer, default=0)
    batV = db.Column('batteryV', db.Float, default=0)

    def __init__(self, recordtime, temperature, moisture, batV, sensorID, unitID):
        self.recordtime = recordtime
        self.temperature = temperature
        self.moisture = moisture
        self.sensorID = sensorID
        self.unitID = unitID
        self.batV = batV

    def to_json(self):
        return dict(id=self.name, temperature=self.datetime)

    def __repr__(self):
        return "<Entry Time: {}>".format(self.timestamp)

class digitalRead(db.Model):
    __tablename__ = 'digitalreads'
    id = db.Column('id', db.Integer, unique=True, nullable=False, primary_key=True)
    timestamp = db.Column('entryTime', db.DateTime, default=datetime.utcnow)
    recordtime = db.Column('recordtime', db.DateTime, default=datetime.utcnow)
    sensorID = db.Column('sensorID', db.Integer, default=0)
    unitID = db.Column('unitID', db.Integer, default=0)
    count = db.Column('count', db.Integer, default=0)
    lastState = db.Column('lastState', db.Integer, default=0)

    def __init__(self, recordtime, unitID, sensorID, count, lastState):
        self.recordtime = recordtime
        self.sensorID = sensorID
        self.unitID = unitID
        self.count = count
        self.lastState = lastState

    def to_json(self):
        return dict(id=self.id, count=self.count)

    def __repr__(self):
        return "<Entry Time: {}>".format(self.timestamp)