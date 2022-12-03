#include <Arduino.h>
#include <EEPROM.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <string>
#include <queue>
#include <Streaming.h>

#include <Scheduler.h>

#include <myThermostat.h>

uint8_t BME280_i2caddr = 0x76;

#define I2C_SDA ( 0x04 )
#define I2C_SCL ( 0x05 )
TwoWire I2CBME = TwoWire(0);


// create the initial data structures
// and connect to the BME280
MyThermostat::MyThermostat( )
{
	// read in the eeprom settings
	eepromInit();
	fanRunTime = 0;
}


MyThermostat::MyThermostat( mode_e mode )
{
	int addr = 0;

	// read in the eeprom settings
	eepromInit();

	fanRunTime = 0;

	eepromData.mode = mode;
	EEPROM.put( addr, eepromData );
}


// init function must be called after Serial has been instantiated
// in setup() is a good place
void MyThermostat::init()
{
	// default settings
	safeToRunCompressor = true;
	setCompressorOffTime( eepromData.compressorOffDelay );

	// use the I2CBME object to reasign the i2c pins
	I2CBME.begin(I2C_SDA, I2C_SCL, 100000);

	unsigned status;
    status = bme.begin( BME280_i2caddr,  &I2CBME );  
    if (!status) {
        Serial << (F("Could not find a valid BME280 sensor, check wiring, address, sensor ID!")) << endl;
        Serial << (F("SensorID was: 0x")); Serial.println(bme.sensorID(),16);
        Serial << (F("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085")) << endl;
        Serial << (F("   ID of 0x56-0x58 represents a BMP 280,")) << endl;
        Serial << (F("        ID of 0x60 represents a BME 280.")) << endl;
        Serial << (F("        ID of 0x61 represents a BME 680.")) << endl;
        // while (1) { delay(10); }
    }
    
	bme.setSampling(
		Adafruit_BME280::MODE_FORCED,
		Adafruit_BME280::SAMPLING_X1, // temperature
		Adafruit_BME280::SAMPLING_X1, // pressure
		Adafruit_BME280::SAMPLING_X1, // humidity
		Adafruit_BME280::FILTER_OFF,
		Adafruit_BME280::STANDBY_MS_1000
		);
	// suggested rate is 1/60Hz (1m)

    Serial << (F("\n-- BME280 connected on address 0x76 --")) << endl;

	// configure GPIO
	Serial << (F("\n-- Configuring GPIO --")) << endl;
	digitalWrite( GPIO_FAN, LOW );
	digitalWrite( GPIO_COMPRESSOR, LOW );
	digitalWrite( GPIO_OB, LOW );
	digitalWrite( GPIO_EMGHEAT, LOW );

	pinMode( GPIO_FAN, OUTPUT );
	pinMode( GPIO_COMPRESSOR, OUTPUT );
	pinMode( GPIO_OB, OUTPUT );
	pinMode( GPIO_EMGHEAT, OUTPUT );
}


float MyThermostat::getTempRaw()
{
	float temp = bme.readTemperature();
	return temp * 1.8 + 32;
}


float MyThermostat::getTemperature_f() 
{
	static bool firstRun = true;
	float temperature;
	static float outputTemp;
	static float a = 16.0f;

	// read the temperature sensor
	temperature = bme.readTemperature();

	// convert to Fahrenheit
	temperature = 1.8f * temperature + 32.0f;

	// check for invalid value and put in -1 so 
	// the user can see something is wrong
	if (isnan( temperature )) 
	{
		temperature = -1.0f;
	}

	// // pre-load the array on the first run
	if( firstRun )
	{
		outputTemp = temperature;
		firstRun = false;
	}


	// low pass filter
	outputTemp += ( temperature - outputTemp ) / a;
	
	// calculate the slope of the temperatures over time
	calculateSlope( outputTemp );

	return outputTemp;
}

// calculate the slope of the temperature over time
void MyThermostat::calculateSlope( float temperature )
{
	// static bool firstRun = true;
	temperatureQue.push( temperature );

	if( temperatureQue.size() > 15 )
	{
		float prev;
		float cur;
		slope = 0;

		prev = temperatureQue.front();
		temperatureQue.pop();
		Serial << "slope: " << prev << ",";
		
		while( temperatureQue.size() )
		{
			cur = temperatureQue.front();
			temperatureQue.pop();
			Serial << cur << ",";
			slope += cur - prev;
			prev = cur;
		}
		Serial << ": "<< slope << endl;

		newSlope = true;
	}
}

