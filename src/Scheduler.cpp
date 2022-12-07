#include "Arduino.h"
#include <string>
#include <Streaming.h>
#include <time.h>		// time() ctime()
#include <sys/time.h>		// struct timeval

#include <Scheduler.h>

// our NTP servers
const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";

// const char* ntpServer = "pool.ntp.org";
const int daylightOffset_sec = 3600;
const long gmtOffset_sec = 0;
// struct tm timeinfo;

// constructor
Scheduler::Scheduler()
{
	// time zone string array
	// must be in the same order as timezone_e
	timeZoneStr[0] = std::string();	//empty string
	timeZoneStr[1] = "PST8PDT,M3.2.0,M11.1.0"; //"America/Los_Angeles";
	timeZoneStr[2] = "MST7MDT,M3.2.0,M11.1.0"; //"America/Denver";
	timeZoneStr[3] = "CST6CDT,M3.2.0,M11.1.0"; //"America/Chicago";
	timeZoneStr[4] = "EST5EDT,M3.2.0,M11.1.0"; //"America/New_York";

	weHaveTime = false;
}


// tickTemperature is run
newTemperature_t Scheduler::tickTemperature( SchedMode_e mode )
{
	setTime_t storedTime;
	newTemperature_t newTemp = { .newValue = false, .temp = 0.0f };

	// run the ezTime task
	// this will poll pool.ntp.org about every 30 minutes
	// events();
	

	// // day of week, dow
	// uint8_t dow = myTZ.weekday();
	
	// // off, don't search or update the temperature
	// if( mode == eOff )
	// 	return newTemp;

	// // shift the mode down by 1, so that we don't have to waste 
	// // ram for an Off schedule.  Only heat and cool are in the arrays
	// mode = (SchedMode_e)(mode - 1);

	// check date & time against the stored schedule
	// for( int idx = 0; idx < 4; idx++ )
	// {
	// 	storedTime.hour = schedule[dow][mode].setting[idx].hour;
	// 	storedTime.minute = schedule[dow][mode].setting[idx].minute;
	// 	storedTime.ampm = schedule[dow][mode].setting[idx].ampm;

	// 	if( storedTime.hour > 0 )
	// 	{
	// 		if( myTZ.isAM() == storedTime.ampm )
	// 		{
	// 			if( myTZ.hourFormat12() == storedTime.hour )
	// 			{
	// 				if( myTZ.minute() == storedTime.minute )
	// 				{
	// 					// we have a match!
	// 					Serial << "We have a sched match!" << endl;
	// 					Serial << "storedTime: " << storedTime.hour << ":" << storedTime.minute << ((storedTime.ampm == AM ) ? "AM" : "PM" ) << endl;
	// 					Serial << "time: " << myTZ.hourFormat12() << ":" << myTZ.minute() << ( (myTZ.isAM() ) ? "AM" : "PM" ) << endl;
	// 					newTemp.newValue = true;
	// 					newTemp.temp =  schedule[dow][mode].setting[idx].temperature;
	// 					break;
	// 				}
	// 			}
	// 		}
	// 	}
	// }

	
	// if there is a match then indicate that the temperature should be updated
	return newTemp;
}


newFanTime_t Scheduler::tickFan( void )
{
	setTime_t storedTime;
	newFanTime_t newFanRun = { .newValue = false, .fanTime = 0 };

	// day of week, dow
	// uint8_t dow = myTZ.weekday();

	// // now look if we have a fan run time to complete
	// for( int idx = 0; idx < 2; idx++ )
	// {
	// 	storedTime.hour = fanTime[dow][idx].hour;
	// 	storedTime.minute = fanTime[dow][idx].minute;
	// 	storedTime.ampm = fanTime[dow][idx].ampm;

	// 	if( storedTime.hour > 0 )
	// 	{
	// 		if( myTZ.isAM() == storedTime.ampm )
	// 		{
	// 			if( myTZ.hourFormat12() == storedTime.hour )
	// 			{
	// 				if( myTZ.minute() == storedTime.minute )
	// 				{
	// 					// we have a fan match
	// 					Serial << "We have a FAN match!" << endl;
	// 					break;
	// 				}
	// 			}
	// 		}

	// 	}
	// }

	return newFanRun;
}

void Scheduler::init( timezone_e new_tz )
{
	this->tz = new_tz;

	// Init and get the time
	configTime( 0, 0, ntpServer1, ntpServer2 );

	Serial << (F( "Syncing NTP" ) ) << endl;
	if( !getLocalTime( &timeinfo ) )
	{
		Serial.println("Failed to obtain time");
		return;
	}
	
	// this is "America/Chicago" or central time "CST6CDT,M3.2.0,M11.1.0"
	setTimezone( timeZoneStr[ tz ].c_str() );
	weHaveTime = true;

	Serial << getDateTimeString().c_str() << endl;

	// Provide official timezone names
	// https://en.wikipedia.org/wiki/List_of_tz_database_time_zones
	Serial <<  F("Timezone: " ) << timeZoneStr[ tz ].c_str() << endl;
}

std::string Scheduler::getDateTimeString()
{
	struct tm timeinfo;

	if( !getLocalTime( &timeinfo ) )
	{
		Serial.println( "Failed to obtain time" );
		return( "Failed to obtain time" );
	}

	Serial.println( &timeinfo, "%A, %I:%M:%S %p" );
	
	//11 chars each should be enough
	char timeStringBuff[20];
	char dayStringBuff[15];
	strftime( dayStringBuff, sizeof(dayStringBuff), "%A, ", &timeinfo );
	strftime( timeStringBuff, sizeof(timeStringBuff), "%I:%M:%S %p", &timeinfo );
	
	// remove leading zero from time if it exists
	if( timeStringBuff[ 0 ] == '0' )
		timeStringBuff[ 0 ] = ' ';

	// create std strings from the char arrays, then concat them in the return
	std::string foo( dayStringBuff );
	std::string bar( timeStringBuff );
	return foo + bar;
}


void Scheduler::setTimezone( String timezone )
{
	Serial.printf( "Setting Timezone to %s\n", timezone.c_str() );

	//  Now adjust the TZ.  Clock settings are adjusted to show the new local time
	setenv( "TZ", timezone.c_str(), 1 );
	tzset();
}


// assume control over the schedule data
void Scheduler::loadSchedule( schedAry_t *sched )
{
	this->schedule = sched;
}


void Scheduler::loadFanSched( fanTimeAry_t *fanSched )
{
	this->fanTime = fanSched;
}