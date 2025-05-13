from flask import Flask
from flask_sqlalchemy import SQLAlchemy
from routes.html_routes import html_routes
from routes.api_routes import api_routes
from models import db

# Initialize app
app = Flask(__name__)


# Config
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///snacklog.db'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False

db.init_app(app)


# Register Blueprints

app.register_blueprint(html_routes)
app.register_blueprint(api_routes)

# Run
if __name__ == '__main__':
    with app.app_context():
        db.create_all()
    app.run(debug=True, host='0.0.0.0', port=8000)