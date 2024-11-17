from flask import Flask, request

app = Flask(__name__)

@app.route("/receive", methods=["POST"])
def receive():
    message = request.data.decode("utf-8")
    print(f"Received message: {message}")
    return "OK", 200

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=9988)