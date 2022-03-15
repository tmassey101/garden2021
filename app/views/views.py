# Python modules
import os, logging
from unicodedata import decimal 

# Flask modules
from flask               import render_template, request, url_for, redirect, send_from_directory
from flask_login         import login_user, logout_user, current_user, login_required
from werkzeug.exceptions import HTTPException, NotFound, abort
from jinja2              import TemplateNotFound
from flask_wtf import FlaskForm
from wtforms import StringField, SubmitField, IntegerField, DecimalField, FloatField
from wtforms.validators import DataRequired
from decimal import *

# App modules
from app        import app, db
from app.models.models import Book
#from app.forms  import LoginForm, RegisterForm

# Grabs the folder where the script runs.
basedir = os.path.abspath(os.path.dirname(__file__))

@app.route('/', methods=['GET', 'POST'])
def index():
    return render_template("index.html")

@app.route('/blog', methods=['GET', 'POST'])
def blog():

    postName = 'TestPost'
    path = returnPost(postName)
    print(path)

    with open(path) as f:
        blog_text = f.read()
        
    print(blog_text)
    
    #blog_image = 'xxx'
    #blog_image_link = str("{{ url_for('static', filename='assets/img/"+blog_image+".jpg}}")

    #return render_template("blog.html", blog_text=blog_text)
    #return (str(path))
    return render_template("blog.html", blog_text=blog_text)

# TEST APP BELOW
@app.route("/update", methods=["POST"])
def update():
    newtitle = request.form.get("newtitle")
    oldtitle = request.form.get("oldtitle")
    book = Book.query.filter_by(title=oldtitle).first()
    book.title = newtitle
    db.session.commit()
    return redirect("/entry")

@app.route("/delete", methods=["POST"])
def delete():
    title = request.form.get("title")
    book = Book.query.filter_by(title=title).first()
    db.session.delete(book)
    db.session.commit()
    return redirect("/entry")

@app.route('/entry', methods=['GET', 'POST'])
def entry():
    
    if request.form:
        book = Book(title=request.form.get("title"))
        db.session.add(book)
        db.session.commit()
        print(book)
    books = Book.query.all()

    return render_template("entry.html", books=books)

def returnPost(postName):

    basedir = app.root_path
    path = "\\static\\assets\\posts\\"+postName+".txt"
    path = basedir + path
    return path

class EnergyForm(FlaskForm):
    milage = IntegerField('milage', validators=[DataRequired()])
    mpg = FloatField('mpg', validators=[DataRequired()])
    energykw = FloatField('EnergyKW', validators=[DataRequired()])
    poundsLi = FloatField('poundsLi', validators=[DataRequired()])
    poundsKw = FloatField('poundsKw', validators=[DataRequired()])
    submit = SubmitField('Submit')

@app.route('/ccalc', methods=['GET', 'POST'])
def ccalc():

    form = EnergyForm()
    milage = 10000
    mpg = 40.0
    energyPerKw = 3.0
    poundsLi = 1.55
    poundsKw = 0.36  

    # on POST get form values
    if request.method == 'POST':
        print('Post request')
        milage = int(request.form['milage'])
        mpg = float(request.form['mpg'])
        energyPerKw = float(request.form['energykw'])
        poundsLi = float(request.form['poundsLi'])
        poundsKw = float(request.form['poundsKw'])
        
    # calculations
    totalEnergyKw = round(float(milage/energyPerKw),2)
    totalFuelLitres =  round(float(4.55 * (milage / mpg)),2)
    costPerYearKw = round((totalEnergyKw * poundsKw),2)
    costPerYearLitres = round((totalFuelLitres * poundsLi),2)
    
    calculations = [
        totalEnergyKw,
        totalFuelLitres,
        costPerYearKw,
        costPerYearLitres 
    ]

    form.milage.data = milage
    form.mpg.data = mpg
    form.energykw.data = energyPerKw
    form.poundsLi.data = poundsLi
    form.poundsKw.data = poundsKw

    print(calculations)

    return render_template('ccalc.html', form=form, calc=calculations)