import os

from flask            import Flask
from flask_sqlalchemy import SQLAlchemy
from flask_migrate import Migrate, MigrateCommand
from dotenv import load_dotenv

#from flask_login      import LoginManager
#from flask_bcrypt     import Bcrypt

load_dotenv()  # take environment variables from .env.

# Grabs the folder where the script runs.
basedir = os.path.abspath(os.path.dirname(__file__))


app = Flask(__name__)

app.config.from_object('app.config.Config')

db = SQLAlchemy(app)
migrate = Migrate(app, db)

#bc = Bcrypt(app) 

#lm = LoginManager(   ) # flask-loginmanager
#lm.init_app(app)       # init the login manager

# Setup database
@app.before_first_request
def initialize_database():
    db.create_all()

from app.models.models import Book, Reading

# Import routing, models and Start the App
from app.views import views, garden, api
from app.models import models


