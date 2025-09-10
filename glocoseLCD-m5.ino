// Glucose monitor - m5stickC version
#include <M5Unified.h>
 // add https://static-cdn.m5stack.com/resource/arduino/package_m5stack_index.json to arduino board manager
 // m5unified in libraries

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include "mbedtls/sha256.h"

const char* ssid = "wifi";
const char* pass = "password";
String apiBaseUrl = "https://api.libreview.io";
//libreview credentials
String username="email";
String password="password!";

String timestamp;
String value;
short patientIndex;
String token = "";
String sha256AccountId;
String patientId = "";
JsonArray graphData;
DynamicJsonDocument graphDoc(20480);

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
    DeserializationError error = deserializeJson(graphDoc, payload);
    value = (String) graphDoc["data"]["connection"]["glucoseMeasurement"]["Value"];
    timestamp = (String) graphDoc["data"]["connection"]["glucoseMeasurement"]["Timestamp"];

    graphData = graphDoc["data"]["graphData"]; 
  

    Serial.println(value);
  } else {
    Serial.println("ERROR");
    Serial.println(httpResponseCode);
  }
  http.end();
  return true;
}

void setup(void) {
  Serial.begin(115200);

  M5.begin();
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Display.setCursor(0, 0);
  M5.Display.setRotation(3);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) delay(500); 
}

void loop() {
  M5.Display.clearDisplay();

  auth();
  fetchData();

   M5.Display.setCursor(0, 0);
   M5.Display.setTextSize(1);
  int space = timestamp.indexOf(" ");
  if (space) timestamp = timestamp.substring(space,-1);
   M5.Display.println("Glucose " + timestamp);
  
   M5.Display.setCursor(20, 24);
   M5.Display.setTextSize(4);
   M5.Display.println(value);

   M5.Display.setCursor(80,55);
   M5.Display.setTextSize(1);
   M5.Display.println("mmol/L");
  
  sleep(60);
}
