# models.py
from flask_sqlalchemy import SQLAlchemy
from datetime import datetime, date

# SQLAlchemy object instance
db = SQLAlchemy()

# Average weight of one cookie in grams
COOKIE_WEIGHT_GRAMS = 9.2

class SnackLog(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    timestamp = db.Column(db.DateTime, default=datetime.utcnow)
    weight_before = db.Column(db.Float, nullable=False)
    weight_after = db.Column(db.Float, nullable=False)
    number_intake = db.Column(db.Integer, nullable=False)

def calculate_intake(weight_before, weight_after):
    weight_diff = weight_before - weight_after
    if weight_diff < 0:
        return 0  # Refill or sensor error
    return round(weight_diff / COOKIE_WEIGHT_GRAMS)