float MyThermostat::getSlope( void )
{
	return slope;
}

void MyThermostat::setOldSlope( void )
{
	newSlope = false;
}

bool MyThermostat::isNewSlope( void )
{
	return newSlope;
}

// get the temperature and convert it to a string
std::string MyThermostat::getTemperature()
{
	return std::to_string( getTemperature_f() );
}


// Returns the pressure from the sensor (in Pascal)
float MyThermostat::getPressure_f()
{
	float pressure;
	static float outputPressure;
	static float a = 16.0f;
	static bool firstRun = true;

	// read the temperature sensor and convert to in-Hg
	pressure = bme.readPressure() / 3386.3886666667f;

	// // pre-load the array on the first run
	if( firstRun )
	{
		outputPressure = pressure;
		firstRun = false;
	}

	// low pass filter
	outputPressure += ( pressure - outputPressure ) / a;
	
	// Serial.print("Pressure:	");
	// Serial.println( pressure );
	return outputPressure;
}


// return a string of the pressure
std::string MyThermostat::getPressure()
{
	return std::to_string( getPressure_f() );
}


float MyThermostat::getHumidity_f() 
{
	static bool firstRun = true;
	static float humidArry[ 4 ] = {0,0,0,0};
	static unsigned char idx = 0;
	float humidity;

	humidity = bme.readHumidity();

	// check for invalid value and put in -1 so 
	// the user can see something is wrong
	if (isnan( humidity )) 
	{
		humidity = -1.0f;
	}

	// store and average this reading in with the rest
	humidArry[ idx ] = humidity;
	idx++;
	idx &= 0x03;

	if( firstRun )
	{
		humidArry[3] = humidArry[2] = humidArry[1] = humidArry[0];
		firstRun = false;
	}

	// reuse the temp variable, and get the average
	humidity = 0;
	for( unsigned char loop = 0; loop < 4; loop++ )
	{
		humidity += humidArry[ loop ];
	}
	humidity /= 4.0f;

	// Serial.print( "Humidity:	");
	// Serial.println( humidity );
	return humidity;
}


std::string MyThermostat::getHumidity()
{
	return std::to_string( getHumidity_f() );
}


// check what mode we are in
bool MyThermostat::isMode( mode_e mode )
{
	if( mode == eepromData.mode )
		return true;
	else
		return false;
}


// set the mode setting
// usually based upon a user input
void MyThermostat::setMode( mode_e mode )
{
	int addr = 0;
	eepromData.mode = mode;
	EEPROM.put( addr, eepromData );
}


// return the mode
mode_e MyThermostat::getMode( void )
{
	return eepromData.mode;
}


// get the readings, which triggers reading the sensors, 
// but don't bother to return the values
void MyThermostat::updateMeasurements( void )
{
	bme.takeForcedMeasurement();
	getTemperature_f();
	getHumidity_f();
	getPressure_f();
}


// run background logic and timers
void MyThermostat::runSlowTick( void )
{
	// decrement or clear the run once flag, so the fan can be set to run again
	decrementFanRunTime();
	decrementAuxRunTime();

	// decrement and 
	decrementCompressorOffTime();

	if( getCompressorOffTime() == 0 )
		setSafeToRunCompressor( true );
}


// this is the fast tickTemperature
void MyThermostat::loopTick( void )
{
	// run the schedule tickTemperature, and see if there is a new value returned
	newTemperature_t newTemp = mySched.tickTemperature( ( SchedMode_e )this->getMode() );

	// if there is a new value, then set the temperature
	if( newTemp.newValue == true )
	{
		setTemperatureSetting( newTemp.temp );
	}

	newFanTime_t newFanTime = mySched.tickFan();
	if( newFanTime.newValue == true )
	{
		// run the fan!
	}
}


// read the non-volatile temperature setting for the mode we are in
float MyThermostat::getTemperatureSetting( void )
{
	if( isMode( MODE_COOLING ) )
	{
		return eepromData.coolTemp;
	}
	else
	if( isMode( MODE_HEATING ) )
	{
		return eepromData.hotTemp;
	}
	else
	{
		return -66.6f;
	}
}


