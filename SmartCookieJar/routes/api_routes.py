from datetime import datetime, date, timedelta
from flask import Blueprint, jsonify
from models import db, SnackLog
from collections import defaultdict

api_routes = Blueprint('api_routes', __name__)

@api_routes.route('/api/today_status')
def today_status():
    today_start = datetime.combine(date.today(), datetime.min.time())

    # Total number of cookies consumed today
    total_intake = db.session.query(db.func.sum(SnackLog.number_intake)) \
                             .filter(SnackLog.timestamp >= today_start) \
                             .scalar() or 0

    # Latest weight after snack event
    latest_log = SnackLog.query.order_by(SnackLog.timestamp.desc()).first()
    latest_weight = latest_log.weight_after if latest_log else 0

    return jsonify({
        'latest_weight': latest_weight,
        'today_total_intake': total_intake
    })


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
