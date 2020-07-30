#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ArduinoJson.h>


ESP8266WiFiMulti WiFiMulti;

// Set this section
const char* ssid = "";
const char* pass = "";

// Spotify authentication
String refresh = "";
String auth = "";

//Constants
const int button = D3;

// Variables
String token = "";

void setup() {
  pinMode(button,INPUT_PULLUP);
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(ssid, pass);

  while (WiFiMulti.run() != WL_CONNECTED){
    Serial.print(".");
    delay(500);
  }
}

void loop() {

  if (!digitalRead(button)) {
      token = getNewToken();
      String id = getTrackId();
      saveTrack(id);
  }
  

}

String getTrackId(){
  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  client->setInsecure();
  HTTPClient https;

  if (https.begin(*client, "https://api.spotify.com/v1/me/player/currently-playing")) {
    https.addHeader("Authorization","Bearer " + token);
    int httpCode = https.GET();
    if (httpCode > 0) {
      Serial.printf("Track request code: %d\n", httpCode);
      // Successful
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = https.getString();
        int index = payload.lastIndexOf("\"id\" : \"") + 8;
        String id;
        for (int i = 0; i < 22; i++)
          id += payload[index+i];
        Serial.println(id);
        https.end();
        return id;
      } else if (httpCode == HTTP_CODE_NO_CONTENT) {
        Serial.print("There aren't any track playing\n");
      }
    } else {
      Serial.printf("Track request failed, error: %s\n", https.errorToString(httpCode).c_str());

    }
    https.end();
  } else {
    Serial.print("Unable to connect with track endpoint\n");

  }
}

bool saveTrack(String id){
  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  client->setInsecure();
  HTTPClient https;

  if (https.begin(*client, "https://api.spotify.com/v1/me/tracks/?ids=" + id)) {
    https.addHeader("Authorization","Bearer " + token);
    int httpCode = https.PUT("");
    if (httpCode > 0) {
      Serial.printf("Save request code: %d\n", httpCode);
      String payload = https.getString();
      Serial.print(payload);
      // Successful
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        Serial.print("Hurray");
        https.end();
        return false;
      }
    } else {
      Serial.printf("Save request failed, error: %s\n", https.errorToString(httpCode).c_str());
      return true;
    }
    https.end();
  } else {
    Serial.print("Unable to connect with save endpoint\n");
    return true;
  }
}


String getNewToken(){
  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  client->setInsecure();
  HTTPClient https;

  if (https.begin(*client, "https://accounts.spotify.com/api/token")) {
    https.addHeader("Authorization","Basic " + auth); // +
    https.addHeader("Content-Type","application/x-www-form-urlencoded");
    int httpCode = https.POST("grant_type=refresh_token&refresh_token=" + refresh);
    if (httpCode > 0) {
      Serial.printf("Refresh token request code: %d\n", httpCode);
      String payload = https.getString();
      // Successful
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        Serial.print("Hurray");
        https.end();
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, payload);
        return doc["access_token"];
      }
    } else {
      Serial.printf("Refresh token request failed, error: %s\n", https.errorToString(httpCode).c_str());
    }
    https.end();
  } else {
    Serial.print("Unable to connect with refresh token endpoint\n");
  }
}
