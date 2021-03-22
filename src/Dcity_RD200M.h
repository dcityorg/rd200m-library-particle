#pragma once        // include this file only once in the compile

/*
  Dcity_RD200M.h

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
		I purchased a RD200 complete unit, and I replaced all the electronics in the RD200 to get WiFi data 
		sent to my blynk iPhone application.
		I used a Particle Photon microcontroller, but an ESP32 or ESP8266 would also be good choices.
		This forum has a lot of good information in it about using the RD200M sensor and taking apart the RD200 unit:
				https://community.openhab.org/t/smart-radon-sensor/53730
		RD200M Datasheet: https://github.com/Foxi352/radonsensor/blob/master/datasheet_RD200M_v1.2_eng.pdf


  	https://www.dcity.org/portfolio/rd200m-library/
    This link has details including:
      * software library installation for use with Arduino, Particle and related boards
      * list of functions available in these libraries
      * a demo program (which shows the usage of most library functions)

  	License Information:  https://www.dcity.org/license-information/
*/

// if you want this library to print some debug messages to the Serial port, then set this to 1, else set to 0
#define DEBUG 0


// rd200m definitions
#define RD200M_STX              			0x02		// 1st byte of cmd or response from the radon sensor
#define RD200M_REQUEST_ALL_DATA_CMD  	0x01		// command byte to request all data from sensor
#define RD200M_RESET_CMD         			0xA0		// command byte to reset the sensor
#define RD200M_SET_PERIOD_CMD    			0xA1		// command byte to set the period in minutes for how often sensor should send data
#define RD200M_DATA_RESULT   					0x10		// response byte from a read all data command
#define RD200M_DATA_RESULT_SIZE  			0x04		// there are 4 bytes in the data response from the sensor

#define RD200M_STATUS_UNDER_200_SECS  0x00		// measuring time is less than 200 seconds
#define RD200M_STATUS_UNDER_1_HOUR    0x01		// measuring time is less than 1 hour
#define RD200M_STATUS_HIGH_LEVEL			0x10		// measuring time is less than 30 minutes AND count is greater than 10
#define RD200M_STATUS_NORMAL     			0x02		// normal measurement, time is more than 1 hour
#define RD200M_STATUS_VIBRATION				0xE0		// invalid measurement due to vibration


// define the rd200m radon sensor class
template <typename RD200MUARTSerial>					// we use a template class to that we can pass in which serial port to use
class Dcity_RD200M
{
public:
	// constructor. Specify the desired serial port to talk to the sensor as  Serial or Serial1 or Serial2, etc
  Dcity_RD200M(RD200MUARTSerial& serial) : mySerial(serial) 
  {
    mySerial.template begin(19200,SERIAL_8N1);
  }
	// destructor
  ~Dcity_RD200M() 
  {
    mySerial.template end();
  }

	// returns the current radon value in units of pCi/l
	float getRadon()
	{
		return radonValue;
	}
	
	// returns the minutes of operation from the sensor... but this number resets to 0 every hour
	int getMinutes()
	{
		return minutes;
	}
	
	// returns the status value from the sensor. see the #define statements above for their meaning (e.g. RD200M_STATUS_NORMAL)
	int getStatus()
	{
		return status;
	}
	
	// tell the radon sensor to send data every x minutes (the period is specified in minutes)
	void setPeriod(uint8_t period)
	{
		setPeriodCommand[3] = period;		// set the period value into the command array
		mySerial.template write(setPeriodCommand, sizeof(setPeriodCommand));
	}

	// reset the rd200m - the sensor will take 30 minutes to start sending data 
  void reset() 
  {
  	// flush the serial input buffer (in case there was some error during the last serial transmission)
  	while (mySerial.template read() != -1 );
  	
  	mySerial.template write(resetCommand, sizeof(resetCommand));
  	  	
  }

	// sends the readAllDataCommand to the sensor so that it will return the radon value, minutes, and status data
	// this also clears out the processors serial receive buffer in case some characters were left in it by some error condition.
	// It typically is not necessary for the user to call this function, as the available() function will call it to get the new data.
	void requestData(void)
	{
  	// flush the serial input buffer (in case there was some error during the last serial transmission)
  	while (mySerial.template read() != -1 );
	
		mySerial.template write(readAllDataCommand, sizeof(readAllDataCommand));
	}

