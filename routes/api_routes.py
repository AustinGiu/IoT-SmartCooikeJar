from datetime import datetime, date, timedelta
from math import ceil
from flask import Blueprint, jsonify, request, current_app
from models import db, SnackLog
from collections import defaultdict
import sqlite3
import os

api_routes = Blueprint('api_routes', __name__)

# default settings
# the standard weight for a cookie
COOKIE_WEIGHT_GRAMS = 9.2
# The daily limit for cookies' intake
DAILY_LIMIT = 5
# The container lid status
lock_status = "UNLOCK" 
# Lid lock time
lock_until = None

# fetch the data for today's consumption page
@api_routes.route('/api/today_status')
def today_status():
    global lock_until

    now = datetime.now()
    today_start = datetime.combine(date.today(), datetime.min.time())

    # Total number of cookies consumed today
    total_intake = db.session.query(db.func.sum(SnackLog.number_intake)) \
                             .filter(SnackLog.timestamp >= today_start) \
                             .scalar() or 0

    # Latest weight after snack event
    latest_log = SnackLog.query.order_by(SnackLog.timestamp.desc()).first()
    latest_weight = latest_log.weight_after if latest_log else 0

    # Determine if locked
    is_locked = lock_until and now < lock_until
    lock_status = "LOCK" if is_locked else "UNLOCK"

    return jsonify({
        'latest_weight': latest_weight,
        'today_total_intake': total_intake,
        'lock_status': lock_status,
        'lock_until': lock_until.isoformat() if lock_until else None
    })

# fetch the data for past data page
@api_routes.route('/api/usage_summary', methods=['GET'])
def usage_summary():
    logs = SnackLog.query.order_by(SnackLog.timestamp).all()

    if not logs:
        return jsonify({
            "days_used": 0,
            "total_cookies": 0,
            "days_with_5_cookies": 0,
            "days_with_0_cookies": 0
        })

    # Step 1: Build a dict of daily cookie counts
    daily_counts = defaultdict(int)
    for log in logs:
        day = log.timestamp.date()
        daily_counts[day] += log.number_intake

    # Step 2: Create full date range from first log to today
    start_date = logs[0].timestamp.date()
    end_date = datetime.now().date()
    total_days = (end_date - start_date).days + 1

    days_with_5 = 0
    days_with_0 = 0
    total_cookies = 0

    for i in range(total_days):
        current_date = start_date + timedelta(days=i)
        intake = daily_counts.get(current_date, 0)
        total_cookies += intake
        if intake == 5:
            days_with_5 += 1
        if intake == 0:
            days_with_0 += 1

    return jsonify({
        "days_used": total_days,
        "total_cookies": total_cookies,
        "days_with_5_cookies": days_with_5,
        "days_with_0_cookies": days_with_0
    })


# fetch the data for creating the line chart
@api_routes.route('/api/7_day_intake', methods=['GET'])
def get_7_day_intake():
    # Get the current date
    today = datetime.now()
    # Calculate the date 7 days ago
    seven_days_ago = today - timedelta(days=7)

    # Query the database for snack logs in the past 7 days
    snack_logs = SnackLog.query.filter(SnackLog.timestamp >= seven_days_ago).all()

    # Initialize a dictionary to track daily cookie intake
    daily_intakes = {}

    for log in snack_logs:
        # Get the date (ignore the time part)
        log_date = log.timestamp.date()

        # Add the cookies taken to the daily intake (we assume this is the number of cookies)
        cookies_taken = log.number_intake  # Assuming this is the number of cookies

        # If we have data for this day, add the cookies to the total
        if log_date in daily_intakes:
            daily_intakes[log_date] += cookies_taken
        else:
            daily_intakes[log_date] = cookies_taken

    # Prepare the result for the last 7 days
    result = []
    for i in range(7):
        date = today - timedelta(days=i)
        # Get the intake for this date, defaulting to 0 if no data exists for this day
        intake = daily_intakes.get(date.date(), 0)
        result.append({
            'date': date.strftime('%Y-%m-%d'),
            'cookies': intake  # Return the number of cookies taken
        })

    return jsonify({'daily_intakes': result})


# get the path to database
def get_db_path():
    return os.path.join(current_app.instance_path, "snacklog.db")

