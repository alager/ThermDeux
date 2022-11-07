#include <Arduino.h>
#include <Streaming.h>
#include <FS.h>
#include <LittleFS.h>

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
// #include <WebSocketsClient.h>

#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#include <myThermostat.h>
#include <main.h>


// global variables
MyThermostat *someTherm;

// our wifi
const char* ssid     = "NestRouter1";
const char* password = "This_isapassword9";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// create a web socket object
AsyncWebSocket webSock("/ws");

// let us know if getting NTP time worked
// bool weHaveTime = false;


void setup() 
{
	// configure super fast serial port
	Serial.begin( 460800 );
	// Serial.begin( 115200 );

	Serial << ( F( "CPU Freq: ")) << getCpuFrequencyMhz() << endl;

	// MyThermostat someThermObj;
	someTherm = new MyThermostat;

	// init the object for first run
	someTherm->init();

	// debug, set the mode to cooling
	#ifdef _DEBUG_
	  Serial << ( F( "isMode: " )) << someTherm->getMode() << endl;
	#endif

	// Initialize LittleFS
	if(!LittleFS.begin())
	{
		Serial << ( F( "LittleFS Mounting Error" )) << endl;
		return;
	}
	Serial << ( F( "LittleFS Mounted") ) << endl;
	
	startWiFi();

	Serial.printf("ESP32 Chip model = %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
	Serial.printf("This chip has %d cores\n", ESP.getChipCores());

	// initTime();

	// after the network is up, we can init the scheduler
	// it needs networking for NTP first
	someTherm->sched_init();
	

	// configure web server routes
	configureRoutes();

	// configure web server web socket
	initWebSocket();

	// Start Elegant OTA
	AsyncElegantOTA.begin( &server );
	// AsyncElegantOTA.begin(&server, "username", "password");

	// deal with CORS access
	// DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
	// DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "content-type");

	// Start web server
	server.begin();

} // end setup()


// websocket server events
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
			 void *arg, uint8_t *data, size_t len)
{
	switch (type)
	{
	case WS_EVT_CONNECT:
		Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
		
		// if( MAC_OUTSIDE == ESP.getChipId() )
		// {
		// 	// wsConnected = true;
		// }

		sendTelemetry();
		break;
	case WS_EVT_DISCONNECT:
		Serial.printf("WebSocket client #%u disconnected\n", client->id());
		// if( MAC_OUTSIDE == ESP.getChipId() )
		// {
		// 	// wsConnected = false;
		// }
		break;
	case WS_EVT_DATA:
		handleWebSocketMessage(arg, data, len);
		break;
	case WS_EVT_PONG:
	case WS_EVT_ERROR:
		break;
	}
}

// websocket server
void notifyClients( std::string data )
{
	webSock.textAll( (char *)data.c_str() );
}


