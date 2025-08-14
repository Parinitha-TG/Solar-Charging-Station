/*
 * Solar Charging Station - Final Version
 * Fixes database contention issues
 */

#include <WiFi.h>
#include <FirebaseESP32.h>

// WiFi credentials
#define WIFI_SSID "Parinitha"
#define WIFI_PASSWORD "pari@1234"

// Firebase credentials
#define API_KEY "AIzaSyBLf8_3RJfUUFDszjyIP7Ixcg28x5VyAdU"
#define DATABASE_URL "https://solar-powered-charging-station-default-rtdb.asia-southeast1.firebasedatabase.app"
#define DATABASE_SECRET "XkQ3fi8KyZzes3ZSAkTfNhGsPpNBPm0X6Xs5LozB"

// Relay pin
#define RELAY_PIN 26  // GPIO26
// For this specific relay module (SRD-05VDC-SL-C):
// When relay is NOT energized: COM is connected to NC, NO is disconnected
// When relay is energized: COM is connected to NO, NC is disconnected
// Since we want charging OFF by default (relay not energized), we should connect:
// - Solar controller positive to NC
// - Battery positive to COM
// This way, when relay is not energized, the circuit is complete (charging)
// When relay is energized, the circuit breaks (not charging)
// So to STOP charging, we need to energize the relay (set pin HIGH)
#define RELAY_ENERGIZE_TO_STOP_CHARGING true

// Timing constants
#define STATUS_CHECK_INTERVAL 10000  // Check status every 10 seconds
#define CHARGING_CHECK_INTERVAL 2000 // Check charging every 2 seconds (increased from 0.5s)
#define WIFI_TIMEOUT 30000          // 30 second timeout for WiFi
#define FIREBASE_TIMEOUT 20000      // 20 second timeout for Firebase

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variables to track charging state
volatile bool isCharging = false;
volatile unsigned long chargingStartTime = 0;
volatile unsigned long chargingDuration = 0;
unsigned long lastStatusCheck = 0;
unsigned long lastChargingCheck = 0;
bool firebaseInitialized = false;
bool chargingSessionLocked = false; // New flag to lock a charging session

// Function to turn relay on based on active high/low setting
void relayOn() {
  if (RELAY_ENERGIZE_TO_STOP_CHARGING) {
    digitalWrite(RELAY_PIN, LOW);
  } else {
    digitalWrite(RELAY_PIN, HIGH);
  }
  Serial.println("Relay turned ON");
  Serial.println("Relay pin state: " + String(digitalRead(RELAY_PIN) == HIGH ? "HIGH" : "LOW"));
}

// Function to turn relay off based on active high/low setting
void relayOff() {
  if (RELAY_ENERGIZE_TO_STOP_CHARGING) {
    digitalWrite(RELAY_PIN, HIGH);
  } else {
    digitalWrite(RELAY_PIN, LOW);
  }
  Serial.println("Relay turned OFF");
  Serial.println("Relay pin state: " + String(digitalRead(RELAY_PIN) == HIGH ? "HIGH" : "LOW"));
}

// Safety function to ensure relay is off
void ensureRelayOff() {
    Serial.println("Safety: Ensuring charging is OFF");
    stopCharging();
}

// Function to initialize Firebase database with default values
// IMPORTANT: This function now only initializes if the database is empty
void initializeFirebaseDefaults() {
    Serial.println("Checking if Firebase initialization is needed...");
    
    // First check if values already exist
    if (Firebase.getBool(fbdo, "/charging_station/charging")) {
        bool currentCharging = fbdo.boolData();
        Serial.print("Existing charging value found: ");
        Serial.println(currentCharging ? "true" : "false");
        
        // Do not overwrite existing values
        Serial.println("Database already initialized, not overwriting values");
        return;
    } 
    
    // Only initialize if we couldn't read existing values
    Serial.println("No existing values found, initializing with defaults");
    FirebaseJson json;
    json.add("charging", false);
    json.add("duration", 0);
    json.add("startTime", 0);
    json.add("paymentStatus", false);
    
    if (Firebase.setJSON(fbdo, "/charging_station", json)) {
        Serial.println("Firebase: Initialized with default values");
    } else {
        Serial.println("Firebase: Initialization failed");
        Serial.println("Error: " + fbdo.errorReason());
    }
}

