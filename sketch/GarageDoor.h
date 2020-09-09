#ifndef GarageDoor_h
#define GarageDoor_h

#include <EEPROM.h>
#include <ArduinoJson.h>              // v5.13.2 - https://github.com/bblanchon/ArduinoJson
#include <WebSocketsServer.h>         // v2.2.0 - https://github.com/Links2004/arduinoWebSockets
#include <Bounce2.h>                  // v2.53 - https://github.com/thomasfredericks/Bounce2

#include "settings.h"
#include "auth.h"

class GarageDoor {
  public:
    WebSocketsServer webSocket = WebSocketsServer(81);

    Button openReedSwitch = Button();
    Button closedReedSwitch = Button();
    Button obstructionDetectedSwitch = Button();

    GarageDoor(void);

    char* targetDoorState;  // OPEN | CLOSED
    char* currentDoorState; // OPEN | CLOSED | OPENING | CLOSING | STOPPED
    bool obstructionDetected = false;

    void begin();
    void loop ();
    void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
    bool validateHttpHeader(String headerName, String headerValue);

    void processIncomingRequest(String payload);
    void triggerContactRelay(unsigned long contactTime);
    void broadcastSystemStatus(); 
};

#endif