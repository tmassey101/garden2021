import os
from   decouple import config

basedir = os.path.abspath(os.path.dirname(__file__))

class Config():

    CSRF_ENABLED = True
	
    SECRET_KEY = config('SECRET_KEY', default='S3crEt_007')

    SQLALCHEMY_DATABASE_URI = os.environ.get('JAWSDB_URL') #or \
        #'sqlite:///' + os.path.join(basedir, 'db.sqlite3')

    SQLALCHEMY_TRACK_MODIFICATIONS = False
