# Solar-Powered IoT Charging Station

A prototype for a solar-powered mobile charging station controlled via a web interface and ESP32 microcontroller, featuring UPI payment integration.

## Project Overview

This system allows users to:
1. Select charging duration through a web interface
2. Make payment via UPI QR code
3. Start charging after successful payment verification
4. Monitor charging status in real-time
5. Automatically charge lead acid battery via solar panel when not in use

## Components

### Hardware Required
- ESP32 Development Board
- 5V Relay Module
- 12V Lead Acid Battery
- Solar Charge Controller
- Solar Panel (12V)
- USB Charging Module (5V output)
- Connecting wires
- Project enclosure

### Software Stack
- Frontend: HTML, CSS, JavaScript
- Payment: UPI QR Code Integration
- Database: Firebase Realtime Database
- Microcontroller: ESP32 (Arduino Framework)
- Communication: WiFi + Firebase

## Project Structure
```
solar-charging-station/
├── web/                    # Web frontend
│   ├── index.html         # Main web interface
│   ├── css/               # Styling
│   └── js/                # JavaScript files
├── esp32/                 # ESP32 Arduino code
│   └── charging_station/  # Main ESP32 sketch
└── diagrams/             # Circuit diagrams
```

## Setup Instructions

### 1. Firebase Setup
1. Create a new Firebase project
2. Enable Realtime Database
3. Copy Firebase configuration to `web/js/firebase-config.js`
4. Set database rules to allow read/write (for prototype only)

### 2. UPI Payment Setup
1. Replace the UPI ID in `web/js/main.js` with your merchant UPI ID
2. Test the payment flow in development mode
3. Ensure proper error handling for payment verification

### 3. Web Frontend
1. Open `web/index.html` in a browser
2. Configure charging duration
3. Scan UPI QR code and make payment
4. Click "Start Charging" after payment verification

### 4. ESP32 Setup
1. Install required libraries in Arduino IDE:
   - Firebase ESP32 Client
   - ArduinoJson
2. Upload `esp32/charging_station/charging_station.ino`
3. Configure WiFi credentials in the code

### 5. Hardware Setup
1. Follow wiring diagram in `diagrams/`
2. Connect ESP32 to relay
3. Connect battery through solar charge controller
4. Connect USB charging module
5. Ensure proper solar panel orientation for optimal charging

## Usage Flow
1. User selects desired charging duration
2. System calculates payment amount (₹20 per hour)
3. Displays UPI QR code for payment
4. User scans QR code and completes payment
5. After payment verification, charging begins
6. System monitors charging duration and stops automatically
7. Solar panel charges lead acid battery when station is idle

## Safety Notes
- This is a prototype system
- Always follow proper safety guidelines when working with batteries
- Ensure proper ventilation for lead acid batteries
- Use appropriate fuses and protection circuits
- Monitor charging temperature
- Implement proper surge protection

## Future Enhancements
- Payment success verification API integration
- Real-time power monitoring
- Battery level indicators
- Multiple charging port support
- Solar charging efficiency monitoring
- Mobile app integration
- Admin dashboard for monitoring

## License
MIT License 