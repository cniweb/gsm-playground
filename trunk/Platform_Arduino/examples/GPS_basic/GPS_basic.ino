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

  // wait until a GSM module is registered in the GSM network
  while (!gsm.IsRegistered()) {
    gsm.CheckRegistration();
    delay(1000);
  }
  

  ret_val = gps.ResetGPSModul(GPS_RESET_WARMSTART);
  
  //ret_val = gps.ResetGPSModul(GPS_RESET_COLDSTART);
  delay(5000);
  gps.ControlGPSAntenna(1);
  gps.GetGPSAntennaSupplyVoltage(&redout_voltage); 
  gps.GetGPSAntennaCurrent (&redout_current);
  gps.GetGPSSwVers(string);

}


void loop()
{

  gps.GetGPSData(&position, &time, &date);
GPSdebugserial.print("position->fix: "); GPSdebugserial.println(position.fix);



  if (position.fix) {
    // GPS data valid

    // send by debug interface

   
    GPSdebugserial.print("Lat: ");
    gps.ConvertPosition2String(&position, PART_LATITUDE, GPS_POS_FORMAT_1, string);
    GPSdebugserial.println(string);
    GPSdebugserial.print("Longitude: ");
    gps.ConvertPosition2String(&position, PART_LONGITUDE, GPS_POS_FORMAT_1, string);
    GPSdebugserial.println(string);

    //gsm.IsUserButtonPushed()
  }
  else {
    // send by debug interface
 GPSdebugserial.println("GPS data not valid");
  }

  delay(1000);
}