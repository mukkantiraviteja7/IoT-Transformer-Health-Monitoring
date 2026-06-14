/*
 * IoT-Based Transformer Health Monitoring System
 * Author  : Mukkanti Ravi Teja
 * Board   : NodeMCU ESP8266
 * Version : 1.0
 *
 * Monitors: Oil Temperature, Load Current, Primary Voltage,
 *           Oil Level, Humidity, Gas/Fault Detection
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

// ── WiFi Credentials ─────────────────────────────────────────────────────────
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// ── Pin Definitions ───────────────────────────────────────────────────────────
#define ONE_WIRE_BUS  D4   // DS18B20 temperature sensor
#define DHT_PIN       D3   // DHT11 humidity sensor
#define DHT_TYPE      DHT11
#define TRIG_PIN      D6   // HC-SR04 ultrasonic TRIG
#define ECHO_PIN      D7   // HC-SR04 ultrasonic ECHO
#define BUZZER_PIN    D5   // Buzzer
#define RED_LED       D8   // Fault indicator
#define GREEN_LED     D9   // Normal indicator
#define ANALOG_PIN    A0   // Shared analog (ACS712 / ZMPT101B / MQ2)

// ── Alert Thresholds ──────────────────────────────────────────────────────────
#define TEMP_WARNING    75.0   // °C
#define TEMP_CRITICAL   85.0   // °C
#define CURRENT_WARNING 20.0   // A
#define CURRENT_MAX     25.0   // A
#define OIL_LEVEL_MIN   70.0   // %
#define GAS_THRESHOLD   300    // ppm
#define HUMIDITY_MAX    75.0   // %

// ── Sensor & Server Objects ───────────────────────────────────────────────────
OneWire           oneWire(ONE_WIRE_BUS);
DallasTemperature tempSensor(&oneWire);
DHT               dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
ESP8266WebServer  server(80);

// ── Global Sensor Data ────────────────────────────────────────────────────────
struct SensorData {
  float oilTemp      = 0;
  float current      = 0;
  float voltage      = 0;
  float oilLevel     = 0;
  float humidity     = 0;
  int   gasLevel     = 0;
  bool  faultActive  = false;
  String faultMsg    = "Normal";
  unsigned long timestamp = 0;
};

SensorData data;
unsigned long lastRead = 0;
const unsigned long READ_INTERVAL = 5000; // 5 seconds

// ── Setup ─────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  Serial.println("\n⚡ Transformer Health Monitor Starting...");

  // Init pins
  pinMode(TRIG_PIN,    OUTPUT);
  pinMode(ECHO_PIN,    INPUT);
  pinMode(BUZZER_PIN,  OUTPUT);
  pinMode(RED_LED,     OUTPUT);
  pinMode(GREEN_LED,   OUTPUT);

  // Init sensors
  tempSensor.begin();
  dht.begin();

  // Init LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Transformer Mon.");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");

  // Connect WiFi
  connectWiFi();

  // Setup API endpoints
  server.on("/",        handleRoot);
  server.on("/data",    handleData);
  server.on("/status",  handleStatus);
  server.begin();

  Serial.println("✅ System Ready! IP: " + WiFi.localIP().toString());
  digitalWrite(GREEN_LED, HIGH);
}

// ── Main Loop ─────────────────────────────────────────────────────────────────
void loop() {
  server.handleClient();

  if (millis() - lastRead >= READ_INTERVAL) {
    readAllSensors();
    checkThresholds();
    updateDisplay();
    printSerial();
    lastRead = millis();
  }
}

// ── WiFi Connection ───────────────────────────────────────────────────────────
void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected: " + WiFi.localIP().toString());
  } else {
    Serial.println("\n⚠️  WiFi Failed — Running in offline mode");
  }
}

// ── Read All Sensors ──────────────────────────────────────────────────────────
void readAllSensors() {
  // 1. Oil Temperature (DS18B20)
  tempSensor.requestTemperatures();
  data.oilTemp = tempSensor.getTempCByIndex(0);
  if (data.oilTemp == -127.0) data.oilTemp = 0; // Sensor error

  // 2. Load Current (ACS712 — 30A module)
  //    ACS712 outputs 2.5V at 0A; sensitivity = 66mV/A for 30A version
  int rawCurrent = analogRead(ANALOG_PIN);
  float voltage_mV = (rawCurrent / 1023.0) * 3300.0; // 3.3V ADC
  data.current = abs((voltage_mV - 2500.0) / 66.0);
  if (data.current < 0.1) data.current = 0; // Remove noise

  // 3. Primary Voltage (ZMPT101B — calibrated)
  //    Calibration factor may need adjustment for your setup
  float rawVoltage = analogRead(ANALOG_PIN);
  data.voltage = (rawVoltage / 1023.0) * 250.0; // Map to 0-250V AC

  // 4. Oil Level (HC-SR04 Ultrasonic)
  //    Assuming tank height = 30cm, full oil = 5cm from top
  long duration;
  digitalWrite(TRIG_PIN, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  duration = pulseIn(ECHO_PIN, HIGH);
  float distance = (duration * 0.0343) / 2.0; // cm
  data.oilLevel = constrain(map(distance, 30, 5, 0, 100), 0, 100); // %

  // 5. Humidity & Ambient Temp (DHT11)
  float h = dht.readHumidity();
  data.humidity = isnan(h) ? 0 : h;

  // 6. Gas / Fault Detection (MQ-2)
  data.gasLevel = analogRead(ANALOG_PIN);

  data.timestamp = millis();
}

// ── Check Thresholds & Set Alerts ────────────────────────────────────────────
void checkThresholds() {
  data.faultActive = false;
  data.faultMsg    = "Normal";

  if (data.oilTemp >= TEMP_CRITICAL) {
    data.faultActive = true;
    data.faultMsg = "CRITICAL: High Temp!";
  } else if (data.oilTemp >= TEMP_WARNING) {
    data.faultActive = true;
    data.faultMsg = "WARNING: Temp Rising";
  } else if (data.current >= CURRENT_MAX) {
    data.faultActive = true;
    data.faultMsg = "CRITICAL: Overcurrent!";
  } else if (data.oilLevel <= OIL_LEVEL_MIN) {
    data.faultActive = true;
    data.faultMsg = "WARNING: Low Oil Level";
  } else if (data.gasLevel >= GAS_THRESHOLD) {
    data.faultActive = true;
    data.faultMsg = "CRITICAL: Gas Detected!";
  } else if (data.humidity >= HUMIDITY_MAX) {
    data.faultActive = true;
    data.faultMsg = "WARNING: High Humidity";
  }

  // Set LEDs and buzzer
  if (data.faultActive) {
    digitalWrite(RED_LED,   HIGH);
    digitalWrite(GREEN_LED, LOW);
    // Beep buzzer
    for (int i = 0; i < 3; i++) {
      digitalWrite(BUZZER_PIN, HIGH); delay(200);
      digitalWrite(BUZZER_PIN, LOW);  delay(100);
    }
  } else {
    digitalWrite(RED_LED,    LOW);
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(BUZZER_PIN, LOW);
  }
}

// ── LCD Display ───────────────────────────────────────────────────────────────
void updateDisplay() {
  static int screen = 0;
  lcd.clear();

  switch (screen % 3) {
    case 0:
      lcd.setCursor(0, 0); lcd.print("Temp: ");   lcd.print(data.oilTemp,1);   lcd.print("C");
      lcd.setCursor(0, 1); lcd.print("Current: ");lcd.print(data.current,1);   lcd.print("A");
      break;
    case 1:
      lcd.setCursor(0, 0); lcd.print("Voltage: ");lcd.print(data.voltage,1);   lcd.print("V");
      lcd.setCursor(0, 1); lcd.print("Oil Lvl: ");lcd.print(data.oilLevel,0);  lcd.print("%");
      break;
    case 2:
      lcd.setCursor(0, 0); lcd.print("Humidity:");lcd.print(data.humidity,0);  lcd.print("%");
      lcd.setCursor(0, 1);
      if (data.faultActive) { lcd.print("!! FAULT !!"); }
      else                   { lcd.print("Status: OK"); }
      break;
  }
  screen++;
}

// ── Serial Debug Output ───────────────────────────────────────────────────────
void printSerial() {
  Serial.println("────────────────────────────────");
  Serial.println("📊 Transformer Health Report");
  Serial.print("  🌡️  Oil Temp   : "); Serial.print(data.oilTemp);   Serial.println(" °C");
  Serial.print("  ⚡  Current    : "); Serial.print(data.current);   Serial.println(" A");
  Serial.print("  🔌  Voltage    : "); Serial.print(data.voltage);   Serial.println(" V");
  Serial.print("  💧  Oil Level  : "); Serial.print(data.oilLevel);  Serial.println(" %");
  Serial.print("  💨  Humidity   : "); Serial.print(data.humidity);  Serial.println(" %");
  Serial.print("  ☢️  Gas Level  : "); Serial.print(data.gasLevel);  Serial.println(" ppm");
  Serial.print("  🚦  Status     : ");
  if (data.faultActive) { Serial.println("⚠️  " + data.faultMsg); }
  else                   { Serial.println("✅ Normal"); }
  Serial.println("────────────────────────────────");
}

// ── Web Server Handlers ───────────────────────────────────────────────────────
void handleData() {
  StaticJsonDocument<256> doc;
  doc["oil_temp"]   = data.oilTemp;
  doc["current"]    = data.current;
  doc["voltage"]    = data.voltage;
  doc["oil_level"]  = data.oilLevel;
  doc["humidity"]   = data.humidity;
  doc["gas_level"]  = data.gasLevel;
  doc["fault"]      = data.faultActive;
  doc["fault_msg"]  = data.faultMsg;
  doc["uptime"]     = millis() / 1000;

  String json;
  serializeJson(doc, json);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void handleStatus() {
  String status = data.faultActive ? "FAULT" : "NORMAL";
  server.send(200, "text/plain", status);
}

void handleRoot() {
  server.sendHeader("Location", "/data");
  server.send(302);
}