// set the non-volatile temperature setting for the mode we are in
void MyThermostat::setTemperatureSetting( float newTemp )
{
	int addr = 0;
	
	if( isMode( MODE_COOLING ) )
	{
		eepromData.coolTemp = newTemp;
		Serial.println( "isMode: cooling" );
	}
	else
	if( isMode( MODE_HEATING ) )
	{
		eepromData.hotTemp = newTemp;
		Serial.println( "isMode: heating" );
	}
	else
	if( isMode( MODE_OFF ) )
	{
		Serial.println( "isMode: off" );
	}
	else
	{
		Serial.println( "isMode: unknown" );
	}

	EEPROM.put( addr, eepromData );
}


void MyThermostat::setTempHysteresis( float hys )
{
	int addr = 0;
	eepromData.hysteresis = hys;
	EEPROM.put( addr, eepromData );
}

float MyThermostat::getTempHysteresis( void )
{
	return eepromData.hysteresis;
}


// set the fan run time in seconds (10 seconds minimum)
// set to 0 to turn off
void MyThermostat::setFanRunTime( unsigned long time )
{
	if( time < 10UL )
		time = 0UL;

	fanRunTime = time / 10UL;
}


// returns the remaining fan run time in seconds
unsigned long MyThermostat::getFanRunTime( void )
{
	return fanRunTime * 10UL;
}


void MyThermostat::decrementFanRunTime( void )
{
	if( fanRunTime )
		fanRunTime--;
}

void MyThermostat::turnOffFan( void )
{
	// only turn off the fan if we are not activley heating or cooling
	if( MODE_OFF == this->currentState() )
		digitalWrite( GPIO_FAN, LOW );

	this->setFanRunTime( 0ul );
}

void MyThermostat::turnOnFan( void )
{
	digitalWrite( GPIO_FAN, HIGH );
}

// turn off the cooler but set the fan to run for a while
void MyThermostat::turnOffCooler( void )
{
	if( !fanRunOnce )
	{
		fanRunOnce = true;
		setFanRunTime( eepromData.fanDelay );
		setCompressorOffTime( eepromData.compressorOffDelay );
	}

	// turn off the compressor
	digitalWrite( GPIO_COMPRESSOR, LOW );
	digitalWrite( GPIO_OB, LOW );


	// shadow the mode based on GPIO
	currentMode = MODE_OFF;

	// turn off the fan after a delay
	if( getFanRunTime() == 0 )
	{
		// don't use turnOffFan(), it will set the FanRunTime to 0 
		// and prevent use when not running
		digitalWrite( GPIO_FAN, LOW );
	}
		
}


bool MyThermostat::turnOnCooler( void )
{
	// make sure that the heater is off
	//digitalWrite( GPIO_OB, LOW );

	if( isSafeToRunCompressor() )
	{
		// the active state is cooling turn on the fan and compressor
		turnOnFan();
		digitalWrite( GPIO_COMPRESSOR, HIGH );
		digitalWrite( GPIO_OB, HIGH );

		// shadow the mode based on GPIO
		currentMode = MODE_COOLING;

		// true, the GPIO changed
		return true;
	}

	// no GPIO changed
	return false;
}


void MyThermostat::turnOffHeater( void )
{
	if( !fanRunOnce )
	{
		fanRunOnce = true;
		setFanRunTime( eepromData.fanDelay );
		setCompressorOffTime( eepromData.compressorOffDelay );
	}

	// turn off the compressor
	digitalWrite( GPIO_COMPRESSOR, LOW );

	// turn off AUX / Emergency heat
	turnOffAuxHeater();

	// shadow the mode based on GPIO
	currentMode = MODE_OFF;

	// turn off the fan after a delay
	if( getFanRunTime() == 0 )
	{
		// don't use turnOffFan(), it will set the FanRunTime to 0 
		// and prevent use when not running
		digitalWrite( GPIO_FAN, LOW );
	}
}