// a message from the websocket has arrived
// so process it.
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
	std::string replyStr;

	Serial << ( F( "Got Data from WebSocket" ));
	data[len] = 0;
	Serial.println( (char *)data );

	AwsFrameInfo *info = (AwsFrameInfo *)arg;
	if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
	{
		data[len] = 0;
		if (strcmp((char *)data, "temperatureUp") == 0)
		{
			float theTemp = someTherm->getTemperatureSetting();
			if( theTemp == 0 || theTemp == -1 || isnan( theTemp ) )
				theTemp = 65.0f;

			theTemp += 0.5f;
			someTherm->setTemperatureSetting( theTemp );
			
			// Serial << F( "UP theTemp: " ) << theTemp << mendl;

			replyStr = std::to_string( theTemp );
			// send the new temperature setting to the websocket clients
			notifyClients( "{\"tempSet\":" + replyStr + "}" );
		}
		else
		if (strcmp((char *)data, "temperatureDown") == 0)
		{
			float theTemp = someTherm->getTemperatureSetting();
			if( theTemp == 0 || theTemp == -1 || isnan( theTemp ) )
				theTemp = 65.0f;

			theTemp -= 0.5f;
			someTherm->setTemperatureSetting( theTemp );

			replyStr = std::to_string( theTemp );
			// send the new temperature setting to the websocket clients
			notifyClients( "{\"tempSet\":" + replyStr + "}" );
		}
		else
		if (strcmp((char *)data, "modeClick") == 0)
		{
			mode_e mode = someTherm->getMode();

			// cycle through the modes of operation
			if( mode == MODE_HEATING )
				mode = MODE_OFF;
			else
				mode = (mode_e)(mode + 1);

			// prep for a mode change
			preModeChange();

			someTherm->setMode( mode );

			// send the new temperature setting to the websocket clients
			replyStr = std::to_string( mode );
			notifyClients( "{\"modeSet\":" + replyStr + "}" );

			sendTelemetry();
		}
		else  
		if (strcmp((char *)data, "sendSettings") == 0)
		{
			// send the settings to the websockets
			std::string settingsStr;

			// StaticJsonObject allocates memory on the stack
			StaticJsonDocument<200> doc;
			JsonObject settings  =	doc.createNestedObject("settings");
			settings[ "fanDelay" ] = 			 someTherm->settings_getFanDelay();
			settings[ "compressorOffDelay" ]  =	 someTherm->settings_getCompressorOffDelay();
			settings[ "compressorMaxRuntime" ] = someTherm->settings_getCompressorMaxRuntime();
			settings[ "timeZone" ] =			 someTherm->timeZone_get();
		
			//serializeJsonPretty( doc, Serial );

			// put it into a buffer to send to the clients
			serializeJson( doc, settingsStr );

			// send it to the clients
			notifyClients( settingsStr );
		}
		else
		{
			Serial << ( F("trying it as JSON")) << endl;

			// try to parse as JSON
			StaticJsonDocument<200> json;
			DeserializationError err = deserializeJson(json, data);
			if (err)
			{
				Serial << (F("deserializeJson() failed with code ")) << err.c_str() << endl;
				return;
			}

			JsonObject fan = json["fanClick"];
			
			if( !fan.isNull() )
			{
				std::string JSONRetStr;

				bool off = fan[ "off" ];
				if( off )
				{
					// we got an off command, so clear the fan timer
					// then notify the UI to update the fan
					Serial << F( "FAN: Off" ) << endl;

					// turn off the fan in a safe way in case heat/cooling is running
					someTherm->turnOffFan();

					Serial << F( "fanTime: " ) << someTherm->getFanRunTime() << endl;

					
					// put it into a buffer to send to the clients
					serializeJson( json, JSONRetStr );

					// send it to the clients
					notifyClients( JSONRetStr );
					sendTelemetry();
					return;
				}

				bool add15minutes = fan[ "add15minutes" ];
				// serializeJsonPretty( add15minutes, Serial );
				// Serial << "add15minutes: " << add15minutes << mendl;

				if( add15minutes )
				{
					// we got an add command, so add to the fan timer (and turn it on if applicable)
					// then notify the UI to update the fan
					Serial << "FAN: add 15 minutes" << endl;
					
					// only turn on the fan if it is off right now
					if( MODE_OFF == someTherm->currentState() )
					{
						someTherm->turnOnFan();
						someTherm->setFanRunTime( (60 * 15) + someTherm->getFanRunTime() );

						// Serial << "fanTime: " << someTherm->getFanRunTime() << mendl;

						// put it into a buffer to send to the clients
						serializeJson( json, JSONRetStr );

						// send it to the clients
						notifyClients( JSONRetStr );
					}
					sendTelemetry();
					return;
				}
			}
			
			JsonObject settings = json["settings"];
			if( !settings.isNull() )
			 {
				
				someTherm->settings_setFanDelay( settings[ "fanDelay" ] );
				someTherm->settings_setCompressorOffDelay( settings[ "compressorOffDelay" ] );
				someTherm->settings_setCompressorMaxRuntime( settings[ "compressorMaxRuntime" ] );
				
				uint16_t newTz = settings[ "timeZone" ];

				// store the new timezone value
				someTherm->timeZone_set( (timezone_e)newTz );

				// debug console output
				Serial << F("json value: ") << newTz << endl;
				// Serial << F("Setting new time zone to: ") << someTherm->mySched.timeZone[ someTherm->mySched.tz ].c_str() << mendl;

				// the timezone possible just changed, so update the ezTime object
				// someTherm->mySched.myTZ.setLocation( someTherm->mySched.timeZone[ someTherm->mySched.tz ].c_str() );

				// myTZ.setLocation( F("America/Chicago") );
				// waitForSync();

				// Serial << F("Setting new time zone DONE") << mendl;
				return;
			 }
		}
	}
}


