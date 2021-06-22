#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>

#include <WiFiManager.h>       // circa September 2019 development branch - https://github.com/tzapu/WiFiManager.git

#include "auth.h"
#include "settings.h"
#include "GarageDoor.h"
#include "html.h"

GarageDoor garageDoor;
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

// parameters
char device_name[40];
char hostname[18];
bool resetRequired = false;
unsigned long loopLastRun;

const char* update_path = "/firmware";
const char* update_username = AUTH_USERNAME;
const char* update_password = AUTH_PASSWORD;

void saveConfigCallback() {
  Serial.println("Resetting device...");
  delay(5000);
  resetRequired = true;
}

void setup(void) {
  pinMode(LED_BUILTIN, OUTPUT);

  // turn LED on at boot
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
  WiFi.mode(WIFI_STA);

  delay(1000);

  Serial.println("Starting...");

  // WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;

  // setup hostname
  String id = WiFi.macAddress();
  id.replace(":", "");
  id.toLowerCase();
  id = id.substring(6,12);
  id = "garagedoor-" + id;
  id.toCharArray(hostname, 18);

  WiFi.hostname(hostname);
  Serial.println(hostname);

  // reset the device after config is saved
  wm.setSaveConfigCallback(saveConfigCallback);

  // sets timeout until configuration portal gets turned off
  wm.setConfigPortalTimeout(120);

  // connect time out
  wm.setConnectTimeout(1200);

  // connect retry count
  wm.setConnectRetries(30);

  // first parameter is name of access point, second is the password
  if (!wm.autoConnect(hostname, AUTH_PASSWORD))
  {
    Serial.println("Failed to connect and hit timeout");
    delay(3000);

    // reset and try again
    ESP.reset();
    delay(5000);
  }

  WiFi.hostname(hostname);

  // reset if flagged
  if (resetRequired) {
    ESP.reset();
  }

  // Add service to MDNS-sd
  delay(2000);

  if (MDNS.begin(hostname, WiFi.localIP())) {
    Serial.println("MDNS responder started");
  }

  MDNS.addService("oznu-platform", "tcp", 81);
  MDNS.addServiceTxt("oznu-platform", "tcp", "type", "garage-door");
  MDNS.addServiceTxt("oznu-platform", "tcp", "mac", WiFi.macAddress());
  // MDNS end

  // garage door start
  garageDoor.begin();

  // start http update server
  httpUpdater.setup(&httpServer, update_path, update_username, update_password);

  httpServer.on("/", []() {
    if (!httpServer.authenticate(update_username, update_password)) {
      return httpServer.requestAuthentication();
    }
    String s = MAIN_page;
    httpServer.send(200, "text/html", s);
  });

  httpServer.begin();

  // turn LED off once ready
  digitalWrite(LED_BUILTIN, HIGH);     
}

void loop (void) {
  httpServer.handleClient();
  garageDoor.loop();
  MDNS.update();
}