bool MyThermostat::turnOnHeater( void )
{
	// make sure that the cooler is off
	digitalWrite( GPIO_OB, LOW );

	if( isSafeToRunCompressor() )
	{
		// the active state is cooling turn on the fan and compressor
		turnOnFan();
		digitalWrite( GPIO_COMPRESSOR, HIGH );

		// shadow the mode based on GPIO
		currentMode = MODE_HEATING;
		
		// true, the GPIO changed
		return true;
	}

	// no GPIO changed
	return false;
}

// set the aux heater run time in seconds (10 seconds minimum)
// set to 0 to turn off
void MyThermostat::setAuxRunTime( unsigned long time )
{
	if( time < 10UL )
		time = 0UL;

	auxRunTime = time / 10UL;
}


// returns the remaining aux run time in seconds
unsigned long MyThermostat::getAuxRunTime( void )
{
	return auxRunTime * 10;
}


void MyThermostat::decrementAuxRunTime( void )
{
	if( auxRunTime )
		auxRunTime--;
}

// set the GPIO for the AUX heater to on
// time is the number of seconds to run
// if time is passed in, add it to the exiting time
bool MyThermostat::turnOnAuxHeater( void )
{
	Serial << "AUX ON" << endl;
	// Always make sure the fan is on if AUX is on
	turnOnFan();
	digitalWrite( GPIO_EMGHEAT, HIGH );
	return true;
}


// set the GPIO for the AUX heater to off
void MyThermostat::turnOffAuxHeater( void )
{
	Serial << "AUX OFF" << endl;
	digitalWrite( GPIO_EMGHEAT, LOW );
	setAuxRunTime( 0ul );
}


// return the current run mode
mode_e MyThermostat::currentState( void )
{
	return currentMode;
}

void MyThermostat::turnOffAll( void )
{
	setMode( MODE_OFF );

	// turn off the fan, except when it was turned on by the user/schedule
	if( getFanRunTime() == 0 )
		digitalWrite( GPIO_FAN, LOW );

	// turn off the heating & cooling
	digitalWrite( GPIO_OB, LOW );
	digitalWrite( GPIO_COMPRESSOR, LOW );
	turnOffAuxHeater();

	if( currentMode != MODE_OFF )
	{
		// set the flag to prevent short cycling (only once)
		setSafeToRunCompressor( false );
		setCompressorOffTime( eepromData.compressorOffDelay );
	}

	// shadow the mode based on GPIO
	currentMode = MODE_OFF;
}


void MyThermostat::clearFanRunOnce( void )
{
	fanRunOnce = false;
}


bool MyThermostat::isSafeToRunCompressor( void )
{
	return safeToRunCompressor;
}


void MyThermostat::setSafeToRunCompressor( bool safe )
{
	safeToRunCompressor = safe;
}


void MyThermostat::decrementCompressorOffTime( void )
{
	if( compressorOffTime )
		compressorOffTime--;
}


// returns the remaining fan run time in seconds
unsigned long MyThermostat::getCompressorOffTime( void )
{
	return compressorOffTime * 10;
}


// set the fan run time in seconds (10 seconds minimum)
// set to 0 to turn off
void MyThermostat::setCompressorOffTime( unsigned long time )
{
	compressorOffTime = time / 10;
}


// read in the eeprom
// if the cookie is invalid, then initialize the eeprom
void MyThermostat::eepromInit( void )
{
	// since we use a structure, the address is always 0
	int addr = 0;

	// initialize the eeprom datastructure
	// and allocate enough bytes for our structure
	EEPROM.begin( sizeof( eepromData ) );

	// read the bytes into our structure
	EEPROM.get( addr, eepromData );

	// check the cookie
	if( !eepromCookieIsValid() )
	{
		// write the initial eeprom values
		eepromWriteFirstValues();
	}
}


unsigned short MyThermostat::settings_getFanDelay( void )
{
	return eepromData.fanDelay;
}
unsigned short MyThermostat::settings_getCompressorOffDelay( void )
{
	return eepromData.compressorOffDelay;
}
unsigned short MyThermostat::settings_getCompressorMaxRuntime( void )
{
	return eepromData.compressorMaxRuntime;
}