// websocket server
void initWebSocket()
{
	// add a function pointer for the "on" event handler
	webSock.onEvent( onEvent );
	server.addHandler(&webSock);
}

void startWiFi( void )
{
	static uint32_t chipId = 0;

	if( !chipId )
	{
		for(int i=0; i<17; i=i+8) 
		{
			chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
		}
	}

	Serial << ( F( "MAC Address: " ));
	Serial.println( chipId, HEX );  // this won't print correctly using the stdout notation

	Serial << ( F( "WiFi STA." )) << endl;
 	WiFi.mode(WIFI_STA);

	Serial << ( F( "Connecting to WiFi." )) << endl;
	WiFi.begin( ssid, password );
	while( WiFi.status() != WL_CONNECTED )
	{
		delay(500);
		Serial << (F("."));
	}
	Serial << endl;
	Serial << (F( "WiFi connected." )) << endl;
	Serial << (F( "Wifi RSSI=" )) << WiFi.RSSI() << endl;
	Serial << ( WiFi.localIP() ) << endl;

}

void loop()
{
	unsigned long currentMillis = millis();
	if (currentMillis - previousMillis >= interval)
	{
		// update counter for the next interval
		previousMillis = currentMillis;

		// update our current readings
		someTherm->updateMeasurements();

		// run thermostat background logic and timers
		someTherm->runSlowTick();

		// update websocket data pipe
		sendTelemetry();

		// run the ezTime task
		// this will poll pool.ntp.org about every 30 minutes
		someTherm->loopTick();

		// check wifi status
		if( (WiFi.status() != WL_CONNECTED) )
		{
				// it's been 10s and we haven't connected, so shut wifi down
				// so we can start afresh
				WiFi.disconnect();
				delay( 5 );
				
				// start the wifi again
				startWiFi();
		}

		if( someTherm->isMode( MODE_COOLING ) )
		{
			// thermostat logic
			if( someTherm->getTemperature_f() > ( someTherm->getTemperatureSetting() + someTherm->getTempHysteresis() ) )
			{
				// turn on the cooler (if we can)
				if( someTherm->turnOnCooler() )
				{
					sendCurrentMode();
					sendDelayStatus( false );
				}

				// allow the extended fan run time happen again
				someTherm->clearFanRunOnce();
			}
			else
			if( someTherm->getTemperature_f() <= someTherm->getTemperatureSetting() - someTherm->getTempHysteresis() )
			{
				// turn off the cooler, but run fan for a little longer
				someTherm->turnOffCooler();
				sendCurrentMode();
				sendDelayStatus( false );
			}

		}
		else
		if( someTherm->isMode( MODE_HEATING ) )
		{
			if( someTherm->getTemperature_f() < ( someTherm->getTemperatureSetting() - someTherm->getTempHysteresis() ) )
			{
				// turn on the heater (if we can)
				if( someTherm->turnOnHeater() )
				{
					sendCurrentMode();
					sendDelayStatus( false );
				}

				// allow the extended fan run time happen again
				someTherm->clearFanRunOnce();
			}
			else
			if( someTherm->getTemperature_f() >= someTherm->getTemperatureSetting() + someTherm->getTempHysteresis() )
			{
				// turn off the heater, but run fan for a little longer
				someTherm->turnOffHeater();
				sendCurrentMode();
				sendDelayStatus( false );
			}
		}
		else
		{
			// off
			someTherm->turnOffAll();
			sendDelayStatus( false );

		}

		// only check to update the eeprom once per loop
		// the eeprom will only write to the flash if the 
		// datastructure cache has been changed
		someTherm->saveSettings();

		// clean up dangling websockets
		 webSock.cleanupClients();

		// sometimes getting time will fail, so try it again
		if( !someTherm->mySched.weHaveTime )
			someTherm->sched_init();

	} // end of 10s loop
}


