#define RELAY_PIN  26  // Define the GPIO pin connected to the relay

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);  // Set relay pin as output
}

void loop() {
  digitalWrite(RELAY_PIN, HIGH);  // Turn relay ON
  Serial.println("Relay is HIGH (ON)");
  delay(5000);  // Wait for 5 seconds

  digitalWrite(RELAY_PIN, LOW);  // Turn relay OFF
  Serial.println("Relay is LOW (OFF)");
  delay(5000);  // Wait for 5 seconds
}
