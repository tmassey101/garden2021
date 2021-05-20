# Python modules
import os, logging 

# Flask modules
from flask               import render_template, request, url_for, redirect, send_from_directory
from flask_login         import login_user, logout_user, current_user, login_required
from werkzeug.exceptions import HTTPException, NotFound, abort
from jinja2              import TemplateNotFound

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