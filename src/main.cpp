#include <Arduino.h>
#include <Streaming.h>
#include <FS.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#include <myThermostat.h>
#include <main.h>


// global variables
MyThermostat *someTherm;

const char* ssid     = "NestRouter1";
const char* password = "This_isapassword9";

const char* ntpServer1 = "at.pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";


void setup() 
{
	uint32_t chipId = 0;

	// configure super fast serial port
	Serial.begin( 460800 );
	// Serial.begin( 115200 );

	Serial << "CPU Freq: " << getCpuFrequencyMhz() << endl;

	// MyThermostat someThermObj;
	someTherm = new MyThermostat;

	// init the object for first run
	someTherm->init();

	// debug, set the mode to cooling
	#ifdef _DEBUG_
	  Serial << ( F( "isMode: " )) << someTherm->getMode() << endl;
	#endif

	// Initialize SPIFFS
	if(!LittleFS.begin())
	{
		Serial << ( F( "An Error has occurred while mounting LittleFS" )) << endl;
		return;
	}
	
	for(int i=0; i<17; i=i+8) 
	{
		chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
	}
	Serial << ("MAC Address: ");
	Serial.println( chipId, HEX);  // this won't print correctly using the stdout notation

	Serial << ("WiFi STA.") << endl;
 	WiFi.mode(WIFI_STA);


	Serial << ( "Connecting to WiFi." ) << endl;
	WiFi.begin( ssid, password );
	while( WiFi.status() != WL_CONNECTED )
	{
		delay(500);
		Serial << (".");
	}
	Serial << endl;
	Serial << ("WiFi connected.") << endl;
	Serial << ("Wifi RSSI=") << WiFi.RSSI() << endl;
	Serial << ( WiFi.localIP() ) << endl;

	Serial.printf("ESP32 Chip model = %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
	Serial.printf("This chip has %d cores\n", ESP.getChipCores());

	initTime();

} // end setup()


void loop() {
  // put your main code here, to run repeatedly:

	Serial << ( "high") << endl;
	delay(1500);	// wait for half a second or 500 milliseconds

	Serial << ( "low") << endl;

	delay(1500);	// wait for half a second or 500 milliseconds

	Serial << getDateTimeString().c_str() << endl;

	// if( Ping.ping("www.google.com", 3) )
	// {
	// 	Serial << ("Ping succesful.") << endl;
	// }
	// else
	// {
	// 	Serial << ("Ping failed.") << endl;

	// }

	// no more blinking if wifi dies
	while(WiFi.status() != WL_CONNECTED)
	{
		delay( 1000 );
	}
}


void initTime()
{
	struct tm timeinfo;

	// Init and get the time
	configTime( 0, 0, ntpServer1, ntpServer2 );
	// configTzTime( "CST6CDT,M3.2.0,M11.1.0", ntpServer );

	if( !getLocalTime( &timeinfo ) )
	{
		Serial.println( "Failed to obtain time" );
		return;
	}

	// this is "America/Chicago" or central time
	setTimezone( "CST6CDT,M3.2.0,M11.1.0" );
	Serial << getDateTimeString().c_str() << endl;
}


// takes the special timezone strings
void setTimezone(String timezone)
{
	Serial.printf( "  Setting Timezone to %s\n",timezone.c_str() );
	setenv( "TZ",timezone.c_str(), 1 );  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
	tzset();
}


// 
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