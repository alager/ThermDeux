#include <Arduino.h>
#include <time.h>
#include <Streaming.h>
#include <FS.h>
#include <WiFi.h>
#include <AsyncTCP.h>

#include <main.h>


const int ledPin = 2;


const char* ssid     = "NestRouter1";
const char* password = "This_isapassword9";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;



void setup() {
	struct tm timeinfo;

	Serial.begin( 460800 );

	// setup pin 5 as a digital output pin
	pinMode (ledPin, OUTPUT);

	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		Serial << (".");
	}
	Serial << endl;
	Serial << ("WiFi connected.") << endl;
	Serial << ("Wifi RSSI=") << WiFi.RSSI() << endl;
	Serial << ( WiFi.localIP() ) << endl;
	Serial << "chip ID: 0x";
	// Serial.println( ESP.getChipId(), HEX); // this won't print correctly using the stdout notation

	uint32_t chipId = 0;
	for(int i=0; i<17; i=i+8) 
	{
		chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
	}

	Serial.printf("ESP32 Chip model = %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
	Serial.printf("This chip has %d cores\n", ESP.getChipCores());
	Serial << ("Chip ID: ");
	Serial.println( chipId, HEX);




	// Init and get the time
	configTime( 0, 0, ntpServer);

	// setenv( "TZ", "America/Chicago", 1 );
	// tzset();

	if( !getLocalTime( &timeinfo ) )
	{
		Serial.println( "Failed to obtain time" );
		return;
	}

	setTimezone( "CST6CDT,M3.2.0,M11.1.0" );
	Serial << getDateTimeString().c_str() << endl;

}

void loop() {
  // put your main code here, to run repeatedly:

	Serial << ( "high") << endl;
	digitalWrite (ledPin, HIGH);	// turn on the LED
	delay(1500);	// wait for half a second or 500 milliseconds

	Serial << ( "low") << endl;

	digitalWrite (ledPin, LOW);	// turn off the LED
	delay(1500);	// wait for half a second or 500 milliseconds

	Serial << getDateTimeString().c_str() << endl;
}


void setTimezone(String timezone)
{
	Serial.printf( "  Setting Timezone to %s\n",timezone.c_str() );
	setenv( "TZ",timezone.c_str(), 1 );  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
	tzset();
}

std::string getDateTimeString()
{
	struct tm timeinfo;

	if( !getLocalTime( &timeinfo ) )
	{
		Serial.println( "Failed to obtain time" );
		return( "Failed to obtain time" );
	}

	Serial.println( &timeinfo, "%A, %B %d %Y %H:%M:%S" );
	
	//50 chars should be enough
	char timeStringBuff[50];
	strftime( timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo );

	std::string foo( timeStringBuff );
	return foo;

//   Serial.print("Day of week: ");
//   Serial.println(&timeinfo, "%A");
//   Serial.print("Month: ");
//   Serial.println(&timeinfo, "%B");
//   Serial.print("Day of Month: ");
//   Serial.println(&timeinfo, "%d");
//   Serial.print("Year: ");
//   Serial.println(&timeinfo, "%Y");
//   Serial.print("Hour: ");
//   Serial.println(&timeinfo, "%H");
//   Serial.print("Hour (12 hour format): ");
//   Serial.println(&timeinfo, "%I");
//   Serial.print("Minute: ");
//   Serial.println(&timeinfo, "%M");
//   Serial.print("Second: ");
//   Serial.println(&timeinfo, "%S");

//   Serial.println("Time variables");
//   char timeHour[3];
//   strftime(timeHour,3, "%H", &timeinfo);
//   Serial.println(timeHour);
//   char timeWeekDay[10];
//   strftime(timeWeekDay,10, "%A", &timeinfo);
//   Serial.println(timeWeekDay);
//   Serial.println();
}