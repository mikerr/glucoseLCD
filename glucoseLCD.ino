// show glucose level on LCD display

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include "mbedtls/sha256.h"

const char* ssid     = "wifi";
const char* pass = "password";

String apiBaseUrl = "https://api.libreview.io";
String username="email";
String password="password";

Adafruit_SSD1306 display(128, 64, &Wire, -1);

void setup() {
  // Initialize the I2C communication
  Wire.begin(6,7); // Use GPIO 8 for SDA and GPIO 9 for SCL
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Check your display's I2C address
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();

  // Connect to Wi-Fi
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) delay(500); 

  Serial.begin(115200);
}

String value;
String trendArrow;
String name;
String percChange;

String timestamp;
String oldTimestamp;
short missingUpdates;

short patientIndex;
String token = "";
String sha256AccountId;
String patientId = "";

String sha256hex(const String& input) {
  byte hash[32];
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts(&ctx, 0); // 0 = SHA-256, 1 = SHA-224
  mbedtls_sha256_update(&ctx, (const unsigned char *)input.c_str(), input.length());
  mbedtls_sha256_finish(&ctx, hash);
  mbedtls_sha256_free(&ctx);

  String output = "";
  for (int i = 0; i < 32; i++) {
    if (hash[i] < 16) output += "0";
    output += String(hash[i], HEX);
  }
  return output;
}
bool auth() {
  HTTPClient http;
    http.begin(apiBaseUrl + "/llu/auth/login");
    http.addHeader("product", "llu.android");
    http.addHeader("version", "4.15.0");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Accept", "application/json");
    String postData = "{";
    postData += "\"email\":\"" + username + "\",";
    postData += "\"password\":\"" + password + "\"";
    postData += "}";
    int httpResponseCode = http.POST(postData);

    if (httpResponseCode > 0) {
      String authPayload = http.getString();
      Serial.println(authPayload);
      DynamicJsonDocument authDoc(20480);
      DeserializationError error = deserializeJson(authDoc, authPayload);
      if (error || authDoc["status"] != 0){
        //showInvalidParams();
      }else{
        String t = authDoc["data"]["authTicket"]["token"];
        String ai = authDoc["data"]["user"]["id"];
        http.end();
        http.begin(apiBaseUrl + "/llu/connections");
        http.addHeader("product", "llu.android");
        http.addHeader("version", "4.15.0");
        http.addHeader("Content-Type", "application/json");
        http.addHeader("Accept", "application/json");
        http.addHeader("Authorization", "Bearer " + t);
        http.addHeader("account-id", sha256hex(ai));
        int httpResponseCode = http.GET();
        authPayload = http.getString();

        error = deserializeJson(authDoc, authPayload);
        String pi = authDoc["data"][patientIndex]["patientId"];
        sha256AccountId=sha256hex(ai);
        patientId=pi;
        token = t;
      }
    } else {
      Serial.println("wrong password");
    }  
    http.end();
    return true;
}
bool fetchData() {
  Serial.println("fetchData");

  HTTPClient http;
  http.begin(apiBaseUrl + "/llu/connections/" + patientId + "/graph");
  http.addHeader("Authorization", "Bearer " + token);
  http.addHeader("product", "llu.android");
  http.addHeader("version", "4.15.0");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");
  http.addHeader("account-id", sha256AccountId);

  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    String payload = http.getString();
    Serial.println(payload);
    DynamicJsonDocument graphDoc(20480);
    DeserializationError error = deserializeJson(graphDoc, payload);
    value = (String) graphDoc["data"]["connection"]["glucoseMeasurement"]["Value"];
   
    Serial.println(value);
  } else {
    Serial.println("ERROR");
    Serial.println(httpResponseCode);
  }
  http.end();
  return true;
}

void loop() {

  display.clearDisplay();

  auth();
  fetchData();

  display.setCursor(20, 0);
  display.setTextSize(1);
  display.println("Glucose");
  
  display.setCursor(20, 24);
  display.setTextSize(4);
  display.println(value);

  display.setCursor(80,55);
  display.setTextSize(1);
  display.println("mmol/L");
  display.display();
  
  sleep(60);
}
