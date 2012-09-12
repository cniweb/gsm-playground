/*
    This sketch demonstrates how to use GPS connection

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "GPS_GE863.h"

//#ifdef DEBUG_PRINT_DEVELOPMENT
//  #include "SoftwareSerial.h"
//  #include "Debug.h"
//#endif


#include "SoftwareSerial.h"

#define rxPin A4
#define txPin A5
SoftwareSerial GPSdebugserial(rxPin, txPin);





// definition of instance of GPS class
GPS_GE863 gps;

// return variable
signed char ret_val;
uint16_t num_of_rx_bytes;
byte* ptr_to_data;
Position position;
Time time;
Date date;
signed char gps_data_valid;

unsigned short redout_voltage;
unsigned short redout_current;
char  string[100];



void setup()
{
//  #ifdef DEBUG_PRINT_DEVELOPMENT
//    debug.Init();
//  #endif

//DEBUG_INIT;

  GPSdebugserial.begin(57600);
  GPSdebugserial.println("Debug: GPS_basic.ino");

 
  // initialization of serial line
  gsm.InitSerLine(57600);
  // turn on GSM module
  gsm.TurnOn();

  
  
  #ifdef DEBUG_PRINT
    // print library version
    gsm.DebugPrint("DEBUG AT library version: ", 0);
    gsm.DebugPrint(gsm.LibVer(), 0);
    gsm.DebugPrint("DEBUG GSM library version: ", 0);
    gsm.DebugPrint(gsm.GSMLibVer(), 0);
    gsm.DebugPrint("DEBUG GPS library version: ", 0);
    gsm.DebugPrint(gps.GPSLibVer(), 1);
  #endif

  
  // reset GPS modul
  ret_val = gps.ResetGPSModul(GPS_RESET_WARMSTART);
  // delay after initialization
  delay(5000);
  // turn on GPS antenna
  gps.ControlGPSAntenna(1); 

  // read some technical data if necessary
  /*
  gps.GetGPSAntennaSupplyVoltage(&redout_voltage); 
  gps.GetGPSAntennaCurrent (&redout_current);
  gps.GetGPSSwVers(string);
  */

}


void loop()
{
  // check registration
  // ------------------
  gsm.CheckRegistration();

  // read GPS data
  // -------------
  gps_data_valid = gps.GetGPSData(&position, &time, &date);


  // enable user button if GSM modul is connected to the network
  // and GPS modul is connected to some satelites(=GPS data are valid)
  // -----------------------------------------------------------------
  if (gsm.IsRegistered() && (gps_data_valid == 1) ) {
    // GSM modul is registered and GPS data are valid
    // ----------------------------------------------
    gsm.EnableUserButton();
    gsm.TurnOnLED();
  }
  else {
    // not registered - so disable button
    // ----------------------------------
    gsm.DisableUserButton();
    gsm.TurnOffLED();
  }

  // send SMS with GPS position if user button is pushed
  // ---------------------------------------------------
  if (gsm.IsUserButtonEnable() && gsm.IsUserButtonPushed()) {
    GPSdebugserial.print("Lat: ");
    gps.ConvertPosition2String(&position, PART_LATITUDE, GPS_POS_FORMAT_1, string);
    GPSdebugserial.println(string);
    GPSdebugserial.print("Longitude: ");
    gps.ConvertPosition2String(&position, PART_LONGITUDE, GPS_POS_FORMAT_1, string);
    GPSdebugserial.println(string);
    GPSdebugserial.print("Altitude: ");
    gps.ConvertPosition2String(&position, PART_ALTITUDE, GPS_POS_FORMAT_1, string);
    GPSdebugserial.println(string);
  }
  
   
  // wait 500msec. before next iteration
  // -----------------------------------
  delay(500);
}