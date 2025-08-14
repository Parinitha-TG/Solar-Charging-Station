import http.server
import socketserver
import firebase_admin
from firebase_admin import credentials
from firebase_admin import db

# Firebase configuration
cred = credentials.Certificate("firebase-key.json")  # Use the existing firebase-key.json file
firebase_admin.initialize_app(cred, {
    'databaseURL': 'https://solar-powered-charging-station-default-rtdb.asia-southeast1.firebasedatabase.app'
})

# Reset Firebase values
def reset_firebase_values():
    print("Resetting Firebase values...")
    db.reference('/charging_station').set({
        'charging': False,
        'duration': 0,
        'paymentStatus': False,
        'startTime': 0
    })
    print("Firebase values reset successfully")

# Web server configuration
PORT = 8080
DIRECTORY = "web"

class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DIRECTORY, **kwargs)

# Reset Firebase values before starting the server
reset_firebase_values()

# Start the web server
with socketserver.TCPServer(("", PORT), Handler) as httpd:
    print(f"Serving at http://localhost:{PORT}")
    httpd.serve_forever() 