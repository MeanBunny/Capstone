//#include <Wire.h>
//#include <LiquidCrystal_I2C.h>
#include "DHT.h"

#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>

#define DHTPIN D4
#define DHTTYPE DHT11
#define SOIL_MOISTURE_PIN A0
#define N_SENS_PIN A1
#define P_SENS_PIN A2
#define K_SENS_PIN A3

DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "GlobeAtHome_d7d38_5";
const char* password = "Jy6YEfHQ";
const char* serverName = "192.168.1.100";
const int port = 80;
const char* postPath = "/save_data.php";

WifiClient wifi;
HttpClient http(wifi, serverName, port);

void setup() {
  Serial.begin(9600);
  //lcd.init();
  //lcd.backlight();
  //Wire.begin();
  
  dht.begin();
  pinMode(DHTPIN, INPUT);
  pinMode(N_SENS_PIN, INPUT);
  pinMode(P_SENS_PIN, INPUT);
  pinMode(K_SENS_PIN, INPUT);
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
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  int soilMoistureValue = analogRead(SOIL_MOISTURE_PIN);
  int soilMoisturePercent = map(soilMoistureValue, 620, 215, 0, 100);

  if (isnan(h) || isnan(t)) {

    Serial.println("Failed to read from  DHT sensor!");
    return;

  }

  if (Wifi.status() == WL_Connected) {
    String postData = "category=environmental&humidity=" + String(h) + "&temperature=" + String(t);

    http.beginRequest();
    http.post(postPath);
    http.sendHeader("Contest-Type", "application/x-www-form-urlencoded");
    http.sendHeader("Content-Length", postData.Length());
    http.beginBody();
    http.print(postData);
    http.endRequest();

    int httpResponseCode = http.responseStatusCode();
    String response = http.responseBody();

    Serial.println(httpResponseCode);
    Serial.println(response);
  }
  else{
    Serial.println("Wifi Disconnected");
  }

  

  if (soilMoisturePercent < 0) soilMoisturePercent = 0;
  if (soilMoisturePercent > 100) soilMoisturePercent = 100;

  if (Firebase.ready() && signupOK) {
    
    if (Firebase.RTDB.setFloat(&fbdo, "DHT/humidity",h)){
      
      Serial.print("Humidity: ");
      Serial.print(h);
      Serial.println(" %");
    
    }
    else {

      Serial.println("FAILED");
      Serial.print("REASON:: " + fbdo.errorReason());
    
    }

    if (Firebase.RTDB.setFloat(&fbdo, "DHT/temperature", t)) {

      Serial.print("Temperature: ");
      Serial.print(t);
      Serial.println(" °C");

    }
    else {

      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    
    }

    if (Firebase.RTDB.setInt(&fbdo, "SoilMoisture/Percent", soilMoisturePercent)){

      Serial.print("Soil Moisture: ");
      Serial.print(soilMoisturePercent);
      Serial.println(" %");

    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }

  Serial.println("______________________________");

  unsigned long currentMillis = millis();

  Serial.print("Relay Status: ");
  Serial.println(relayOn ? "ON" 



}