// Function to connect to WiFi with timeout
bool connectToWiFi() {
    unsigned long startAttemptTime = millis();
    
    Serial.print("Connecting to WiFi");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    while (WiFi.status() != WL_CONNECTED && 
           millis() - startAttemptTime < WIFI_TIMEOUT) {
        Serial.print(".");
        delay(100); // Reduced delay during WiFi connection attempts
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("Connected with IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("Signal strength (RSSI): ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
        return true;
    } else {
        Serial.println("Failed to connect to WiFi");
        return false;
    }
}

// Function to initialize Firebase with timeout
bool initializeFirebase() {
    if (!WiFi.isConnected()) {
        Serial.println("Cannot initialize Firebase: WiFi not connected");
        return false;
    }
    
    unsigned long startAttemptTime = millis();
    
    Serial.println("Initializing Firebase...");
    
    // Print credentials (partially hidden for security)
    Serial.print("Database URL: ");
    Serial.println(DATABASE_URL);
    
    // Configure Firebase credentials
    config.database_url = DATABASE_URL;
    
    // Set database secret as legacy token
    config.signer.tokens.legacy_token = DATABASE_SECRET;
    Serial.println("Database secret set");
    
    // Set timeout for Firebase operations
    fbdo.setResponseSize(2048);
    
    Serial.println("Beginning Firebase connection...");
    
    // Initialize Firebase
    Firebase.begin(&config, &auth);
    
    // Set automatic reconnection
    Firebase.reconnectWiFi(true);
    
    // Set database read timeout to 1 minute
    Firebase.setReadTimeout(fbdo, 1000 * 60);
    Firebase.setwriteSizeLimit(fbdo, "tiny");
    
    // Wait for Firebase to be ready with timeout
    Serial.println("Waiting for Firebase ready state...");
    
    while (!Firebase.ready() && 
           millis() - startAttemptTime < FIREBASE_TIMEOUT) {
        Serial.print(".");
        delay(500);
    }
    Serial.println();
    
    if (Firebase.ready()) {
        Serial.println("Firebase initialized successfully");
        firebaseInitialized = true;
        return true;
    } else {
        Serial.println("Firebase initialization failed");
        Serial.println("Last Error: " + fbdo.errorReason());
        Serial.println("Possible reasons:");
        Serial.println("1. Incorrect API key or database URL");
        Serial.println("2. Poor internet connection");
        Serial.println("3. Firebase may be blocking the connection");
        firebaseInitialized = false;
        return false;
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000); // Give serial connection time to settle
    
    Serial.println("\nStarting Solar Charging Station - Final Version");
    Serial.println("ESP32 SDK: " + String(ESP.getSdkVersion()));
    
    // Initialize relay pin and ensure it's OFF
    pinMode(RELAY_PIN, OUTPUT);
    
    // Make sure relay starts in OFF state
    ensureRelayOff();
    
    // Double-check relay state
    Serial.println("\nINITIAL RELAY STATUS:");
    Serial.println("Relay pin state: " + String(digitalRead(RELAY_PIN) == HIGH ? "HIGH" : "LOW"));
    Serial.println("RELAY_ENERGIZE_TO_STOP_CHARGING setting: " + String(RELAY_ENERGIZE_TO_STOP_CHARGING ? "true (energize to stop charging)" : "false (de-energize to stop charging)"));
    
    // Print initial memory info
    Serial.print("Initial free heap: ");
    Serial.println(ESP.getFreeHeap());
    
    // Connect to WiFi
    if (!connectToWiFi()) {
        Serial.println("Initial WiFi connection failed. Will keep retrying in loop.");
    } else {
        // Initialize Firebase
        if (initializeFirebase()) {
            // Initialize database with default values if connected
            // But only if there are no existing values
            initializeFirebaseDefaults();
        } else {
            Serial.println("Initial Firebase connection failed. Will keep retrying in loop.");
        }
    }
    
    lastStatusCheck = millis();
    lastChargingCheck = millis();
    
    // Print relay state for debugging
    Serial.println("\nRELAY STATUS CHECK:");
    Serial.println("Current relay state: " + String(digitalRead(RELAY_PIN) == HIGH ? "HIGH" : "LOW"));
    Serial.println("isCharging flag: " + String(isCharging ? "true" : "false"));
    Serial.println("RELAY_ENERGIZE_TO_STOP_CHARGING setting: " + String(RELAY_ENERGIZE_TO_STOP_CHARGING ? "true" : "false"));
}

void loop() {
    unsigned long currentMillis = millis();

    // Safety check - ensure relay is off if not properly connected
    if (WiFi.status() != WL_CONNECTED || !firebaseInitialized) {
        ensureRelayOff();
    }

    // Check WiFi and Firebase status every 10 seconds
    if (currentMillis - lastStatusCheck >= STATUS_CHECK_INTERVAL) {
        lastStatusCheck = currentMillis;
        
        // First check WiFi - this is the part with the 10-second interval
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("Status: WiFi disconnected, attempting reconnection...");
            ensureRelayOff();
            if (connectToWiFi()) {
                // If WiFi reconnected, also reconnect Firebase (no delay here)
                initializeFirebase();
            }
        } else if (!firebaseInitialized || !Firebase.ready()) {
            // WiFi is connected but Firebase is not ready (no delay here)
            Serial.println("Status: Firebase disconnected, attempting reconnection...");
            ensureRelayOff();
            initializeFirebase();
        } else {
            // If we reach here, both WiFi and Firebase are connected
            Serial.println("Status: System OK - WiFi and Firebase connected");
            // Print memory usage
            Serial.print("Free heap: ");
            Serial.println(ESP.getFreeHeap());
            
            // Print relay state for debugging
            Serial.println("Relay pin state: " + String(digitalRead(RELAY_PIN) == HIGH ? "HIGH" : "LOW"));
            Serial.println("isCharging flag: " + String(isCharging ? "true" : "false"));
            
            // Direct Firebase path check for debugging
            Serial.println("\nDirect Firebase path check:");
            
            // Check charging status directly
            if (Firebase.getBool(fbdo, "/charging_station/charging")) {
                bool charging = fbdo.boolData();
                Serial.print("charging = ");
                Serial.println(charging ? "true" : "false");
                
                // Check payment status directly
                if (Firebase.getBool(fbdo, "/charging_station/paymentStatus")) {
                    bool payment = fbdo.boolData();
                    Serial.print("paymentStatus = ");
                    Serial.println(payment ? "true" : "false");
                    
                    // Check duration directly
                    if (Firebase.getInt(fbdo, "/charging_station/duration")) {
                        int dur = fbdo.intData();
                        Serial.print("duration = ");
                        Serial.println(dur);
                        
                        // Check startTime directly
                        if (Firebase.getInt(fbdo, "/charging_station/startTime")) {
                            unsigned long start = fbdo.intData();
                            Serial.print("startTime = ");
                            Serial.println(start);
                        } else {
                            Serial.println("Failed to read startTime");
                        }
                    } else {
                        Serial.println("Failed to read duration");
                    }
                } else {
                    Serial.println("Failed to read paymentStatus");
                }
            } else {
                Serial.println("Failed to read charging status");
            }
        }
    }

    // Only proceed with charging checks if properly connected
    if (WiFi.status() == WL_CONNECTED && firebaseInitialized && Firebase.ready()) {
        // Check charging status every 2 seconds
        if (currentMillis - lastChargingCheck >= CHARGING_CHECK_INTERVAL) {
            lastChargingCheck = currentMillis;
            
            // If we're already charging, just check if we need to stop based on elapsed time
            if (isCharging) {
                unsigned long elapsedTime = currentMillis - chargingStartTime;
                Serial.println("Charging progress: " + String(elapsedTime/1000) + "/" + String(chargingDuration/1000) + " seconds");
                
                if (elapsedTime >= chargingDuration) {
                    Serial.println("Timer completed");
                    stopCharging();
                    updateFirebase(false);
                    chargingSessionLocked = false; // Unlock the session
                }
                return; // Skip the rest of the function if we're already charging
            }
            
            // Only check Firebase if we're not in a charging session
            if (!chargingSessionLocked) {
                bool shouldCharge = false;
                bool paymentStatus = false;
                int duration = 0;
                unsigned long startTime = 0;
                bool readSuccess = false;
                
                // Read all required values first
                if (Firebase.getBool(fbdo, "/charging_station/charging")) {
                    shouldCharge = fbdo.boolData();
                    readSuccess = true;
                    
                    Serial.print("Read charging status: ");
                    Serial.println(shouldCharge ? "true" : "false");
                    
                    if (shouldCharge) {
                        // Get payment status
                        if (Firebase.getBool(fbdo, "/charging_station/paymentStatus")) {
                            paymentStatus = fbdo.boolData();
                            
                            Serial.print("Read payment status: ");
                            Serial.println(paymentStatus ? "true" : "false");
                            
                            if (!paymentStatus) {
                                // Payment not completed, don't charge
                                Serial.println("Payment not verified, not starting charge");
                                return;
                            }
                            
                            // Continue with charging logic
                            if (Firebase.getInt(fbdo, "/charging_station/duration")) {
                                duration = fbdo.intData();
                                
                                Serial.print("Read duration: ");
                                Serial.println(duration);
                                
                                if (Firebase.getInt(fbdo, "/charging_station/startTime")) {
                                    startTime = fbdo.intData();
                                    
                                    Serial.print("Read startTime: ");
                                    Serial.println(startTime);
                                    
                                    // Validate values
                                    if (duration > 0 && startTime > 0) {
                                        // Start new charging session
                                        Serial.println("Starting new charging session");
                                        chargingDuration = duration * 1000;  // Convert to milliseconds
                                        chargingStartTime = millis();  // Use local time, not Firebase time
                                        chargingSessionLocked = true; // Lock the session
                                        startCharging();
                                    } else {
                                        // Invalid values, don't start charging
                                        Serial.println("Invalid duration or startTime, not starting charge");
                                    }
                                } else {
                                    Serial.println("Failed to read startTime: " + fbdo.errorReason());
                                    readSuccess = false;
                                }
                            } else {
                                Serial.println("Failed to read duration: " + fbdo.errorReason());
                                readSuccess = false;
                            }
                        } else {
                            Serial.println("Failed to read paymentStatus: " + fbdo.errorReason());
                            readSuccess = false;
                        }
                    }
                } else {
                    Serial.println("Failed to read charging status: " + fbdo.errorReason());
                    readSuccess = false;
                }
                
                // Error reading from Firebase, check if it's a serious error
                if (!readSuccess) {
                    // Check if it's an authentication or connection error
                    if (fbdo.errorReason().indexOf("auth") >= 0 || 
                        fbdo.errorReason().indexOf("token") >= 0 || 
                        fbdo.errorReason().indexOf("timeout") >= 0) {
                        // Serious error, try to reinitialize Firebase
                        Serial.println("Serious Firebase error, attempting to reinitialize...");
                        firebaseInitialized = false;
                        initializeFirebase();
                    }
                }
            }
        }
    }
}

