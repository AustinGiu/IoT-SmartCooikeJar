from datetime import datetime, date, timedelta
from flask import Blueprint, jsonify
from models import db, SnackLog

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
