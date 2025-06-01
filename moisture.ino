#include <WiFi.h>
#include <PubSubClient.h>

// WiFi and MQTT Configuration
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* mqtt_server = "MQTT_BROKER_IP"; // e.g., "192.168.1.100"
const int REPORT_INTERVAL_MIN = 30; // Minutes between reports

// MQTT Topics
const char* control_topic = "home/sensor/soil_moisture/control";
const char* moisture_topic = "home/sensor/soil_moisture/value";

// Sensor Setup
const int moisturePin = A0;  // Analog pin for soil sensor
const int dryValue = 4095;   // Raw value in dry air (adjust)
const int wetValue = 2000;   // Raw value in water (adjust)

WiFiClient espClient;
PubSubClient client(espClient);

// ====================== WiFi Setup ======================
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected\nIP address: ");
  Serial.println(WiFi.localIP());
}

// ====================== MQTT Callback ======================
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  // Convert payload to string
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  if (message == "get_moisture") {
    publishMoisture();
  }
}

// ====================== MQTT Reconnect ======================
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("SoilMoistureSensor")) {
      Serial.println("connected");
      client.subscribe(control_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 seconds...");
      delay(5000);
    }
  }
}

// ====================== Publish Moisture Data ======================
void publishMoisture() {
  int rawValue = analogRead(moisturePin);
  int moisturePercent = constrain(map(rawValue, dryValue, wetValue, 0, 100), 0, 100);

  char payload[8];
  dtostrf(moisturePercent, 1, 0, payload); // Format: 0-100 with no decimals

  client.publish(moisture_topic, payload);
  Serial.print("Published: ");
  Serial.print(payload);
  Serial.println("%");
}

// ====================== Main Setup ======================
void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  pinMode(moisturePin, INPUT);
}

// ====================== Main Loop ======================
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Automatic reporting
  static unsigned long lastReport = 0;
  if (millis() - lastReport > (REPORT_INTERVAL_MIN * 60 * 1000)) {
    publishMoisture();
    lastReport = millis();
  }

  delay(100);
}