void configureRoutes( void )
{
	// Route for root / web page
	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		request->send( LittleFS, F("/index.html"), F("text/html") );
	});

	// Route to load style.css file
	server.on("/mario.css", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		request->send(LittleFS, F("/mario.css"), F("text/css") );
	});

	// sen the sprites
	server.on("/Mario_3_sprites.jpg", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		request->send(LittleFS, F("/Mario_3_sprites.jpg"), F("image/jpeg") );
	});
	server.on("/thermSprites.jpg", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		request->send(LittleFS, F("/thermSprites.jpg"), F("image/jpeg") );
	});

	// send the fonts
	server.on("/marioFont.woff", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		request->send(LittleFS, F("/marioFont.woff"), F("font/woff") );
	});

	// deal with the favicon.ico
	server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		request->send(LittleFS, F("/favicon.ico"), F("image/x-image") );
	});

	// long press javascript
	server.on("/longClick.js", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		request->send(LittleFS, F("/longClick.js"), F("text/javascript") );
	});

	// deal with not found ( 404 )
	server.onNotFound( []( AsyncWebServerRequest *request )
	{
		request->send(404, "text/plain", F( "404: Not found") );
	});
}


void sendTelemetry( void )
{
	std::string telemetryStr;

	// StaticJsonObject allocates memory on the stack
	StaticJsonDocument<200> doc;

	JsonObject telemetry  =			doc.createNestedObject("telemetry");
	telemetry[ "mode" ] =			someTherm->getMode();
	telemetry[ "tempSetting" ] =	someTherm->getTemperatureSetting();
	telemetry[ "currentMode" ] =	someTherm->currentState();
	telemetry[ "tempAvg" ] =		someTherm->getTemperature_f();
	telemetry[ "humidAvg" ] =		someTherm->getHumidity_f();
	telemetry[ "presAvg" ] =		someTherm->getPressure_f();
	telemetry[ "time" ] =			someTherm->mySched.getDateTimeString();
	telemetry[ "delayTime" ] =		someTherm->getCompressorOffTime();
	
	if( MODE_OFF == someTherm->currentState() )
		telemetry[ "fanTime" ] =		someTherm->getFanRunTime();
	else
		telemetry[ "fanTime" ] = 0;

	// Generate the prettified JSON and send it to the Serial port.
	// serializeJsonPretty(doc, Serial);

	// put it into a buffer to send to the clients
	serializeJson( doc, telemetryStr );

	// send it to the clients
	notifyClients( telemetryStr );
	
}


// set the IO and additional items for a mode change
void preModeChange( void )
{
	// turn on the delay blinker.  
	// If going to off mode, then it'll set it to false on its own
	sendDelayStatus( true );

	// turn off all IO
	someTherm->turnOffAll();
}


// let the web socket know the delay status
// use simple preformated json string
void sendDelayStatus( bool status )
{
	if( status )
		notifyClients( "{\"delay\":true}" );
	else
		notifyClients( "{\"delay\":false}" );
}


void sendCurrentMode( void )
{
	std::string currentState;
	currentState = "{\"currentMode\":";
	currentState +=  std::to_string( someTherm->currentState() );
	currentState += "}";

	// Serial.println( currentState.c_str() );

	notifyClients( currentState );
}