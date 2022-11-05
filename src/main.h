#ifndef __MAIN_H__
 #define __MAIN_H__

// The debug symbol controls the mDNS name and other serial outputs
#define _DEBUG_

// Generally, you should use "unsigned long" 32 bits, for variables that hold time
// The value will quickly become too large for an int to store
uint32_t previousMillis = 0;    // will store last time DHT was updated

// Updates readings every 10 seconds
const uint32_t interval = 10000;

#include <Arduino.h>


void initTime( void );
std::string getDateTimeString();
void setTimezone( String timezone );

// main.cpp prototypes
void sendTelemetry( void );
void preModeChange( void );
void sendDelayStatus( bool status );
void sendCurrentMode( void );

void configureRoutes( void );
void startWiFi( void );

// websocket server
void initWebSocket();
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void notifyClients( std::string data );

#endif