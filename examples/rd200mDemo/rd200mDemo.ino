

/*
  rd200mDemo.ino

  Written by: Gary Muhonen  gary@dcity.org

  Versions
    1.0.0 - 3/18/2021
      Original Release.

  Short Description:

		These files provide a software library and demo program for the Arduino
		platform devices (ESP32, Particle, Arduino, etc.). This library uses 
		a serial port, so you will need at least one. It's nice to use a 2nd
		serial port to talk to this radon sensor, so that you can use the primary
		serial port for software downloads and debugging.

		The library files provide useful functions to make it easy
		to communicate with the RD200M radon sensor.
		The demo program shows the usage of the functions in the library.

		This radon sensor is a high performance sensor, but it is not being sold by itself (to my knowledge).
		I purchased a RD200 complete unit, and I replaced all the electronics in the RD200 so I could send WiFi data 
	  to my blynk iPhone application.
		I used a Particle Photon microcontroller, but an ESP32 or ESP8266 would also be good choices.
		This forum has a lot of good information in it about using the RD200M sensor and taking apart the RD200 unit:
				https://community.openhab.org/t/smart-radon-sensor/53730
				
		RD200M Datasheet: https://github.com/Foxi352/radonsensor/blob/master/datasheet_RD200M_v1.2_eng.pdf


  	https://www.dcity.org/portfolio/rd200m-library/
    This link has details including:
      * software library installation for use with Arduino and Particle boards
      * list of functions available in these libraries
      * a demo program (which shows the usage of most library functions)


  This demo program is public domain. You may use it for any purpose.
    NO WARRANTY IS IMPLIED.

  License Information:  https://www.dcity.org/license-information/
*/

#include <Arduino.h>							// some platforms require this include file (e.g. ESP32)
#include <Dcity_RD200M.h>


// create rd200m Serial object - make the following changes as needed for your application:

// Use   <HardwareSerial>   for Arduino, ESP32 and most other related boards
// Use   <USARTSerial>      for Particle boards

// Use   (Serial1)  if sensor is connected to the second serial port on your processor
// Use   (Serial2)  if sensor is connected to the third serial port on your processor
// Use   (Serial3)  if sensor is connected to the forth serial port on your processor
// Use   (Serial)   if sensor is connected to the primary serial port (BUT this is usually used for USB programming)

Dcity_RD200M<USARTSerial> rd200m(Serial1);      



void setup() 
{
  Serial.begin(9600);   // start the console serial port for displaying messages
  delay(1000);
  Serial.println("Radon Sensor Test");
 
 	// Reset the radon sensor. No need to do this unless you move sensor to new location
 	// After a reset, the unit sends 0.00 as the radon value for 30 minutes.
	// rd200m.reset();
	// delay(2000); 			// this delay may not be necessary. Spec does not say if the unit needs some time to reset
}

void loop() 
{

  // request data from the sensor, and wait until the data is available. 
  // Returns true if valid data was received within the number of milliseconds (typically 10) that is specified as a parameter.
  // Note: The available() function will wait the specified number of milliseconds (e.g. 10 typical) to get the data from sensor.
  //       If this time is exceeded, this function will return false which means it did not get a valid response from the sensor in time.
  if (rd200m.available(10))
  {
  	Serial.println();
  
  	// get the radon value from the sensor.
  	// the sensor returns 0.0 for the first 30 minutes after a reset or power on.
  	Serial.println("Radon level: " + String(rd200m.getRadon()) + " pCi/L");

		// get the minutes the sensor has been on - this resets every hour back to 0
		Serial.println("Sensor run time is " + String(rd200m.getMinutes()) + " minutes (this resets every hour back to 0).");

		// get the status from the radon sensor
		switch(rd200m.getStatus())
		{
			case RD200M_STATUS_UNDER_200_SECS: Serial.println("Status: Run time is less than 200 seconds"); break; 
			case RD200M_STATUS_UNDER_1_HOUR:   Serial.println("Status: Run time is less than 1 hour"); break;
			case RD200M_STATUS_HIGH_LEVEL:     Serial.println("Status: Run time is less than 30 minutes - Warning! High Count!"); break;
			case RD200M_STATUS_NORMAL:         Serial.println("Status: Normal - Run time is more than 1 hour"); break;
			case RD200M_STATUS_VIBRATION:      Serial.println("Status: VIBRATION!"); break;
			default: 										       Serial.println("Status: Unknown status from radon sensor"); break;
		}
  
  } 
  // else data was not available from the sensor
  else
  {
  	Serial.println("Valid data was not available from radon sensor !!");
  }
  
  
  delay(4000);

  
  
}

