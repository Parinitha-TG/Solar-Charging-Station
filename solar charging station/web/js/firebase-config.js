// Firebase configuration
// These credentials connect your web app to your Firebase project
const firebaseConfig = {
    apiKey: "AIzaSyBLf8_3RJfUUFDszjyIP7Ixcg28x5VyAdU",
    authDomain: "solar-powered-charging-station.firebaseapp.com",
    databaseURL: "https://solar-powered-charging-station-default-rtdb.asia-southeast1.firebasedatabase.app",
    projectId: "solar-powered-charging-station",
    storageBucket: "solar-powered-charging-station.firebasestorage.app",
    messagingSenderId: "99208542689",
    appId: "1:99208542689:web:600a4afa8fa807b0153aa0",
    measurementId: "G-2VCKEZMBN2"
};

// Initialize Firebase with compat libraries
firebase.initializeApp(firebaseConfig);

// Get database reference
const database = firebase.database();

// Reference to charging status
const chargingRef = database.ref('charging_station');

// Export for use in main.js
window.database = database;
window.chargingRef = chargingRef; 