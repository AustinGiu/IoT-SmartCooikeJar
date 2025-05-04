from flask import Blueprint, render_template

html_routes = Blueprint('html_routes', __name__)


@html_routes.route('/')
def index():
    return "Welcome to the Smart Cookie Jar!"

@html_routes.route('/today')
def today_page():
    return render_template('today_consumption.html')

@html_routes.route('/past')
def past_page():
    return render_template('past_data_analysis.html')
