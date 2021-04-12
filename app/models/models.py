from app         import db
from datetime    import *

class Book(db.Model):
    title = db.Column(db.String(80), unique=True, nullable=False, primary_key=True)

    def __repr__(self):
        return "<Title: {}>".format(self.title)

class Reading(db.Model):
    __tablename__ = 'readings'
    id = db.Column('id', db.Integer, unique=True, nullable=False, primary_key=True)
    timestamp = db.Column('entry time', db.DateTime, default=datetime.utcnow())
    temperature = db.Column('temperature', db.Float, default=0.0)

    def __init__(self, temperature):
        self.temperature = temperature

    def to_json(self):
        return dict(id=self.name, name=self.name, temperature=self.datetime)

    def __repr__(self):
        return "<Entry Time: {}>".format(self.timestamp)