#include <WiFi.h>
#include <HTTPClient.h> 
#include "DHT.h"

//CREDENTIALS 
#define WIFI_SSID "Dialog 4G 335"
#define WIFI_PASSWORD "F94818d7"

//FIREBASE 
#define API_KEY "AIzaSyBS3l2f5IWCTOFOrohgtR2fE_awAINNf7E"                
#define DATABASE_URL "https://final-project-285b6-default-rtdb.asia-southeast1.firebasedatabase.app"
#define USER_EMAIL "dulshan.prasanna227z@gmail.com"
#define USER_PASSWORD "Fire@176"

//PINS 
#define PIN_DHT 27
#define PIN_FLAME 5
#define PIN_MQ6 34   
#define PIN_LED 18
#define PIN_BUZZER 19

//SETTINGS 
int GAS_THRESHOLD = 2100; 
#define DHTTYPE DHT11 

//TIMERS
// (30 minutes in milliseconds = 30 * 60 * 1000 = 1,800,000)
const unsigned long TEMP_ALARM_DELAY = 1000; 

DHT dht(PIN_DHT, DHTTYPE);

unsigned long lastUpload = 0;
unsigned long tempHighStartTime = 0; // Stores when the high temp started
bool isTempTimerRunning = false;     // Tracks if we are currently counting down
String lastStatus = "";

void setup() {
  Serial.begin(115200);
  
  pinMode(PIN_FLAME, INPUT);
  pinMode(PIN_MQ6, INPUT); 
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  dht.begin();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("."); delay(300);
  }
  Serial.println(" Connected!");
}

void sendToFirebase(String path, String jsonPayload) {
  if(WiFi.status() == WL_CONNECTED){
    HTTPClient http;
    

    String url = String(DATABASE_URL) + "/" + path + ".json?auth=" + API_KEY; 
    
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
  
    int httpResponseCode = http.PUT(jsonPayload); 
    
    if (httpResponseCode > 0) {
        Serial.print("Data sent! Response: ");
        Serial.println(httpResponseCode); 
    } else {
        Serial.print("Error sending: ");
        Serial.println(httpResponseCode);
    }
    
    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }
}

void loop() {

  bool fire = (digitalRead(PIN_FLAME) == LOW); 
  int gasLevel = analogRead(PIN_MQ6); 
  bool gas = (gasLevel > GAS_THRESHOLD); 

  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (isnan(t)) t = 0.0;
  if (isnan(h)) h = 0.0;

  bool tempCritical = false;

  if (t > 45) {
   
    if (!isTempTimerRunning) {
      tempHighStartTime = millis();
      isTempTimerRunning = true;
      Serial.println("Temp High! Timer Started...");
    }
    

    if (millis() - tempHighStartTime >= TEMP_ALARM_DELAY) {
      tempCritical = true; 
  } else {
    // Temp dropped back to normal, reset timer
    isTempTimerRunning = false;
    tempCritical = false;
  }
  }


  String status = "SAFE";
  
  if (fire && gas) status = "CRITICAL DANGER 🚨";
  else if (fire) status = "FIRE DETECTED 🔥";
  else if (gas) status = "GAS LEAK ⛽";
  else if (tempCritical) status = "PROLONGED HIGH TEMP 🌡️"; 

  // ALARM CONTROL
  if (status != "SAFE") {
    digitalWrite(PIN_LED, HIGH);
    digitalWrite(PIN_BUZZER, HIGH);
  } else {
    digitalWrite(PIN_LED, LOW);
    digitalWrite(PIN_BUZZER, LOW);
  }

  bool statusChanged = (status != lastStatus);
  
  if (millis() - lastUpload > 3000 || statusChanged) {
    lastUpload = millis();
    
    Serial.print("Gas: "); Serial.print(gasLevel); 
    Serial.print(" | Temp: "); Serial.print(t);
    Serial.print(" | Humidity: "); Serial.print(h);
    Serial.print(" | Status: "); Serial.println(status);


    if (isTempTimerRunning) {
       Serial.print("High Temp Duration: ");
       Serial.print((millis() - tempHighStartTime)/1000);
       Serial.println(" seconds");
    }

    String json = "{";
    json += "\"temperature\":" + String(t) + ",";
    json += "\"humidity\":" + String(h) + ",";
    json += "\"gas_status\":\"" + String(status == "SAFE" ? "SAFE" : "DANGER") + "\"";
    json += "}";
    
    sendToFirebase("sensors", json);
    
    if (statusChanged) {
       String alertJson = "\"" + status + "\""; 
       sendToFirebase("status/alert", alertJson);
       lastStatus = status;
    }
  }
  delay(100);
}