// Function to start charging (connect the circuit)
void startCharging() {
    Serial.println("Action: Starting charging");
    Serial.println("Duration: " + String(chargingDuration/1000) + " seconds");
    
    // To start charging, we want COM connected to NC (default state)
    // So we de-energize the relay
    if (RELAY_ENERGIZE_TO_STOP_CHARGING) {
        digitalWrite(RELAY_PIN, LOW);  // De-energize relay
    } else {
        digitalWrite(RELAY_PIN, HIGH); // Energize relay
    }
    
    Serial.println("Relay state for charging ON: " + String(digitalRead(RELAY_PIN) == HIGH ? "HIGH (energized)" : "LOW (de-energized)"));
    isCharging = true;  // Set flag after setting relay
}

// Function to stop charging (break the circuit)
void stopCharging() {
    Serial.println("Action: Stopping charging");
    
    // To stop charging, we want COM disconnected from NC
    // So we energize the relay
    if (RELAY_ENERGIZE_TO_STOP_CHARGING) {
        digitalWrite(RELAY_PIN, HIGH);  // Energize relay
    } else {
        digitalWrite(RELAY_PIN, LOW);   // De-energize relay
    }
    
    Serial.println("Relay state for charging OFF: " + String(digitalRead(RELAY_PIN) == HIGH ? "HIGH (energized)" : "LOW (de-energized)"));
    
    // Clear charging flags
    isCharging = false;
    chargingStartTime = 0;
    chargingDuration = 0;
}

void updateFirebase(bool charging) {
    if (!firebaseInitialized) {
        Serial.println("Cannot update Firebase: not initialized");
        return;
    }
    
    // ONLY update the charging flag, not the other values
    if (Firebase.setBool(fbdo, "/charging_station/charging", charging)) {
        Serial.println("Firebase: Updated charging status successfully");
    } else {
        Serial.println("Firebase: Update failed");
        Serial.println(fbdo.errorReason());
        
        // Check if it's an authentication or connection error
        if (fbdo.errorReason().indexOf("auth") >= 0 || 
            fbdo.errorReason().indexOf("token") >= 0 || 
            fbdo.errorReason().indexOf("timeout") >= 0) {
            // Serious error, try to reinitialize Firebase
            Serial.println("Serious Firebase error during update, attempting to reinitialize...");
            firebaseInitialized = false;
            initializeFirebase();
        }
    }
} 