void MyThermostat::settings_setFanDelay( unsigned short fanDelay )
{
	eepromData.fanDelay = fanDelay;
}
void MyThermostat::settings_setCompressorOffDelay( unsigned short compressorOffDelay )
{
	eepromData.compressorOffDelay = compressorOffDelay;
}
void MyThermostat::settings_setCompressorMaxRuntime( unsigned short compressorMaxRuntime)
{
	eepromData.compressorMaxRuntime = compressorMaxRuntime;
}


void MyThermostat::timeZone_set( timezone_e tz )
{
	eepromData.localTimeZone = tz;
}

timezone_e MyThermostat::timeZone_get( void )
{
	return eepromData.localTimeZone;
}


// return true if the cookie is valid
// otherwise return false
bool MyThermostat::eepromCookieIsValid( void )
{
	if( MAGIC_COOKIE == eepromData.cookie )
		return true;
	else
		return false;
}


// write some sane values to the eeprom
void MyThermostat::eepromWriteFirstValues( void )
{
	eepromData.cookie =		MAGIC_COOKIE;
	eepromData.coolTemp =	75.0f;
	eepromData.hotTemp =	63.5f;
	eepromData.hysteresis =	0.2f;
	eepromData.mode =		MODE_OFF;

	eepromData.fanDelay =			60;			// 2 minutes in seconds ( our heat pump runs for an additional 60 seconds after fan is told to turn off )
	eepromData.compressorOffDelay =	300;		// 5 minutes in seconds		
	eepromData.compressorMaxRuntime = 18000;	// 5 hours in seconds

	eepromData.localTimeZone = eCST;			// Central time is our default

	// defualt schedule is all off
	for( int dow = 0; dow > 8; dow++ )
	{
		for( int idx = 0; idx > 4; idx++ )
		{
			// cooling schedule
			eepromData.schedule[ dow ][ 0 ].setting[ idx ].hour = 0;
			eepromData.schedule[ dow ][ 0 ].setting[ idx ].minute = 0;
			eepromData.schedule[ dow ][ 0 ].setting[ idx ].ampm = AM;
			eepromData.schedule[ dow ][ 0 ].setting[ idx ].temperature = 0;

			// heating schedule
			eepromData.schedule[ dow ][ 1 ].setting[ idx ].hour = 0;
			eepromData.schedule[ dow ][ 1 ].setting[ idx ].minute = 0;
			eepromData.schedule[ dow ][ 1 ].setting[ idx ].ampm = AM;
			eepromData.schedule[ dow ][ 1 ].setting[ idx ].temperature = 0;
		}

		// fan schedule, 2 per day
		eepromData.fanTime[ dow ][ 0 ].ampm = AM;
		eepromData.fanTime[ dow ][ 0 ].hour = 0;
		eepromData.fanTime[ dow ][ 0 ].minute = 0;
		eepromData.fanTime[ dow ][ 0 ].runTime = 0;

		eepromData.fanTime[ dow ][ 1 ].ampm = AM;
		eepromData.fanTime[ dow ][ 1 ].hour = 0;
		eepromData.fanTime[ dow ][ 1 ].minute = 0;
		eepromData.fanTime[ dow ][ 1 ].runTime = 0;

	}

	this->saveSettings();

}


// update the eeprom values to make any changes permanent
// Note: it only wirtes the flash if it has been changed.
void MyThermostat::saveSettings( void )
{
	int addr = 0;

	// the put command writes local data back to 
	// the eeprom cache, but it isn't commited to flash yet 
	 EEPROM.put( addr, eepromData );

	// actually write the content of byte-array cache to
	// hardware flash.  flash write occurs if and only if one or more byte
	// in byte-array cache has been changed, but if so, ALL sizeof(eepromData) bytes are 
	// written to flash
	EEPROM.commit();
}


// read the output pin to check current value
int MyThermostat::digitalReadOutputPin(uint8_t pin)
{
	uint8_t bit = digitalPinToBitMask(pin);
	uint8_t port = digitalPinToPort(pin);

	if (port == NOT_A_PIN) 
		return LOW;

	return (*portOutputRegister(port) & bit) ? HIGH : LOW;
}


void MyThermostat::sched_init( void )
{
	// init the schedule with our timezone
	mySched.init( eepromData.localTimeZone );

	// load the cooling/heating schedule
	mySched.loadSchedule( eepromData.schedule );

	// load the fan schedule
	mySched.loadFanSched( eepromData.fanTime );
}