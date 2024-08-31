#include "DHT.h"
#include <WiFi.h>
#include <ArduinoHttpClient.h>

#define DHTPIN D4
#define DHTTYPE DHT11
#define SOIL_MOISTURE_PIN A0
#define RELAY_PIN D3
#define N_SENSOR_PIN A1  // Nitrogen sensor pin
#define P_SENSOR_PIN A2  // Phosphorus sensor pin
#define K_SENSOR_PIN A3  // Potassium sensor pin

DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "GlobeAtHome_d7d38_2.4";
const char* password = "Jy6YEfHQ";
const char* serverName = "192.168.1.100";  // Replace with your XAMPP IP address or hostname
const int port = 80;
const char* postPath = "/save_data.php";

WiFiClient wifi;
HttpClient http(wifi, serverName, port);

bool relayOn = false;
unsigned long relayStartMillis = 0;
const unsigned long relayOnDuration = 2 * 60 * 1000;  // 2 minutes

void setup() {
    Serial.begin(9600);
    dht.begin();
    pinMode(DHTPIN, INPUT);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);

    Serial.print("Connecting to Wi-Fi");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(300);
    }

    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();
}

void loop() {
    showMenu();
}

void showMenu() {
    Serial.println("Select an option:");
    Serial.println("1. Environmental Monitoring (Temp & Humidity)");
    Serial.println("2. Soil Moisture Monitoring with Relay Control");
    Serial.println("3. Soil Nutrient Monitoring (NPK)");
    Serial.println("4. Add a Plant");
    Serial.println("5. Exit");

    while (!Serial.available());
    int option = Serial.parseInt();
    Serial.read();  // Consume the newline character

    switch (option) {
        case 1:
            environmentalMonitoring();
            break;
        case 2:
            soilMoistureMonitoring();
            break;
        case 3:
            soilNutrientMonitoring();
            break;
        case 4:
            addPlant();
            break;
        case 5:
            Serial.println("Exiting...");
            return;
        default:
            Serial.println("Invalid option. Please try again.");
            break;
    }
}

void environmentalMonitoring() {
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
        Serial.println("Failed to read from DHT sensor!");
        return;
    }

    if (WiFi.status() == WL_CONNECTED) {
        String postData = "category=environmental&humidity=" + String(h) + "&temperature=" + String(t);

        http.beginRequest();
        http.post(postPath);
        http.sendHeader("Content-Type", "application/x-www-form-urlencoded");
        http.sendHeader("Content-Length", postData.length());
        http.beginBody();
        http.print(postData);
        http.endRequest();

        int httpResponseCode = http.responseStatusCode();
        String response = http.responseBody();

        Serial.println(httpResponseCode);
        Serial.println(response);
    } else {
        Serial.println("Wi-Fi Disconnected");
    }
}

// Similar changes for soilMoistureMonitoring(), soilNutrientMonitoring(), and addPlant() functions.

void soilMoistureMonitoring() {
    int soilMoistureValue = analogRead(SOIL_MOISTURE_PIN);
    int soilMoisturePercent = map(soilMoistureValue, 620, 215, 0, 100);

    if (soilMoisturePercent < 0) soilMoisturePercent = 0;
    if (soilMoisturePercent > 100) soilMoisturePercent = 100;

    if (WiFi.status() == WL_CONNECTED) {
        String postData = "soil_moisture=" + String(soilMoisturePercent) + "&category=soil";

        http.beginRequest();
        http.post(postPath);
        http.sendHeader("Content-Type", "application/x-www-form-urlencoded");
        http.sendHeader("Content-Length", postData.length());
        http.beginBody();
        http.print(postData);
        http.endRequest();

        int httpResponseCode = http.responseStatusCode();
        String response = http.responseBody();

        Serial.println(httpResponseCode);
        Serial.println(response);
    } else {
        Serial.println("Wi-Fi Disconnected");
    }

    controlRelay(soilMoisturePercent);
}

void soilNutrientMonitoring() {
    int nitrogen = analogRead(N_SENSOR_PIN);
    int phosphorus = analogRead(P_SENSOR_PIN);
    int potassium = analogRead(K_SENSOR_PIN);

    if (WiFi.status() == WL_CONNECTED) {
        String postData = "nitrogen=" + String(nitrogen) + "&phosphorus=" + String(phosphorus) + "&potassium=" + String(potassium) + "&category=nutrient";

        http.beginRequest();
        http.post(postPath);
        http.sendHeader("Content-Type", "application/x-www-form-urlencoded");
        http.sendHeader("Content-Length", postData.length());
        http.beginBody();
        http.print(postData);
        http.endRequest();

        int httpResponseCode = http.responseStatusCode();
        String response = http.responseBody();

        Serial.println(httpResponseCode);
        Serial.println(response);
    } else {
        Serial.println("Wi-Fi Disconnected");
    }
}

void controlRelay(int soilMoisturePercent) {
    unsigned long currentMillis = millis();

    Serial.print("Relay Status: ");
    Serial.println(relayOn ? "ON" : "OFF");

    if (soilMoisturePercent <= 20 && !relayOn) {
        Serial.println("Turning relay ON");
        digitalWrite(RELAY_PIN, HIGH);
        relayOn = true;
        relayStartMillis = currentMillis;
    }

    if (relayOn && currentMillis - relayStartMillis >= relayOnDuration) {
        Serial.println("Turning relay OFF after duration");
        digitalWrite(RELAY_PIN, LOW);
        relayOn = false;
    } else if (soilMoisturePercent >= 55) {
        Serial.println("Turning relay OFF due to high moisture");
        digitalWrite(RELAY_PIN, LOW);
        relayOn = false;
    }
}

void addPlant() {
    Serial.println("Enter the plant name:");
    while (!Serial.available());
    String plantName = Serial.readStringUntil('\n');

    Serial.println("Enter the plant type:");
    while (!Serial.available());
    String plantType = Serial.readStringUntil('\n');

    if (WiFi.status() == WL_CONNECTED) {
        String postData = "plant_name=" + plantName + "&plant_type=" + plantType + "&category=plant";

        http.beginRequest();
        http.post(postPath);
        http.sendHeader("Content-Type", "application/x-www-form-urlencoded");
        http.sendHeader("Content-Length", postData.length());
        http.beginBody();
        http.print(postData);
        http.endRequest();

        int httpResponseCode = http.responseStatusCode();
        String response = http.responseBody();

        Serial.println(httpResponseCode);
        Serial.println(response);
    } else {
        Serial.println("Wi-Fi Disconnected");
    }
}
