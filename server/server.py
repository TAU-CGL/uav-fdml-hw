from datetime import datetime

from flask import Flask, render_template, request
from flask_socketio import SocketIO, emit

from database import Database

DATABASE = "messages.db"
DATABASE_MAX_MESSAGES = 50
DATABASE_RESET_KEYWORD = "$$$RESET_DB$$$"

app = Flask(__name__)
socketio = SocketIO(app)

@app.route("/")
def index():
    return render_template("index.html")

@app.route("/receive", methods=["POST"])
def receive():
    message = request.data.decode("utf-8")
    timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S') 
    if DATABASE_RESET_KEYWORD in message:
        print(message, "-- requesting reset!")
        db.reset()
        socketio.emit("reset")
        return "RESET", 200
    
    db.append_message(message)
    socketio.emit('new_message', {'message': message, 'timestamp': timestamp})
    return "OK", 200

@app.route("/fetch_messages", methods=["GET"])
def fetch_messages():
    show_distance = request.args.get('show_distance', 'true').lower() == 'true'
    messages = db.get_last_messages(show_distance)
    return {'messages': [{'message': msg, 'timestamp': ts} for msg, ts in messages]}, 200

if __name__ == "__main__":
    db = Database(DATABASE, DATABASE_MAX_MESSAGES)
    app.run(host="0.0.0.0", port=9988)