# Function to calculate the cookies fetched today
def get_today_total_cookies():
    today = datetime.now().date()
    
    # Open a connection to the database
    with sqlite3.connect(get_db_path()) as conn:
        c = conn.cursor()
        
        # Get the total number of cookies consumed today by summing up the number_intake values for today's entries
        c.execute("""
            SELECT SUM(number_intake) 
            FROM snack_log 
            WHERE DATE(timestamp) = ?
        """, (today.isoformat(),))
        
        total_cookies = c.fetchone()[0]

    return total_cookies if total_cookies is not None else 0

# Route to get data from ESP32
@api_routes.route('/upload_weight', methods=['POST'])
def upload_weight():
    global lock_status, lock_until
    data = request.json
    weight_after = float(data.get('weight'))

    if weight_after is None:
        return jsonify({"error": "Missing weight"}), 400

    # Get the last row of data to retrieve the weight_before
    last_entry = None
    with sqlite3.connect(get_db_path()) as conn:
        c = conn.cursor()
        c.execute("SELECT weight_after FROM snack_log ORDER BY timestamp DESC LIMIT 1")
        last_entry = c.fetchone()

    # Trying to fetch the data from last row. If not, then set the weight_before as 0.
    if last_entry:
        weight_before = last_entry[0]
    else:
        weight_before = 0  # If no previous data, set weight_before to 0

    # If weight_after is greater than weight_before, it's a refill, so set intake to 0
    if weight_after > weight_before:
        number_intake = 0
    else:
        # Calculate the cookie intake number
        number_intake = int((weight_before - weight_after) / COOKIE_WEIGHT_GRAMS)

    # Insert the new weight data into the database
    now = datetime.now().isoformat()
    with sqlite3.connect(get_db_path()) as conn:
        c = conn.cursor()
        c.execute("""
    INSERT INTO snack_log (timestamp, weight_before, weight_after, number_intake)
    VALUES (?, ?, ?, ?)
    """, (now, weight_before, weight_after, number_intake))
        conn.commit()

    # Calculate the daily total cookies consumed
    daily_total = get_today_total_cookies()

    # Calculate and apply punishment lock
    if daily_total > DAILY_LIMIT:
        extra = daily_total - DAILY_LIMIT
        penalty_days = ceil(extra / DAILY_LIMIT)
        lock_until = datetime.now() + timedelta(days=penalty_days)

    # Final lid status decision
    if lock_until and datetime.now() < lock_until:
        lock_status = "LOCK"
    elif daily_total == DAILY_LIMIT:
        lock_status = "LOCK"
    else:
        lock_status = "UNLOCK"

    # Format lock_until for display
    formatted_lock_until = (
        lock_until.strftime("%d/%m/%y\n%-I:%M%p").lower()
        if lock_until else None
    )

    # Calculate how many cookies are left in the jar
    number_left = int(weight_after / COOKIE_WEIGHT_GRAMS)

    return jsonify({
        #showing the data's beening received
        "status": "logged",
        "cookie_intake": number_intake,
        "daily_total": daily_total,
        "lid_status": lock_status,
        "cookies_left": number_left,
        "lock_until": formatted_lock_until
    })

# Route to tell the ESP32 when to lock the lid
# when the status is changed to Locked, ESP32 should control to lock the lid and turn the led into red
@api_routes.route('/get_command', methods=['GET'])
def get_command():
    global lock_status, lock_until

    now = datetime.now()

    # Calculate daily remaining allowance
    today_total = get_today_total_cookies()
    cookies_remaining_today = max(DAILY_LIMIT - today_total, 0)

    if lock_until and now < lock_until:
        lock_status = "LOCK"
        reason = "punishment"
        formatted_lock_until = (
            lock_until.strftime("%-d/%-m/%y\n%-I:%M%p").lower()
            if lock_until else None
        )
    elif get_today_total_cookies() == DAILY_LIMIT:
        lock_status = "LOCK"
        reason = "daily_limit"
        formatted_lock_until = (
            lock_until.strftime("%-d/%-m/%y\n%-I:%M%p").lower()
            if lock_until else None
        )
    else:
        lock_status = "UNLOCK"
        reason = None
        lock_until = None  # clear punishment if expired
        formatted_lock_until = None

    return jsonify({
        "lid_status": lock_status,
        "reason": reason,
        "lock_until": formatted_lock_until,
        "cookies_remaining_today": cookies_remaining_today
    })

