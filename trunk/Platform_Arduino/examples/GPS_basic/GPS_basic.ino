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
char  string[50];


void setup()
{
//  #ifdef DEBUG_PRINT_DEVELOPMENT
//    debug.Init();
//  #endif

//DEBUG_INIT;



  // initialization of serial line
  gsm.InitSerLine(115200);
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

}


void loop()
{
  gps.GetGPSData(&position, &time, &date);
  gps.GetGPSAntennaSupplyVoltage(&redout_voltage); 
  gps.GetGPSAntennaCurrent (&redout_current);
  gps.GetGPSSwVers(string);

  delay(1000);
}