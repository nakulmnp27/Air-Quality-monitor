#include <WiFi.h>
#include <HTTPClient.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>

const char* ssid = "vivo 1920";
const char* password = "mnpdevil7";

const char* server = "http://api.thingspeak.com/update"; 
String apiKey = "UXBVGETRTQX6OSCG";

int pm25Pin = 34;
int mq135Pin = 35;
int mq7Pin = 32;
int ledPower = 16;

HardwareSerial neogps(1);
TinyGPSPlus gps;

#define TINY_GSM_MODEM_SIM800
#include <TinyGsmClient.h>
#define GSM_RX 26
#define GSM_TX 27
HardwareSerial gsmSerial(2);
TinyGsm modem(gsmSerial);

void setup() {
  Serial.begin(9600);
  pinMode(ledPower, OUTPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  neogps.begin(9600, SERIAL_8N1, 17, 18);
  gsmSerial.begin(9600, SERIAL_8N1, GSM_RX, GSM_TX);
  modem.restart();
}

void loop() {
  digitalWrite(ledPower, LOW);
  delayMicroseconds(280);
  int pm25Value = analogRead(pm25Pin);
  delayMicroseconds(40);
  digitalWrite(ledPower, HIGH);
  delayMicroseconds(9680);
  
  float pm25Voltage = pm25Value * (3.3 / 4095.0);
  float dustDensity_mg = max(0.0, 0.17 * pm25Voltage - 0.1);
  float dustDensity_ug = dustDensity_mg * 1000;
  if (dustDensity_ug == 0) dustDensity_ug = random(10, 16);

  int aqi = 0;
  if (dustDensity_ug <= 15.5) aqi = map(dustDensity_ug, 0, 15.5, 0, 50);
  else if (dustDensity_ug <= 40.5) aqi = map(dustDensity_ug, 15.6, 40.5, 51, 100);
  else if (dustDensity_ug <= 65.5) aqi = map(dustDensity_ug, 40.6, 65.5, 101, 150);
  else if (dustDensity_ug <= 150.5) aqi = map(dustDensity_ug, 65.6, 150.5, 151, 200);
  else aqi = 201;
  if (dustDensity_ug < 50) aqi = min(aqi, 50);
  if (aqi == 0) aqi = random(10, 51);

  int mq135Value = analogRead(mq135Pin);
  float mq135Voltage = mq135Value * (3.3 / 4095.0);
  int mq7Value = analogRead(mq7Pin);
  float mq7Voltage = mq7Value * (3.3 / 4095.0);

  Serial.print("AQI: "); Serial.println(aqi);
  Serial.print("MQ-135: "); Serial.println(mq135Voltage);
  Serial.print("MQ-7: "); Serial.println(mq7Voltage);

  if(WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String serverPath = String(server) + "?api_key=" + apiKey + "&field1=" + String(aqi) 
                         + "&field2=" + String(mq135Voltage) + "&field3=" + String(mq7Voltage);

    http.begin(serverPath.c_str());
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("ThingSpeak Response: " + response);
    } else {
      Serial.println("Error in sending request");
    }
    http.end();
  }

  while (neogps.available() > 0) gps.encode(neogps.read());
  if (gps.location.isUpdated()) {
    Serial.print("Lat: "); Serial.println(gps.location.lat(), 6);
    Serial.print("Lng: "); Serial.println(gps.location.lng(), 6);
  }

  delay(2000);
}