	// check if new data is available from the sensor. 
	// returns true if there is new data, else returns 0 if there isn't or there is some error.
	// waitTimeMS is the amount of time this function will wait to receive all 8 bytes of data from the radon sensor.
	//       typically 10 milliseconds is enough time.
	// This function calls requestData(), and then waits for the sensor to send the all 8 bytes.
	// All of the sensor data from the sensor must be in the serial receive buffer before this function will return true.
  bool available(unsigned long waitTimeMS) 
  {
  	uint16_t radonValueIntPart = 0;					// integer part of the radon value
		uint16_t radonValueFractionalPart = 0;	// fractional part of the radon value (e.g. .21)
		uint16_t tempMinutes = 0;								// minutes value returned from the sensor. This resets to 0 every hour
		uint16_t tempStatus = 0;								// status value returned from the sensor
		int      currentByteReceived = 0;				// needs to be an int since the serial read command will return -1 if no char is available
		uint8_t  bytesReceived = 0;							// number of bytes received from the sensor so far
		uint16_t checksumCalculated = 0;				// we calculate a checksum of the data received
		unsigned long startTimeMS = 0;			    // time in milliseconds that we started this function
  
  	startTimeMS = millis();
  
  	requestData();													// request data from the sensor.
  
  	// wait here until we get all 8 bytes in the serial receive buffer from the sensor or we have waited past our waitTimeMS
  	while ((mySerial.template available() < 8)  && (millis() - startTimeMS < waitTimeMS));
  
  	// if all of the sensor data is NOT in the serial buffer, then return 0
  	if (mySerial.template available() < 8)
  	{
  		#if DEBUG == 1
  			Serial.println("RD200M: Only " + String(mySerial.template available()) + " bytes were available in the buffer");
  		#endif
  		return 0;
  	}

		#if DEBUG == 1
			Serial.println("RD200M: " + String(mySerial.template available()) + " bytes were available in the buffer");
		#endif
		
    // while there are bytes in the serial buffer...
    // we could make this an if statement, and then only read in one byte each time we call this read function
    while (mySerial.template available() >= 0)
    {   
    	
    
			currentByteReceived = mySerial.template read();         // get the next byte available
			
			#if DEBUG == 1
				Serial.print("RD200M: next received byte = ");
				Serial.println(currentByteReceived,HEX);
			#endif
			
			// check if it is the 1st byte of a message from rd200m
			if ((currentByteReceived == RD200M_STX) && (bytesReceived == 0)) 
			{                      
				checksumCalculated = 0;                        // if so, start a new checksum value, 1st byte does not count
				bytesReceived = 1;                       //        and set the number of bytes received
			} 
			// else check if this byte is the 2nd byte in the message from the rd200m
			else if ((currentByteReceived == RD200M_DATA_RESULT) && (bytesReceived == 1)) 
			{
				checksumCalculated += currentByteReceived;             // update the checksum
				bytesReceived = 2;                       // update the number of bytes received
			} 
			// else check if this is the 3rd byte from the sensor
			else if ((currentByteReceived == RD200M_DATA_RESULT_SIZE) && (bytesReceived == 2)) 
			{
				checksumCalculated += currentByteReceived;             // update the checksum
				bytesReceived = 3;                       // update the number of bytes received
			}   
			
			// else check if this is the 4th byte from the sensor (status)
			else if ( bytesReceived == 3) 
			{
				tempStatus = currentByteReceived;												// get the status byte
				checksumCalculated += currentByteReceived;             // update the checksum
				bytesReceived = 4;                       // update the number of bytes received
			}      
			// else check if this is the 5th byte from the sensor (minutes)
			else if ( bytesReceived == 4) 
			{
				tempMinutes = currentByteReceived;												// get the minutes byte
				checksumCalculated += currentByteReceived;             // update the checksum
				bytesReceived = 5;                       // update the number of bytes received
			}        
			// else check if this is the 6th byte from the sensor (integer part of radon value)
			else if ( bytesReceived == 5) 
			{
				radonValueIntPart = currentByteReceived;												// get the integer part of radon value
				checksumCalculated += currentByteReceived;             // update the checksum
				bytesReceived = 6;                       // update the number of bytes received
			}      
			// else check if this is the 7th byte from the sensor (fractional part of radon value)
			else if ( bytesReceived == 6) 
			{
				radonValueFractionalPart = currentByteReceived;												// get the fractional part of radon value
				checksumCalculated += currentByteReceived;             // update the checksum
				bytesReceived = 7;                       // update the number of bytes received
			}   
			// else check if this is the 8th byte from the sensor (checksum)
			else if ( bytesReceived == 7) 
			{
				bytesReceived = 8;                       // update the number of bytes received
					
				// check if the checksums agree
				if (currentByteReceived == (0xFF - (checksumCalculated & 0xFF)))
				{
					// we have good data from the sensor
					minutes = tempMinutes;
					status = tempStatus;
					radonValue = radonValueIntPart + radonValueFractionalPart / 100.0;
					// flush the serial input buffer (in case there was some error during the last serial transmission)
        	while (mySerial.template read() != -1 );
        	
					return 1;		// return true, since we got good data
					
				}
				// we have a checksum error
				else
				{
					#if DEBUG == 1
						Serial.println("RD200M: Checksum error!!!");
					#endif
					
					// flush the serial input buffer (in case there was some error during the last serial transmission)
        	while (mySerial.template read() != -1 );
        
        	return 0;				
				}
			} 
			// else we have a problem... something was wrong with the data received
			else
			{
				#if DEBUG == 1
					Serial.println("RD200M: One of the received data bytes was not correct");
				#endif
				
				// flush the serial input buffer (in case there was some error during the last serial transmission)
        while (mySerial.template read() != -1 );
        
        return 0;
			}  
        
    }
    // else we have a problem... something was wrong with the data received
        
    #if DEBUG == 1
	    Serial.println("RD200M: No more data available in the while loop");
  	#endif
  	
		// flush the serial input buffer (in case there was some error during the last serial transmission)
    while (mySerial.template read() != -1 );    
    
    return 0;		// this getData command was not successful
  }    
        
  

private:
  RD200MUARTSerial& mySerial;
  
  float radonValue;					// radon value in pCi/l
  uint16_t minutes;					// minutes the sensor has been on... it resets at 60 automatically
  uint16_t status;					// status returned from radon sensor
  
  const unsigned char resetCommand[4] = { 0x02, 0xa0, 0x00, 0x5f };
  const unsigned char readAllDataCommand[4] = { 0x02, 0x01, 0x00, 0xfe };
  unsigned char setPeriodCommand[5] = { 0x02, 0xa1, 0x01, 0x0a, 0x53 };			// not a constant since we change the period value

};





