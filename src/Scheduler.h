#ifndef __SCHEDULER_H__
 #define __SCHEDULER_H__

#include "Arduino.h"
#include <string>
// #include <ezTime.h>
#include <time.h>		// time() ctime()
#include <sys/time.h>		// struct timeval

// simplified list of time zones
typedef enum
{
	none,
	ePST,
	eMST,
	eCST,
	eEST
} __attribute__((packed)) timezone_e;

typedef enum
{
	AM,
	PM
} __attribute__((packed)) ampm_e;

typedef struct
{
	uint8_t		hour;
	uint8_t		minute;
	ampm_e		ampm;
	float		temperature;
} __attribute__((packed)) setTime_t;


typedef struct
{
	setTime_t setting[ 4 ];
} __attribute__((packed)) sched_t;

// two schedules, one for heating, one for cooling
typedef sched_t schedAry_t[ 2 ];

typedef struct
{
	uint8_t		hour;
	uint8_t		minute;
	ampm_e		ampm;
	uint8_t		runTime;	// howlong to run the fan in minutes
} __attribute__((packed)) fanTime_t;

typedef fanTime_t fanTimeAry_t[ 2 ];


typedef struct 
{
	bool	newValue;
	float	temp;
} newTemperature_t;

typedef struct
{
	bool	newValue;
	uint8_t	fanTime;
} newFanTime_t;


typedef enum
{
	eOff,
	eCool,
	eHeat
} SchedMode_e;

// bool getLocalTime(struct tm * info);

class Scheduler
{
	public:
		Scheduler();

		newTemperature_t tickTemperature( SchedMode_e mode );
		newFanTime_t tickFan( void );
		void init( timezone_e tz );
		void loadSchedule( schedAry_t *sched );
		void loadFanSched( fanTimeAry_t *fanSched );
		void setTimezone( String timezone );
		std::string getDateTimeString();



		std::string		timeZoneStr [5];		// array of strings that holds the time zone options
		// Timezone		myTZ;					// create a time & timezone object
		struct tm timeinfo;
		timezone_e 		tz;						// track the currently selected time zone
		bool			weHaveTime;				// boolean to track if NTP had a failure

		// the array is 8 because 0 is not used due to the 
		// pre-defined day values from ezTime.h
		schedAry_t  *schedule;
		fanTimeAry_t *fanTime;

	private:
};

 #endif