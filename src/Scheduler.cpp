
#include "Arduino.h"
#include <string>
// #include <ezTime.h>
#include <Streaming.h>
#include <time.h>		// time() ctime()
#include <sys/time.h>		// struct timeval

#include <Scheduler.h>

const char* ntpServer = "pool.ntp.org";
const int daylightOffset_sec = 3600;
const long gmtOffset_sec = 0;
// struct tm timeinfo;

// constructor
Scheduler::Scheduler()
{
	// time zone string array
	// must be in the same order as timezone_e
	timeZoneStr[0] = std::string();	//empty string
	timeZoneStr[1] = "America/Los_Angeles";
	timeZoneStr[2] = "America/Denver";
	timeZoneStr[3] = "America/Chicago";
	timeZoneStr[4] = "America/New_York";

	configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}


// bool getLocalTime(struct tm * info )
// {
// 	uint32_t ms = 5000;
//     uint32_t start = millis();
//     time_t now;
//     while((millis()-start) <= ms) {
//         time(&now);
//         localtime_r(&now, info);
//         if(info->tm_year > (2016 - 1900)){
//             return true;
//         }
//         delay(10);
//     }
//     return false;
// }

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

	// debug ezTime
	// setDebug(INFO);

	Serial << (F( "Syncing NTP" ) ) << endl;
	if(!getLocalTime(&timeinfo))
	{
		Serial.println("Failed to obtain time");
		return;
	}
	// wait for ezTime to sync
	// waitForSync();

	// Provide official timezone names
	// https://en.wikipedia.org/wiki/List_of_tz_database_time_zones
	Serial <<  F("Timezone: " ) << timeZoneStr[ tz ].c_str() << endl;
	// Serial <<  myTZ.dateTime() << endl;
	Serial << (&timeinfo, "%A, %B %d %Y %H:%M:%S") << endl;

	// myTZ.setLocation( timeZoneStr[ tz ].c_str() );
	// myTZ.setDefault();
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