#include "FastLED.h"
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>

#ifndef STASSID
#define STASSID "Home_Net1"
#define STAPSK  "timeckuchar1973"
#endif

#define LED_COUNT 9
#define LED_PIN 2
#define COLOR_ORDER GRB 

const char *ssid = STASSID;
const char *password = STAPSK;

ESP8266WebServer server(80);

CRGBArray<LED_COUNT> leds;


void handleRoot() {

  char temp[400];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

  snprintf(temp, 400,

           "<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>ESP8266 Demo</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Hello from ESP8266!</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
    <img src=\"/test.svg\" />\
  </body>\
</html>",

           hr, min % 60, sec % 60
          );
  server.send(200, "text/html", temp);

}

void showFrame(){

  StaticJsonDocument<300> JSONData;
  String jsonString = server.arg("plain");
  DeserializationError error = deserializeJson(JSONData, jsonString);

  if (error) {
    server.send(500, "text/plain", "Internal Server Error");
    return;
  }

  JsonArray arr = JSONData["params"].as<JsonArray>();

  if(arr.size() != LED_COUNT){
    server.send(400, "application/json", "Bad Request");
    return;
  }

  for (size_t i = 0; i < LED_COUNT; i++)
  {
    leds[i] = CRGB(arr[i][0].as<uint8_t>(), arr[i][1].as<uint8_t>(), arr[i][2].as<uint8_t>());
    FastLED.show();
  }
  
  server.send(200, "application/json", "OK");

}

void clearFrame(){
  FastLED.clear(1);

  server.send(200, "application/json", "OK");
  FastLED.show();
}

void storeFrame(){
  StaticJsonDocument<300> JSONData;
  String jsonString = server.arg("plain");
  DeserializationError error = deserializeJson(JSONData, jsonString);

  if (error) {
    server.send(500, "text/plain", "Internal Server Error");
    return;
  }

  JsonArray arr = JSONData["params"].as<JsonArray>();
  int pos = JSONData["position"].as<int>();

  if(arr.size() != LED_COUNT){
    server.send(400, "text/plain", "Bad Request");
    return;
  }

  if(pos < 0 || pos > 5){
    server.send(400, "text/plain", "Bad Request");
    return;
  }

  
  for (size_t i = 0; i < LED_COUNT; i++)
  {
    int address = i*sizeof(CRGB)+pos*sizeof(leds);
    EEPROM.put(address, CRGB(arr[i][0].as<uint8_t>(), arr[i][1].as<uint8_t>(), arr[i][2].as<uint8_t>()));
  }

  EEPROM.commit();

  server.send(200, "text/plain", "OK");
}

void showStored(){
  StaticJsonDocument<300> JSONData;
  String jsonString = server.arg("plain");
  DeserializationError error = deserializeJson(JSONData, jsonString);

  if (error) {
    server.send(500, "text/plain", "Internal Server Error");
    return;
  }

  int pos = JSONData["position"].as<int>();

  if(pos < 0 || pos > 5){
    server.send(400, "text/plain", "Bad Request");
    return;
  }
  
  for (size_t i = 0; i < LED_COUNT; i++)
  {
    int address = i*sizeof(CRGB)+pos*sizeof(leds);
    EEPROM.get(address, leds[i]);
    FastLED.show();
  }
  server.send(200, "text/plain", "OK");
}

void handleNotFound() {

  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);

}


void setup(void) {

  Serial.begin(115200);
  EEPROM.begin(512);

  FastLED.addLeds<WS2812, LED_PIN, COLOR_ORDER>(leds, LED_COUNT);
  FastLED.setBrightness(127);
  for (size_t i = 0; i < LED_COUNT; i++)
  {
    leds[i] = CRGB::Blue;
    FastLED.show();
    delay(100);
  }

  FastLED.clear();
  FastLED.show();


  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/frame/show", HTTP_POST, showFrame);
  server.on("/frame/clear", HTTP_POST, clearFrame);
  server.on("/frame/store", HTTP_POST, storeFrame);
  server.on("/frame/show/stored", HTTP_POST, showStored);

  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();
  MDNS.update();
}
