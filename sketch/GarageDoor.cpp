#include <ArduinoJson.h>              // v5.13.2 - https://github.com/bblanchon/ArduinoJson
#include <WebSocketsServer.h>         // v2.1.4 - https://github.com/Links2004/arduinoWebSockets
#include <Bounce2.h>                  // v2.53 https://github.com/thomasfredericks/Bounce2

#include "GarageDoor.h"

GarageDoor::GarageDoor() {

}

void GarageDoor::begin() {
  // set the contact relay to output
  pinMode(CONTACT_RELAY, OUTPUT);
  digitalWrite(CONTACT_RELAY, OFF_STATE);

  // attach the open reed switch interface
  openReedSwitch.attach(OPEN_REED_SWITCH, INPUT);
  openReedSwitch.interval(250);

  // attach the closed reed switch interface
  closedReedSwitch.attach(CLOSED_REED_SWITCH, INPUT);
  closedReedSwitch.interval(250);

  // attach the open reed switch interface
  openReedSwitch.attach(OPEN_REED_SWITCH, INPUT);
  openReedSwitch.interval(250);

  // start the web socket
  webSocket.begin();
  webSocket.onEvent(std::bind(&GarageDoor::webSocketEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

  // get the initial reed switch state
  if ( openReedSwitch.isPressed() ) {
    this->currentDoorState = "OPEN";
    this->targetDoorState = "OPEN";
  } else if (closedReedSwitch.isPressed() ) {
    this->currentDoorState = "CLOSED";
    this->targetDoorState = "CLOSED";
  } else {
    this->currentDoorState = "OPENING";
    this->targetDoorState = "OPEN";
  }
}

void GarageDoor::loop () {
  webSocket.loop();

  // Update the Bounce instances
  openReedSwitch.update();
  closedReedSwitch.update();

  if ( openReedSwitch.released() ) {
    this->currentDoorState = "CLOSING";
    this->targetDoorState = "CLOSED";
    this->broadcastSystemStatus();
    Serial.println("[OPEN SWITCH] DOOR CLOSING");
  }

  if ( openReedSwitch.pressed() ) {
    this->currentDoorState = "OPEN";
    this->targetDoorState = "OPEN";
    this->broadcastSystemStatus();
    Serial.println("[OPEN SWITCH] DOOR FULLY OPEN");
  }

  if ( closedReedSwitch.released() ) {
    this->currentDoorState = "OPENING";
    this->targetDoorState = "OPEN";
    this->broadcastSystemStatus();
    Serial.println("[CLOSED SWITCH] DOOR OPENING");
  }

  if ( closedReedSwitch.pressed() ) {
    this->currentDoorState = "CLOSED";
    this->targetDoorState = "CLOSED";
    this->broadcastSystemStatus();
    Serial.println("[CLOSED SWITCH] DOOR FULLY CLOSED");
  }
}

void GarageDoor::webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\r\n", num);
      break;
    case WStype_CONNECTED: {
      IPAddress ip = webSocket.remoteIP(num);
      Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      // broadcast current settings
      this->broadcastSystemStatus();
      break;
    }
    case WStype_TEXT: {
      // send the payload to the ac handler
      this->processIncomingRequest((char *)&payload[0]);
      break;
    }
    case WStype_PING:
      // Serial.printf("[%u] Got Ping!\r\n", num);
      break;
    case WStype_PONG:
      // Serial.printf("[%u] Got Pong!\r\n", num);
      break;
    default:
      Serial.printf("Invalid WStype [%d]\r\n", type);
      break;
  }
}

void GarageDoor::processIncomingRequest(String payload) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& req = jsonBuffer.parseObject(payload);

  if ( req.containsKey("TargetDoorState") ) {
    unsigned long contactTime = req["contactTime"];

    if (this->currentDoorState == "OPENING" || this->currentDoorState == "CLOSING") {
      this->currentDoorState = "STOPPED";
      this->broadcastSystemStatus();
      this->triggerContactRelay(contactTime);
      return;
    }

    if ( this->currentDoorState == "STOPPED" ) {
      if ( this->targetDoorState == "OPEN" ) {
        this->targetDoorState = "CLOSED";
        this->currentDoorState = "CLOSING";
      } else {
        this->targetDoorState = "OPEN";
        this->currentDoorState = "OPENING";
      }
    } else if ( req["TargetDoorState"] == "OPEN" ) {
      this->targetDoorState = "OPEN";
      this->currentDoorState = "OPENING";
    } else if ( req["TargetDoorState"] == "CLOSED" ) {
      this->targetDoorState = "CLOSED";
      this->currentDoorState = "CLOSING";
    }

    this->broadcastSystemStatus();
    this->triggerContactRelay(contactTime);
  }
}

void GarageDoor::triggerContactRelay(unsigned long contactTime) {
  Serial.print("Triggering Contact Relay - ");
  Serial.println(this->targetDoorState);
  digitalWrite(CONTACT_RELAY, ON_STATE);
  delay(contactTime);
  digitalWrite(CONTACT_RELAY, OFF_STATE);
}

// broadcasts the status for everything
void GarageDoor::broadcastSystemStatus() {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& res = jsonBuffer.createObject();

  res["CurrentDoorState"] = this->currentDoorState;
  res["TargetDoorState"] = this->targetDoorState;
  res["ObstructionDetected"] = this->obstructionDetected || false;

  String payload;
  res.printTo(payload);
  webSocket.broadcastTXT(payload);
}


